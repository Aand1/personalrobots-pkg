import roslib
roslib.load_manifest('visual_odometry')
import rostest
import rospy

import vop

import sys
sys.path.append('lib')

import Image as Image
import ImageChops as ImageChops
import ImageDraw as ImageDraw
import ImageFilter as ImageFilter

import random
import unittest
import math

from stereo_utils.stereo import ComputedDenseStereoFrame, SparseStereoFrame
from stereo_utils.feature_detectors import FeatureDetectorFast, FeatureDetector4x4, FeatureDetectorStar, FeatureDetectorHarris
from stereo_utils.descriptor_schemes import DescriptorSchemeCalonder, DescriptorSchemeSAD
from visual_odometry.visualodometer import VisualOdometer, Pose
import fast
from math import *

import stereo_utils.camera as camera

import numpy
import numpy.linalg
#import pylab

import cairo
import array

from visual_odometry.pe import PoseEstimator
from raytracer import object, sphere, isphere, render_stereo_scene, ray_camera, vec3, shadeLitCloud

import cv

def rotation(angle, x, y, z):
  return numpy.array([
    [ 1 + (1-cos(angle))*(x*x-1)         ,    -z*sin(angle)+(1-cos(angle))*x*y   ,    y*sin(angle)+(1-cos(angle))*x*z  ],
    [ z*sin(angle)+(1-cos(angle))*x*y    ,    1 + (1-cos(angle))*(y*y-1)         ,    -x*sin(angle)+(1-cos(angle))*y*z ],
    [ -y*sin(angle)+(1-cos(angle))*x*z   ,    x*sin(angle)+(1-cos(angle))*y*z    ,    1 + (1-cos(angle))*(z*z-1)       ]])

def circle(im, x, y, r, color):
    draw = ImageDraw.Draw(im)
    box = [ int(i) for i in [ x - r, y - r, x + r, y + r ]]
    draw.pieslice(box, 0, 360, fill = color)

class imgStereo:
  def __init__(self, im):
    self.size = im.size
    self.data = im.tostring()
  def tostring(self):
    return self.data

class TestDirected(unittest.TestCase):

  def test_sad(self):
    im = Image.open("img1.pgm")
    fd = FeatureDetectorStar(300)
    ds = DescriptorSchemeSAD()
    af = SparseStereoFrame(im, im, feature_detector = fd, descriptor_scheme = ds)
    for (a,b) in af.match(af):
      self.assert_(a == b)

  def xtest_smoke(self):
    cam = camera.Camera((389.0, 389.0, 89.23, 323.42, 323.42, 274.95))
    vo = VisualOdometer(cam)
    vo.reset_timers()
    dir = "/u/konolige/vslam/data/indoor1/"

    trail = []
    prev_scale = None

    schedule = [(f+1000) for f in (range(0,100) + range(100,0,-1) + [0]*10)]
    schedule = range(1507)
    schedule = range(30)
    for f in schedule:
      lf = Image.open("%s/left-%04d.ppm" % (dir,f))
      rf = Image.open("%s/right-%04d.ppm" % (dir,f))
      lf.load()
      rf.load()
      af = SparseStereoFrame(lf, rf)

      vo.handle_frame(af)
      print f, vo.inl
      trail.append(numpy.array(vo.pose.M[0:3,3].T)[0])
    def d(a,b):
      d = a - b
      return sqrt(numpy.dot(d,d.conj()))
    print "covered   ", sum([d(a,b) for (a,b) in zip(trail, trail[1:])])
    print "from start", d(trail[0], trail[-1]), trail[0] - trail[-1]

    vo.summarize_timers()
    print vo.log_keyframes

  def xtest_smoke_bag(self):
    import rosrecord
    import visualize

    class imgAdapted:
      def __init__(self, i):
        self.i = i
        self.size = (i.width, i.height)
      def tostring(self):
        return self.i.data

    cam = None
    filename = "/u/prdata/videre-bags/loop1-mono.bag"
    filename = "/u/prdata/videre-bags/vo1.bag"
    framecounter = 0
    for topic, msg in rosrecord.logplayer(filename):
      print framecounter
      if rospy.is_shutdown():
        break
      #print topic,msg
      if topic == "/videre/cal_params" and not cam:
        cam = camera.VidereCamera(msg.data)
        vo = VisualOdometer(cam)
      if cam and topic == "/videre/images":
        if -1 <= framecounter and framecounter < 360:
          imgR = imgAdapted(msg.images[0])
          imgL = imgAdapted(msg.images[1])
          af = SparseStereoFrame(imgL, imgR)
          pose = vo.handle_frame(af)
          visualize.viz(vo, af)
        framecounter += 1
    print "distance from start:", vo.pose.distance()
    vo.summarize_timers()

  def test_solve_spin(self):
    # Test process with one 'ideal' camera, one real-world Videre
    camera_param_list = [
        (200.0, 200.0, 3.00,  320.0, 320.0, 240.0),
        (389.0, 389.0, 89.23, 323.42, 323.42, 274.95),
    ]
    for cam_params in camera_param_list:
      cam = camera.Camera(cam_params)
      vo = VisualOdometer(cam)

      kps = []
      model = [ (x*200,y*200,z*200) for x in range(-3,4) for y in range(-3,4) for z in range(-3,4) ]
      for angle in range(80):
        im = Image.new("L", (640, 480))
        theta = (angle / 80.) * (pi * 2)
        R = rotation(theta, 0, 1, 0)
        kp = []
        for (mx,my,mz) in model:
          pp = None
          pt_camera = (numpy.dot(numpy.array([mx,my,mz]), R))
          (cx,cy,cz) = numpy.array(pt_camera).ravel()
          if cz > 100:
            (x,y,d) = cam.cam2pix(cx, cy, cz)
            if 0 <= x and x < 640 and 0 <= y and y < 480:
              pp = (x,y,d)
              circle(im, x, y, 2, 255)
          kp.append(pp)
        kps.append(kp)

      expected_rot = numpy.array(numpy.mat(rotation(2 * pi / 80, 0, 1, 0))).ravel()

      for i in range(100):
        i0 = i % 80
        i1 = (i + 1) % 80
        pairs = [ (i,i) for i in range(len(model)) if (kps[i0][i] and kps[i1][i]) ]
        def sanify(L, sub):
          return [i or sub for i in L]
        (inliers, rot, shift) = vo.solve(sanify(kps[i0],(0,0,0)), sanify(kps[i1],(0,0,0)), pairs)
        self.assert_(inliers != 0)
        self.assertAlmostEqual(shift[0], 0.0, 2)
        self.assertAlmostEqual(shift[1], 0.0, 2)
        self.assertAlmostEqual(shift[2], 0.0, 2)
        for (et, at) in zip(rot, expected_rot):
          self.assertAlmostEqual(et, at, 2)

  def xtest_sim(self):
    # Test process with one 'ideal' camera, one real-world Videre
    camera_param_list = [
      # (200.0, 200.0, 3.00,  320.0, 320.0, 240.0),
      (389.0, 389.0, 1e-3 * 89.23, 323.42, 323.42, 274.95)
    ]
    def move_forward(i, prev):
      """ Forward 1 meter, turn around, Back 1 meter """
      if i == 0:
        return Pose(rotation(0,0,1,0), (0,0,0))
      elif i < 10:
        return prev * Pose(rotation(0,0,1,0), (0,0,.1))
      elif i < 40:
        return prev * Pose(rotation(math.pi / 30, 0, 1, 0), (0, 0, 0))
      elif i < 50:
        return prev * Pose(rotation(0,0,1,0), (0,0,.1))

    for movement in [ move_forward ]: # move_combo, move_Yrot ]:
      for cam_params in camera_param_list:
        cam = camera.Camera(cam_params)

        random.seed(0)
        def rr():
          return 2 * random.random() - 1.0
        model = [ (3 * rr(), 1 * rr(), 3 * rr()) for i in range(300) ]
        def rndimg():
          b = "".join(random.sample([ chr(c) for c in range(256) ], 64))
          return Image.fromstring("L", (8,8), b)
        def sprite(dst, x, y, src):
          try:
            dst.paste(src, (int(x)-4,int(y)-4))
          except:
            print "paste failed", x, y
        palette = [ rndimg() for i in model ]
        expected = []
        afs = []
        P = None
        for i in range(50):
          print "render", i
          P = movement(i, P)
          li = Image.new("L", (640, 480))
          ri = Image.new("L", (640, 480))
          q = 0
          for (mx,my,mz) in model:
            pp = None
            pt_camera = (numpy.dot(P.M.I, numpy.array([mx,my,mz,1]).T))
            (cx,cy,cz,cw) = numpy.array(pt_camera).ravel()
            if cz > .100:
              ((xl,yl),(xr,yr)) = cam.cam2pixLR(cx, cy, cz)
              if 0 <= xl and xl < 640 and 0 <= yl and yl < 480:
                sprite(li, xl, yl, palette[q])
                sprite(ri, xr, yr, palette[q])
            q += 1
          expected.append(P)
          afs.append(SparseStereoFrame(imgStereo(li), imgStereo(ri)))

      vo = VisualOdometer(cam)
      for i,(af,ep) in enumerate(zip(afs, expected)):
        vo.handle_frame(af)
        if 0:
          print vo.pose.xform(0,0,0)
          print "expected", ep.M
          print "vo.pose", vo.pose.M
          print numpy.abs((ep.M - vo.pose.M))
        self.assert_(numpy.alltrue(numpy.abs((ep.M - vo.pose.M)) < 0.2))

      return
      def run(vos):
        for af in afs:
          for vo in vos:
            vo.handle_frame(af)

      # Check that the pose estimators are truly independent

      v1 = VisualOdometer(cam, feature_detector = FeatureDetectorFast(), descriptor_scheme = DescriptorSchemeSAD(), inlier_error_threshold=1.0)
      v2 = VisualOdometer(cam, feature_detector = FeatureDetectorFast(), descriptor_scheme = DescriptorSchemeSAD(), inlier_error_threshold=2.0)
      v8 = VisualOdometer(cam, feature_detector = FeatureDetectorFast(), descriptor_scheme = DescriptorSchemeSAD(), inlier_error_threshold=8.0)
      v1a = VisualOdometer(cam, feature_detector = FeatureDetectorFast(), descriptor_scheme = DescriptorSchemeSAD(), inlier_error_threshold=1.0)
      run([v1])
      run([v2,v8,v1a])
      self.assert_(v1.pose.xform(0,0,0) == v1a.pose.xform(0,0,0))
      for a,b in [ (v1,v2), (v2,v8), (v1, v8) ]:
        self.assert_(a.pose.xform(0,0,0) != b.pose.xform(0,0,0))

      return

  def test_solve_rotation(self):

    cam = camera.Camera((389.0, 389.0, 89.23, 323.42, 323.42, 274.95))
    vo = VisualOdometer(cam)

    model = []

    radius = 1200.0

    kps = []
    for angle in range(80):
      im = Image.new("L", (640, 480))
      theta = (angle / 80.) * (pi * 2)
      R = rotation(theta, 0, 1, 0)
      kp = []
      for s in range(7):
        for t in range(7):
          y = -400
          pt_model = numpy.array([110 * (s - 3), y, 110 * (t - 3)]).transpose()
          pt_camera = (numpy.dot(pt_model, R) + numpy.array([0, 0, radius])).transpose()
          (cx, cy, cz) = [ float(i) for i in pt_camera ]
          (x,y,d) = cam.cam2pix(cx, cy, cz)
          reversed = cam.pix2cam(x, y, d)
          self.assertAlmostEqual(cx, reversed[0], 3)
          self.assertAlmostEqual(cy, reversed[1], 3)
          self.assertAlmostEqual(cz, reversed[2], 3)
          kp.append((x,y,d))
          circle(im, x, y, 2, 255)
      kps.append(kp)

    expected_shift = 2 * radius * sin(pi / 80)

    for i in range(100):
      i0 = i % 80
      i1 = (i + 1) % 80
      pairs = zip(range(len(kps[i0])), range(len(kps[i1])))
      (inliers, rod, shift) = vo.solve(kps[i0], kps[i1], pairs)
      actual_shift = sqrt(shift[0]*shift[0] + shift[1]*shift[1] + shift[2]*shift[2])

      # Should be able to estimate camera shift to nearest thousandth of mm
      self.assertAlmostEqual(actual_shift, expected_shift, 2)

  def xtest_image_pan(self):
    cam = camera.Camera((1.0, 1.0, 89.23, 320., 320., 240.0))
    vo = VisualOdometer(cam)
    prev_af = None
    pose = None
    im = Image.open("img1.pgm")
    for x in [0,5]: # range(0,100,10) + list(reversed(range(0, 100, 10))):
      lf = im.crop((x, 0, x + 640, 480))
      rf = im.crop((x, 0, x + 640, 480))
      af = SparseStereoFrame(lf, rf)

      if prev_af:
        pairs = vo.temporal_match(prev_af, af)
        pose = vo.solve(prev_af.kp, af.kp, pairs)
        for i in range(10):
          old = prev_af.kp[pairs[i][0]]
          new = af.kp[pairs[i][1]]
          print old, new, new[0] - old[0]
      prev_af = af
      print "frame", x, "has", len(af.kp), "keypoints", pose

    if 0:
      scribble = Image.merge("RGB", (lf,rf,Image.new("L", lf.size))).resize((1280,960))
      #scribble = Image.merge("RGB", (Image.fromstring("L", lf.size, af0.lgrad),Image.fromstring("L", lf.size, af0.rgrad),Image.new("L", lf.size))).resize((1280,960))
      draw = ImageDraw.Draw(scribble)
      for (x,y,d) in af0.kp:
        draw.line([ (2*x,2*y), (2*x-2*d,2*y) ], fill = (255,255,255))
      for (x,y,d) in af1.kp:
        draw.line([ (2*x,2*y+1), (2*x-2*d,2*y+1) ], fill = (0,0,255))
      #scribble.save('out.png')

  def test_pe(self):
    random.seed(0)
    cloud = [ (4 * (random.random() - 0.5), 4 * (random.random() - 0.5), 4 * (random.random() - 0.5)) for i in range(20) ]
    vo = VisualOdometer(camera.Camera((389.0, 389.0, 89.23 * 1e-3, 323.42, 323.42, 274.95)))
    stereo_cam = {}
    af = {}
    fd = FeatureDetectorStar(300)
    ds = DescriptorSchemeSAD()
    for i in range(5):
      stereo_cam[i] = camera.Camera((389.0, 389.0, .080 + .010 * i, 323.42, 323.42, 274.95))
      desired_pose = Pose()
      cam = ray_camera(desired_pose, stereo_cam[i])
      imL = Image.new("RGB", (640,480))
      imR = Image.new("RGB", (640,480))
      scene = []
      scene += [object(isphere(vec3(0,0,0), 1000), shadeLitCloud, {'scale':0.001})]
      scene += [object(isphere(vec3(x,y,z+6), 0.3), shadeLitCloud, {'scale':3}) for (x,y,z) in cloud]
      imL,imR = [ im.convert("L") for im in [imL,imR] ]
      render_stereo_scene(imL, imR, None, cam, scene, 0)
      af[i] = SparseStereoFrame(imL, imR, feature_detector = fd, descriptor_scheme = ds)
      vo.process_frame(af[i])
    pe = PoseEstimator()
    for i1 in range(5):
      for i0 in range(5):
        pairs = vo.temporal_match(af[i1], af[i0])
        res = pe.estimateC(stereo_cam[i0], af[i0].features(), stereo_cam[i1], af[i1].features(), pairs, False)
        x,y,z = res[2]
        self.assertAlmostEqual(x,0,0)
        self.assertAlmostEqual(y,0,0)
        self.assertAlmostEqual(z,0,0)

if __name__ == '__main__':
  if 1:
    rostest.unitrun('visual_odometry', 'directed', TestDirected)
  else:
    suite = unittest.TestSuite()
    suite.addTest(TestDirected('test_stereo'))
    unittest.TextTestRunner(verbosity=2).run(suite)
