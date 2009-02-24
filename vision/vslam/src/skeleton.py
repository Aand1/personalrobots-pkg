import roslib
roslib.load_manifest('vslam')

import Image
from votools import TreeOptimizer3
import place_recognition
from visualodometer import VisualOdometer, Pose, DescriptorSchemeCalonder, DescriptorSchemeSAD, FeatureDetectorFast, FeatureDetector4x4, FeatureDetectorStar, FeatureDetectorHarris, from_xyz_euler
from stereo import SparseStereoFrame
from timer import Timer

import calonder

import pylab, numpy
import random
import pickle
import math

class minimum_frame:
  def __init__(self, id, kp, descriptors, matcher):
    self.id = id
    self.kp = kp
    self.descriptors = descriptors
    self.matcher = matcher
    assert len(kp) == len(descriptors)

def mk_covar(xyz, rp, yaw):
  return (1.0 / math.sqrt(xyz),1.0 / math.sqrt(xyz), 1.0 / math.sqrt(xyz), 1.0 / math.sqrt(rp), 1.0 / math.sqrt(rp), 1.0 / math.sqrt(yaw))
#weak = mk_covar(0.01, 0.0002, 0.002)
weak = mk_covar(5,3,3)
strong = mk_covar(0.0001, 0.000002, 0.00002)

def default_termcrit(count, delta):
  return ((count > 10) or (delta < 1e-1))

class Skeleton:

  def __init__(self, cam):
    self.nodes = set()
    self.edges = set()
    if 1:
      self.pg = TreeOptimizer3()
    else:
      import faketoro
      self.pg = faketoro.faketoro()
    self.pg.initializeOnlineOptimization()
    if 1:
      filename = '/u/mihelich/images/holidays/holidays.tree'
      self.vt = place_recognition.load(filename)
    else:
      self.vt = None
    self.place_ids = []
    self.vo = VisualOdometer(cam, feature_detector = FeatureDetectorFast(), descriptor_scheme = DescriptorSchemeCalonder())
    #self.vo = VisualOdometer(cam)

    self.node_kp = {}
    self.node_descriptors = {}
    self.node_matcher = {}

    self.termcrit = default_termcrit
    self.pr_maximum = 15    # How many out of PR's places to consider for GCC
    self.node_vdist = 15    # how many frame to wait to put in a skeleton node
    self.adaptive = False

    self.timer = {}
    for t in ['toro add', 'toro opt', 'place recognition', 'gcc', 'descriptors']:
      self.timer[t] = Timer()

  def save(self, basename):
    f = open(basename + ".pickle", "w")
    pickle.dump(self.nodes, f)
    pickle.dump(self.edges, f)
    pickle.dump(self.place_ids, f)
    pickle.dump(self.node_kp, f)
    pickle.dump(self.node_descriptors, f)
    f.close()
    self.pg.save(basename + ".toro")

  def load(self, basename):
    f = open(basename + ".pickle", "r")
    self.nodes = pickle.load(f)
    self.edges = pickle.load(f)
    self.place_ids = pickle.load(f)
    self.node_kp = pickle.load(f)
    self.node_descriptors = pickle.load(f)
    self.prev_pose = None
    f.close()

    for id in self.node_descriptors.keys():
      ma = calonder.BruteForceMatcher(self.vo.descriptor_scheme.cl.dimension())
      for sig in self.node_descriptors[id]:
        ma.addSignature(sig)
      self.node_matcher[id] = ma

    for id in self.place_ids:
      self.vt.add(None, self.node_descriptors[id])

    if 0:
      self.pg.load(basename + ".toro")
    else:
      edges = []
      for l in open(basename + ".toro"):
        fld = l.split()
        if fld[0] == 'EDGE3':
          flt = [ float(x) for x in fld[3:] ]
          cov = [flt[i] for i in [ 6,12,17,21,24,26 ]]
          edges.append((int(fld[1]), int(fld[2]), tuple(flt[0:3]), tuple(flt[3:6]), tuple(cov)))
      for e in sorted(edges):
        self.pg.addIncrementalEdge(*e)
      #self.pg.save("silly.toro")

  def add(self, this, connected = True):
    if len(self.nodes) == 0:
      self.nodes.add(this.id)
      self.prev_pose = this.pose
      r = True
    elif not(this.id in self.nodes):
      previd = max(self.nodes)

      # Ignore the node if there are less than node_vist frames since the previous node
      if (this.id - previd) < self.node_vdist:
        return

      if connected:
        print "Strong link from %d to %d" % (previd, this.id)
        relpose = ~self.prev_pose * this.pose
        inf = strong
      else:
        print "Weak link from %d to %d" % (previd, this.id)
        relpose = Pose(numpy.identity(3), [ 5, 0, 0 ])
        inf = weak

      self.nodes.add(this.id)
      self.edges.add( (previd, this.id) )
      self.timer['toro add'].start()
      self.pg.addIncrementalEdge(previd, this.id, relpose.xform(0,0,0), relpose.euler(), inf)
      print "ADDED VO CONSTRAINT", previd, this.id, inf
      self.timer['toro add'].stop()
      #print "added node at", this.pose.xform(0,0,0), "in graph as", self.newpose(this.id).xform(0,0,0)

      self.memoize_node_kp_d(this)
      this_d = self.node_descriptors[this.id]

      far = [ id for id in self.place_find(this.lf, this_d) if (not id in [this.id, previd])]
      self.place_ids.append(this.id)
      self.add_links(this.id, far)

      self.prev_pose = this.pose
      r = True
    else:
      r = False
    return r

  def addConstraint(self, prev, this, relpose):
    self.edges.add((prev, this))
    self.timer['toro add'].start()
    self.pg.addIncrementalEdge(prev, this, relpose.xform(0,0,0), relpose.euler(), strong)
    self.timer['toro add'].stop()

  def optimize(self):
    self.pg.initializeOnlineIterations()
    print "pg.error", self.pg.error()
    for j in range(1):
      for i in range(1000):
        #print "iter", i, "pg.error", self.pg.error()
        self.pg.iterate()
      print "pg.error", self.pg.error()
    #self.pg.recomputeAllTransformations()
    #self.pg.save("render5.graph")

  def newpose(self, id):
    xyz,euler = self.pg.vertex(id)
    return from_xyz_euler(xyz, euler)

  def place_find(self, lf, descriptors):
    if self.vt:
      self.timer['place recognition'].start()
      scores = self.vt.topN(lf, descriptors, len(self.place_ids), True)
      self.timer['place recognition'].stop()
      assert len(scores) == len(self.place_ids)+1
      return [id for (_,id) in sorted(zip(scores, self.place_ids))][:self.pr_maximum]
    else:
      return self.place_ids

  def add_links(self, this, far):
    self.timer['gcc'].start()
    coll = [ self.PE(this, f) + (f,) for f in far ]
    self.timer['gcc'].stop()
    id0 = this
    print coll
    for inl,obs,id1 in coll:
      if 40 <= inl:
        old_error = self.pg.error()
        self.addConstraint(id0, id1, obs)
        print "ADDED CONSTRAINT", id0, id1, "error changed from", old_error, "to", self.pg.error()

    t0 = self.timer['toro opt'].sum
    self.timer['toro opt'].start()
    self.pg.initializeOnlineIterations()
    count = 0
    self.pg.iterate()                   # error can go way up on first iterate
    prev_e = self.pg.error()
    self.pg.iterate()
    print
    print "Starting OPT loop, error ", self.pg.error(), " prev error ", prev_e
    while not self.termcrit(count, prev_e - self.pg.error()):
      prev_e = self.pg.error()
      self.pg.iterate()
      count += 1
    self.timer['toro opt'].stop()
    t1 = self.timer['toro opt'].sum
    td = t1 - t0
    if self.adaptive:
      if (td > 0.300):                    # too large, stretch frame additions
        self.node_vdist = 15 + (td - 0.4)*100
      else:
        self.node_vdist = 15
    print "OPT TIMER ", 1000.0*(t1-t0), "  ITERATIONS ", count, "  FRAMES ", self.node_vdist, "  ERROR ", self.pg.error()
    print

  def my_frame(self, id):
    return minimum_frame(id, self.node_kp[id], self.node_descriptors[id], self.node_matcher[id])

  def xmemoize_node_kp_d(self, af):
    self.timer['descriptors'].start()
    if not (af.id in self.node_kp):
      nf = SparseStereoFrame(af.lf, af.rf)
      self.vo.setup_frame(nf)
      self.node_kp[af.id] = nf.kp
      self.node_descriptors[af.id] = nf.descriptors
      self.node_matcher[af.id] = nf.matcher

      if 0:
        pylab.imshow(numpy.fromstring(af.lf.tostring(), numpy.uint8).reshape(480,640), cmap=pylab.cm.gray)
        pylab.scatter([x for (x,y,d) in nf.kp], [y for (x,y,d) in nf.kp], label = 'f0 kp', c = 'red')
        pylab.show()
        Image.fromstring("L", (640,480), af.lf.tostring()).save("star.png")
    self.timer['descriptors'].stop()

  def memoize_node_kp_d(self, af):
    self.timer['descriptors'].start()
    if not (af.id in self.node_kp):
      self.node_kp[af.id] = af.kp
      self.node_descriptors[af.id] = af.descriptors
      self.node_matcher[af.id] = af.matcher
    self.timer['descriptors'].stop()

  def PE(self, af0, af1):
    return self.vo.proximity(self.my_frame(af0), self.my_frame(af1))

  def correct_frame_pose(self, id):
    if id in self.nodes:
      return self.newpose(id)
    else:
      return None

  def drawable(self):

    pts = dict([ (id,self.newpose(id).xform(0,0,0)) for id in self.nodes ])
    nodepts = pts.values()
    edges = []
    for (f0,f1) in self.edges:
      p0 = pts[f0]
      p1 = pts[f1]
      edges.append((p0, p1))
    return (nodepts, edges)

  def plot(self, color, annotate = False):
    pts = dict([ (id,self.newpose(id).xform(0,0,0)) for id in self.nodes ])
    nodepts = pts.values()
    pylab.scatter([x for (x,y,z) in nodepts], [z for (x,y,z) in nodepts], c = color, label = 'after SGD')
    if annotate:
      for (f,(x,y,z)) in pts.items():
        pylab.annotate('%d' % f, (float(x), float(z)))

    for (f0,f1) in self.edges:
      p0 = pts[f0]
      p1 = pts[f1]
      pylab.plot((p0[0], p1[0]), (p0[2], p1[2]), c = color)

  # returns a summary of the skeleton - intended recipient is planning.
  def localization(self):
    def planar(x, y, z):
      from scipy import optimize
      def rms(sol, args):
          a,b,c,d = sol
          x,y,z = args
          return sum((y - (a*x + b*y + c*z + d)) ** 2)
      sol = [1.0, 1.0, 1.0, 0.0]
      sol = optimize.fmin(rms, sol, args=((x,y,z),))

      return sol

    Ns = sorted(self.nodes)
    poses = [ self.newpose(id) for id in Ns ]
    nodepts = [ p.xform(0,0,0) for p in poses ]
    nodedirs = [ p.xform(0,0,1) for p in poses ]
    a,b,c,d = planar(numpy.array([x for (x,y,z) in nodepts]), numpy.array([y for (x,y,z) in nodepts]), numpy.array([z for (x,y,z) in nodepts]))
    mag = math.sqrt(float(a*a + b*b + c*c))
    a /= mag
    b /= mag
    c /= mag
    plane_xform = numpy.array([[ b, c, a ], [ c, a, b ]])
    pos0 = [ tuple(numpy.dot(plane_xform, numpy.array( [ [x], [y], [z] ]))) for (x,y,z) in nodepts ]
    pos1 = [ tuple(numpy.dot(plane_xform, numpy.array( [ [x], [y], [z] ]))) for (x,y,z) in nodedirs ]
    u = [ p1[0] - p0[0] for (p0,p1) in zip(pos0, pos1) ]
    v = [ p1[1] - p0[1] for (p0,p1) in zip(pos0, pos1) ]
    thetas = [ math.atan2(vi, ui) for (ui, vi) in zip(u,v) ]
    return_positions = [ tuple(numpy.dot(plane_xform, numpy.array( [ [x], [y], [z] ])).transpose()[0]) + (thetas[i],) for (i, (x,y,z)) in enumerate(nodepts) ]

    reversal = dict([(Ns[i],i) for i in range(len(Ns))])

    return_edges = [ (reversal[a], reversal[b]) for (a,b) in self.edges ]

    if len(self.nodes) > 0:
      return_loc = reversal[max(self.nodes)]
    else:
      return_loc = -1
    return (return_positions, return_edges, return_loc)

    localizations = []
    self.vo.setup_frame(qf)
    far = [id for id in self.place_find(qf.lf, qf.descriptors)]
    print "far", far
    coll = [ self.vo.proximity(qf, self.my_frame(f)) + (f,) for f in far ]
    print coll
    for inl,obs,id1 in coll:
      if 40 <= inl:
        localizations.append((id1, obs))
    print localizations

  def summary(self):
    pts = dict([ (id,self.newpose(id).xform(0,0,0)) for id in self.nodes ])
    return (pts, self.edges)

  def average_time_per_frame(self):
    niter = len(self.nodes)
    return 1e3 * sum([t.sum for t in self.timer.values()]) / niter

  def summarize_timers(self):
    print "Graph has", len(self.nodes), "nodes and", len(self.edges), "edges"
    print
    niter = len(self.nodes)
    if niter != 0:
      for n,t in self.timer.items():
        print "%-20s %fms" % (n, 1e3 * t.sum / niter)
      print "%-20s %fms" % ("TOTAL", self.average_time_per_frame())
    #self.vo.summarize_timers()

  def dump_timers(self, filename):
    f = open(filename, 'w')
    d = dict([ (nm, tm.log) for (nm, tm) in self.timer.items() ])
    pickle.dump(d, f)
    f.close()
