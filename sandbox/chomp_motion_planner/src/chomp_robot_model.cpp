/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the Willow Garage nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/** \author Mrinal Kalakrishnan */

#include <chomp_motion_planner/chomp_robot_model.h>
#include <kdl_parser/xml_parser.hpp>
#include <ros/ros.h>
#include <cstdio>

using namespace std;
using namespace mapping_msgs;

namespace chomp
{

ChompRobotModel::ChompRobotModel():
  robot_models_(NULL)
{
}

ChompRobotModel::~ChompRobotModel()
{
  if (robot_models_)
    delete robot_models_;
}

bool ChompRobotModel::init()
{
  node_handle_.param("~reference_frame", reference_frame_, std::string("base_link"));

  // listen to attached objects
  attached_object_subscriber_ = node_handle_.subscribe(std::string("attach_object"), 1, &ChompRobotModel::attachedObjectCallback, this);

  max_radius_clearance_ = 0.0;

  // create the robot model
  robot_models_ = new planning_environment::RobotModels("robot_description");

  if (!robot_models_->loadedModels())
  {
    ROS_ERROR("Could not load robot models.");
    return false;
  }

  // get the urdf as a string:
  string urdf_string;
  if (!node_handle_.getParam(robot_models_->getDescription(), urdf_string))
  {
    return false;
  }

  // get some other params:
  double joint_update_limit;
  node_handle_.param("~collision_clearance", collision_clearance_default_, 0.10);
  node_handle_.param("~joint_update_limit", joint_update_limit, 0.05);

  // Construct the KDL tree
  if (!treeFromString(urdf_string, kdl_tree_))
  {
    ROS_ERROR("Failed to construct KDL tree from URDF.");
    return false;
  }
  num_kdl_joints_ = kdl_tree_.getNrOfJoints();

  // create the joint_segment_mapping, which used to be created by the URDF -> KDL parser
  // but not any more, but the rest of the code depends on it, so we simply generate the mapping here:
  KDL::SegmentMap segment_map = kdl_tree_.getSegments();

  for (KDL::SegmentMap::const_iterator it = segment_map.begin(); it != segment_map.end(); ++it)
  {
    if (it->second.segment.getJoint().getType() != KDL::Joint::None)
    {
      std::string joint_name = it->second.segment.getJoint().getName();
      std::string segment_name = it->first;
      joint_segment_mapping_.insert(make_pair(joint_name, segment_name));
    }
  }

  // create the fk solver:
  fk_solver_ = new KDL::TreeFkSolverJointPosAxis(kdl_tree_, reference_frame_);

  kdl_number_to_urdf_name_.resize(num_kdl_joints_);
  // Create the inverse mapping - KDL segment to joint name
  // (at the same time) Create a mapping from KDL numbers to URDF joint names and vice versa
  for (map<string, string>::iterator it = joint_segment_mapping_.begin(); it!= joint_segment_mapping_.end(); ++it)
  {
    std::string joint_name = it->first;
    std::string segment_name = it->second;
    //std::cout << joint_name << " -> " << segment_name << std::endl;
    segment_joint_mapping_.insert(make_pair(segment_name, joint_name));
    int kdl_number = kdl_tree_.getSegment(segment_name)->second.q_nr;
    if (kdl_tree_.getSegment(segment_name)->second.segment.getJoint().getType() != KDL::Joint::None)
    {
      //std::cout << "Kdl number is " << kdl_number << std::endl;
      kdl_number_to_urdf_name_[kdl_number] = joint_name;
      urdf_name_to_kdl_number_.insert(make_pair(joint_name, kdl_number));
    }
  }

  // initialize the planning groups
  std::map<std::string, std::vector<std::string> > groups = robot_models_->getPlanningGroups();
  for (std::map< std::string, std::vector<std::string> >::iterator it = groups.begin(); it != groups.end() ; ++it)
  {
    ChompPlanningGroup group;
    group.name_ = it->first;
    int num_links = it->second.size();
    group.num_joints_ = 0;
    group.link_names_.resize(num_links);
    std::vector<bool> active_joints;
    active_joints.resize(num_kdl_joints_, false);
    for (int i=0; i<num_links; i++)
    {
      std::string link_name = it->second[i];
      group.link_names_[i] = link_name;
      const KDL::Segment* segment = &(kdl_tree_.getSegment(link_name)->second.segment);
      KDL::Joint::JointType joint_type =  segment->getJoint().getType();
      if (joint_type != KDL::Joint::None)
      {
        ChompJoint joint;
        joint.chomp_joint_index_ = group.num_joints_;
        joint.kdl_joint_index_ = kdl_tree_.getSegment(link_name)->second.q_nr;
        joint.kdl_joint_ = &(segment->getJoint());
        joint.link_name_ = link_name;
        joint.joint_name_ = segment_joint_mapping_[link_name];
        joint.joint_update_limit_ = joint_update_limit;
        planning_models::KinematicModel::Joint* kin_model_joint = robot_models_->getKinematicModel()->getJoint(joint.joint_name_);
        if (planning_models::KinematicModel::RevoluteJoint* revolute_joint = dynamic_cast<planning_models::KinematicModel::RevoluteJoint*>(kin_model_joint))
        {
          joint.wrap_around_ = revolute_joint->continuous;
          joint.has_joint_limits_ = !(joint.wrap_around_);
          joint.joint_limit_min_ = revolute_joint->lowLimit;
          joint.joint_limit_max_ = revolute_joint->hiLimit;
        }
        else if (planning_models::KinematicModel::PrismaticJoint* prismatic_joint = dynamic_cast<planning_models::KinematicModel::PrismaticJoint*>(kin_model_joint))
        {
          joint.wrap_around_ = false;
          joint.has_joint_limits_ = true;
          joint.joint_limit_min_ = prismatic_joint->lowLimit;
          joint.joint_limit_max_ = prismatic_joint->hiLimit;
        }
        else
        {
          ROS_WARN("CHOMP cannot handle floating or planar joints yet.");
        }

        group.num_joints_++;
        group.chomp_joints_.push_back(joint);
        active_joints[joint.kdl_joint_index_] = true;
      }

    }
    group.fk_solver_.reset(new KDL::TreeFkSolverJointPosAxisPartial(kdl_tree_, reference_frame_, active_joints));
    planning_groups_.insert(make_pair(it->first, group));
  }

  generateCollisionPoints();


  // test it:
/*  KDL::JntArray q_in(kdl_tree_.getNrOfJoints());
  std::vector<KDL::Frame> segment_frames;
  std::vector<KDL::Vector> joint_axis;
  std::vector<KDL::Vector> joint_pos;

  ros::WallTime start_time = ros::WallTime::now();

  boost::shared_ptr<KDL::TreeFkSolverJointPosAxisPartial> fks = planning_groups_["right_arm"].fk_solver_;
  double q=0.0;
  int n = kdl_tree_.getNrOfJoints();
  for (int i=0; i<100000; i++)
  {
    for (int j=0; j<n; j++)
    {
      q_in(j) += q;
      q+=0.1;
    }
    if (i==0)
      fks->JntToCartFull(q_in, joint_pos, joint_axis, segment_frames);
    else
      fks->JntToCartPartial(q_in, joint_pos, joint_axis, segment_frames);
  }
  ROS_INFO("100000 FK calls in %f wall-seconds.", (ros::WallTime::now() - start_time).toSec());
*/

  ROS_INFO("Initialized chomp robot model in %s reference frame.", reference_frame_.c_str());

  return true;
}

void ChompRobotModel::getLinkInformation(const std::string link_name, std::vector<int>& active_joints, int& segment_number)
{
  // check if the link already exists in the map, if not, add it:
  if (link_collision_points_.find(link_name) == link_collision_points_.end())
  {
    link_collision_points_.insert(make_pair(link_name, std::vector<ChompCollisionPoint>()));
  }

  // identify the joints that contribute to this link
  active_joints.clear();
  KDL::SegmentMap::const_iterator segment_iter = kdl_tree_.getSegment(link_name);

  // go up the tree until we find the root:
  while (segment_iter != kdl_tree_.getRootSegment())
  {
    KDL::Joint::JointType joint_type =  segment_iter->second.segment.getJoint().getType();
    if (joint_type != KDL::Joint::None)
    {
      int joint_num = segment_iter->second.q_nr;
      if ((int)active_joints.size() < (joint_num+1))
        active_joints.resize(joint_num+1, 0);
      active_joints[joint_num] = 1;
    }
    segment_iter = segment_iter->second.parent;
  }

  segment_number = fk_solver_->segmentNameToIndex(link_name);

}

void ChompRobotModel::addCollisionPointsFromLinkRadius(std::string link_name, double radius, double clearance, double extension)
{
  std::vector<int> active_joints(num_kdl_joints_, 0);
  KDL::SegmentMap::const_iterator segment_iter = kdl_tree_.getSegment(link_name);
  int segment_number;

  getLinkInformation(link_name, active_joints, segment_number);
  std::vector<ChompCollisionPoint>& collision_points_vector = link_collision_points_.find(link_name)->second;

  int first_child=1;
  // find the child:
  for (std::vector<KDL::SegmentMap::const_iterator>::const_iterator child_iter = segment_iter->second.children.begin();
      child_iter!= segment_iter->second.children.end(); ++child_iter)
  {
    //cout << (*child_iter)->first << " is a child of " << link_name << endl;

    KDL::Vector joint_origin = (*child_iter)->second.segment.getJoint().JointOrigin();
    //printf("joint origin for %s is %f %f %f\n", (*child_iter)->first.c_str(), joint_origin.x(), joint_origin.y(), joint_origin.z());

    // generate equidistant collision points for this link:
    double spacing = radius/2.0;
    double distance = joint_origin.Norm();
    distance+=extension;
    int num_points = ceil(distance/spacing)+1;
    spacing = distance/(num_points-1.0);

    KDL::Vector point_pos;
    for (int i=0; i<num_points; i++)
    {
      if (!first_child && i==0)
        continue;
      point_pos = joint_origin * (double)(i/(num_points-1.0));
      collision_points_vector.push_back(ChompCollisionPoint(active_joints, radius, clearance, segment_number, point_pos));
    }

    first_child = 0;
  }

}

void ChompRobotModel::ChompPlanningGroup::addCollisionPoint(ChompCollisionPoint& collision_point, ChompRobotModel& robot_model)
{
  // create the new parent joints indexing vector:
  std::vector<int> parent_joints(num_joints_, 0);

  // check if this collision point is controlled by any joints which belong to the group
  bool add_this_point=false;
  for (int i=0; i<num_joints_; i++)
  {
    if (collision_point.isParentJoint(chomp_joints_[i].kdl_joint_index_))
    {
      add_this_point = true;
      parent_joints[i] = 1;
    }
  }

  if (!add_this_point)
    return;

  collision_points_.push_back(ChompCollisionPoint(collision_point, parent_joints));

}

void ChompRobotModel::getLinkCollisionPoints(std::string link_name, std::vector<ChompCollisionPoint>& points)
{
  std::map<std::string, std::vector<ChompCollisionPoint> >::iterator it = link_collision_points_.find(link_name);
  if (it==link_collision_points_.end())
    return;

  points = it->second;
}

void ChompRobotModel::attachedObjectCallback(const mapping_msgs::AttachedObjectConstPtr& attached_object)
{
  attached_objects_[attached_object->link_name] =  *attached_object;
  generateCollisionPoints();
}

void ChompRobotModel::generateCollisionPoints()
{
  // clear out link collision points:
  link_collision_points_.clear();

  // iterate over all collision checking links to add collision points
  for (vector<string>::const_iterator link_it=robot_models_->getCollisionCheckLinks().begin();
      link_it!=robot_models_->getCollisionCheckLinks().end(); ++link_it)
  {
    // get the "radius" of this link from the param server, if any:
    double link_radius;
    std::string link_name = *link_it;
    std::string link_param_root = "~collision_links/"+link_name+"/";
    if (node_handle_.getParam(link_param_root+"link_radius", link_radius))
    {
      double clearance;
      double extension;
      node_handle_.param(link_param_root+"link_clearance", clearance, collision_clearance_default_);
      node_handle_.param(link_param_root+"link_extension", extension, 0.0);
      addCollisionPointsFromLinkRadius(link_name, link_radius, clearance, extension);
      double new_max_rc = link_radius + clearance;
      if (max_radius_clearance_ < new_max_rc)
        max_radius_clearance_ = new_max_rc;
    }

    // now, if there are attached objects on this link, add them too:
    if (attached_objects_.find(link_name)!=attached_objects_.end())
    {
      AttachedObject& ao = (attached_objects_.find(link_name))->second;
      addCollisionPointsFromAttachedObject(link_name, ao);
    }
  }

  // put all collision points into all groups:
  for (std::map<std::string, ChompPlanningGroup>::iterator group_it=planning_groups_.begin(); group_it!=planning_groups_.end(); ++group_it)
  {
    // clear out collision points for this group:
    group_it->second.collision_points_.clear();
    ROS_INFO("Group %s has collision points from:", group_it->second.name_.c_str());

    for (vector<string>::const_iterator link_name_it=robot_models_->getCollisionCheckLinks().begin();
        link_name_it!=robot_models_->getCollisionCheckLinks().end(); ++link_name_it)
    {
      std::map<std::string, std::vector<ChompCollisionPoint> >::iterator link_it = link_collision_points_.find(*link_name_it);
      if (link_it != link_collision_points_.end())
      {
        ROS_INFO("\t%s", link_it->first.c_str());
        for (std::vector<ChompCollisionPoint>::iterator point_it=link_it->second.begin(); point_it!=link_it->second.end(); ++point_it)
        {
          group_it->second.addCollisionPoint(*point_it, *this);
        }
      }

    }
    ROS_INFO("Group %s has %d collision points", group_it->second.name_.c_str(), group_it->second.collision_points_.size());
  }

}

void ChompRobotModel::addCollisionPointsFromAttachedObject(std::string link_name, mapping_msgs::AttachedObject& attached_object)
{
  std::vector<int> active_joints(num_kdl_joints_, 0);
  int segment_number;

  getLinkInformation(link_name, active_joints, segment_number);
  std::vector<ChompCollisionPoint>& collision_points_vector = link_collision_points_.find(link_name)->second;

  for (size_t i=0; i<attached_object.objects.size(); ++i)
  {
    if (attached_object.poses.size()<=i)
      break;
    Object& object = attached_object.objects[i];
    geometry_msgs::Pose& pose = attached_object.poses[i];
    if (object.type == Object::CYLINDER)
    {
      if (object.dimensions.size()<2)
        continue;
      KDL::Rotation rotation = KDL::Rotation::Quaternion(pose.orientation.x,
          pose.orientation.y,
          pose.orientation.z,
          pose.orientation.w);
      KDL::Vector position(pose.position.x, pose.position.y, pose.position.z);
      KDL::Frame f(rotation, position);
      // generate points:
      double radius = object.dimensions[0];
      double length = object.dimensions[1];
      KDL::Vector p(0,0,0);
      KDL::Vector p2;
      double spacing = radius/2.0;
      int num_points = ceil(length/spacing)+1;
      spacing = length/(num_points-1.0);
      for (int i=0; i<num_points; ++i)
      {
        p(2) = -length/2.0 + i*spacing;
        p2 = f*p;
        collision_points_vector.push_back(ChompCollisionPoint(active_joints, radius, collision_clearance_default_, segment_number, p2));
      }
    }
    else
    {
      ROS_WARN("Attaching objects of non-cylinder types is not supported yet!");
    }
  }

}

} // namespace chomp
