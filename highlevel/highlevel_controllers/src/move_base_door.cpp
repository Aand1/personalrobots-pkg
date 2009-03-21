
/*********************************************************************
 * Software License Agreement (BSD License)
 * 
 *  Copyright (c) 2008, Willow Garage, Inc.
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

/**
 * @mainpage
 *
 * @htmlinclude manifest.html
 *
 * @b move_base is...
 *
 * <hr>
 *
 *  @section usage Usage
 *  @verbatim
 *  $ move_base
 *  @endverbatim
 *
 * <hr>
 *
 * @section topic ROS topics
 *
 * Subscribes to (name/type):
 * - @b 
 *
 * Publishes to (name / type):
 * - @b 
 *
 *  <hr>
 *
 * @section parameters ROS parameters
 *
 * - None
 **/

#include <highlevel_controllers/move_base.hh>
#include <navfn.h>

#include <robot_msgs/JointTraj.h>
#include <angles/angles.h>

using namespace costmap_2d;
using namespace deprecated_msgs;

namespace ros 
{
  namespace highlevel_controllers 
  {

    /**
     * @brief Specialization for the SBPL planner
     */
    class MoveBaseDoor: public MoveBase {
   
      public:

        MoveBaseDoor();

        virtual ~MoveBaseDoor();

      private:

        /**
         * @brief Builds a plan from current state to goal state
         */
        virtual bool dispatchCommands();

        virtual bool makePlan();

        virtual bool goalReached();

        virtual void handleDeactivation();

        void stopTrajectoryControl();

        void checkTrajectory(const robot_msgs::JointTraj& trajectory, robot_msgs::JointTraj &return_trajectory, std::string trajectory_frame_id, std::string map_frame_id);

        bool computeOrientedFootprint(double x_i, double y_i, double th_i, const std::vector<deprecated_msgs::Point2DFloat32>& footprint_spec, std::vector<deprecated_msgs::Point2DFloat32>& oriented_footprint);

        void transform2DPose(const double &x, const double &y, const double &th, const std::string original_frame_id, const std::string &transform_frame_id, double &return_x, double &return_y, double &return_theta);

        bool goalWithinTolerance();

        trajectory_rollout::CostmapModel *cost_map_model_;

        double goal_x_, goal_y_, goal_theta_;

        double current_x_, current_y_, current_theta_;

        double dist_waypoints_max_;

        double dist_rot_waypoints_max_;

        robot_msgs::JointTraj path_;

        robot_msgs::JointTraj valid_path_;

        robot_msgs::JointTraj empty_path_;

        std::string base_trajectory_controller_topic_;

        std::string base_trajectory_controller_frame_id_;

        double circumscribed_radius_;

        double inscribed_radius_;

        std::vector<deprecated_msgs::Point2DFloat32> footprint_wo_nose_;
   };
    
    
    MoveBaseDoor::MoveBaseDoor() : MoveBase(), goal_x_(0.0), goal_y_(0.0), goal_theta_(0.0), dist_waypoints_max_(0.025)
    {
      cost_map_model_ = new trajectory_rollout::CostmapModel(*costMap_);
      ros::Node::instance()->param<std::string>("~move_base_door/trajectory_control_topic",base_trajectory_controller_topic_,"base/trajectory_controller/trajectory_command");
      ros::Node::instance()->param<std::string>("~move_base_door/base_trajectory_controller_frame_id",base_trajectory_controller_frame_id_,"odom_combined");

      ros::Node::instance()->param<double>("~move_base_door/dist_rot_waypoints_max", dist_rot_waypoints_max_,0.05);

      ros::Node::instance()->advertise<robot_msgs::JointTraj>(base_trajectory_controller_topic_,1);
      ros::Node::instance()->param("~costmap_2d/circumscribed_radius", circumscribed_radius_, 0.46);
      ros::Node::instance()->param("~costmap_2d/inscribed_radius", inscribed_radius_, 0.325);
      ros::Node::instance()->param("~move_base_door/dist_waypoints_max", dist_waypoints_max_, 0.025);
      ROS_INFO("Initialized move base door");
      footprint_wo_nose_ = footprint_;
      footprint_wo_nose_.pop_back();
      initialize();
   }

    bool MoveBaseDoor::makePlan()
    {
      ROS_INFO("Making the plan");
      return true;
    }

    bool MoveBaseDoor::computeOrientedFootprint(double x_i, double y_i, double theta_i, const std::vector<deprecated_msgs::Point2DFloat32>& footprint_spec, std::vector<deprecated_msgs::Point2DFloat32>& oriented_footprint)
    {
      //if we have no footprint... do nothing
      if(footprint_spec.size() < 3)
      {
        ROS_ERROR("No footprint available");
        return -1.0;
      }
      //build the oriented footprint
      double cos_th = cos(theta_i);
      double sin_th = sin(theta_i);
      for(unsigned int i = 0; i < footprint_spec.size(); ++i){
        Point2DFloat32 new_pt;
        new_pt.x = x_i + (footprint_spec[i].x * cos_th - footprint_spec[i].y * sin_th);
        new_pt.y = y_i + (footprint_spec[i].x * sin_th + footprint_spec[i].y * cos_th);
        oriented_footprint.push_back(new_pt);
      }
      return true;
    }

    bool MoveBaseDoor::goalReached()
    {
      return goalWithinTolerance();
    }


    bool MoveBaseDoor::goalWithinTolerance()
    {
      double uselessPitch, uselessRoll, yaw;
      global_pose_.getBasis().getEulerZYX(yaw, uselessPitch, uselessRoll);

      double dist_trans = sqrt(pow(global_pose_.getOrigin().x()-stateMsg.goal.x,2) + pow(global_pose_.getOrigin().y()-stateMsg.goal.y,2));        
      double dist_rot = fabs(angles::normalize_angle(yaw-stateMsg.goal.th));

      if(dist_rot < yaw_goal_tolerance_ && dist_trans < xy_goal_tolerance_)
      { 
        ros::Node::instance()->publish(base_trajectory_controller_topic_,empty_path_);         
        ROS_DEBUG("Goal achieved at: (%f, %f, %f) for (%f, %f, %f)\n",global_pose_.getOrigin().x(), global_pose_.getOrigin().y(), yaw,stateMsg.goal.x, stateMsg.goal.y, stateMsg.goal.th);
        return true;
      }
      return false;
    }


    void MoveBaseDoor::checkTrajectory(const robot_msgs::JointTraj& trajectory, robot_msgs::JointTraj &return_trajectory, std::string trajectory_frame_id, std::string map_frame_id)
    {
      double theta;
      return_trajectory = trajectory;
      deprecated_msgs::Point2DFloat32 position;

      for(int i=0; i < (int) trajectory.points.size(); i++)
      {
        double x, y;
        std::vector<deprecated_msgs::Point2DFloat32> oriented_footprint;
/*
        position.x  = trajectory.points[i].positions[0];
        position.y  = trajectory.points[i].positions[1];
        theta = trajectory.points[i].positions[2];
*/

        transform2DPose(trajectory.points[i].positions[0], trajectory.points[i].positions[1], trajectory.points[i].positions[2], trajectory_frame_id, map_frame_id, x, y, theta);

        position.x = x;
        position.y = y;

        computeOrientedFootprint(position.x,position.y,theta, footprint_wo_nose_, oriented_footprint);

        if(cost_map_model_->footprintCost(position, oriented_footprint, inscribed_radius_, circumscribed_radius_) >= 0)
        {
          return_trajectory.points[i].positions[2] = trajectory.points[i].positions[2];
          ROS_INFO("Point %d: position: %f, %f, %f is not in collision",i,position.x,position.y,theta);
          continue;
        }
        else
        {
          ROS_INFO("Point %d: position: %f, %f, %f is in collision",i,position.x,position.y,theta);
          ROS_INFO("Radius inscribed: %f, circumscribed: %f",inscribed_radius_, circumscribed_radius_);
          for(int j=0; j < (int) oriented_footprint.size(); j++)
            ROS_INFO("Footprint point: %d is : %f,%f",j,oriented_footprint[j].x,oriented_footprint[j].y);
          int last_valid_point = std::max<int>(i-10,0);
          if(last_valid_point > 0)
          {
            return_trajectory.points.resize(last_valid_point+1);
          }
          else
          {
            return_trajectory.points.resize(0);
          }
          break;
        }       
      }
    };

    MoveBaseDoor::~MoveBaseDoor()
    {
    }

    void MoveBaseDoor::transform2DPose(const double &x, const double &y, const double &th, const std::string original_frame_id, const std::string &transform_frame_id, double &return_x, double &return_y, double &return_theta)
    {
      tf::Stamped<tf::Pose> pose;
      btQuaternion qt;
      qt.setEulerZYX(th, 0, 0);
      pose.setData(btTransform(qt, btVector3(x, y, 0)));
      pose.frame_id_ = original_frame_id;
      pose.stamp_ = ros::Time();

      tf::Stamped<tf::Pose> transformed_pose;
/*      transformed_pose.setIdentity();
      transformed_pose.frame_id_ = base_trajectory_controller_frame_id_;
      transformed_pose.stamp_ = ros::Time();
*/
      try{
        tf_.transformPose(transform_frame_id, pose, transformed_pose);
      }
      catch(tf::LookupException& ex) {
        ROS_ERROR("No Transform available Error: %s\n", ex.what());
      }
      catch(tf::ConnectivityException& ex) {
        ROS_ERROR("Connectivity Error: %s\n", ex.what());
      }
      catch(tf::ExtrapolationException& ex) {
        ROS_ERROR("Extrapolation Error: %s\n", ex.what());
      }

      return_x = transformed_pose.getOrigin().x();
      return_y = transformed_pose.getOrigin().y();
      double uselessPitch, uselessRoll, yaw;
      transformed_pose.getBasis().getEulerZYX(yaw, uselessPitch, uselessRoll);
      return_theta = (double)yaw;      
    };

    void MoveBaseDoor::handleDeactivation(){
      stopTrajectoryControl();
    }

    void MoveBaseDoor::stopTrajectoryControl(){
      ROS_DEBUG("Stopping the robot now!\n");
      ros::Node::instance()->publish(base_trajectory_controller_topic_,empty_path_);         
    }

    bool MoveBaseDoor::dispatchCommands()
    {
      ROS_INFO("Planning for new goal...\n");
      robot_msgs::JointTraj valid_path;
      stateMsg.lock();
      transform2DPose(stateMsg.pos.x,stateMsg.pos.y,stateMsg.pos.th,global_frame_, base_trajectory_controller_frame_id_,current_x_,current_y_,current_theta_);
      transform2DPose(stateMsg.goal.x,stateMsg.goal.y,stateMsg.goal.th,global_frame_, base_trajectory_controller_frame_id_,goal_x_,goal_y_,goal_theta_);
      ROS_INFO("Current position: in frame %s, %f %f %f, goal position: %f %f %f", global_frame_.c_str(),stateMsg.pos.x,stateMsg.pos.y,stateMsg.pos.th,stateMsg.goal.x,stateMsg.goal.y,stateMsg.goal.th);
      stateMsg.unlock();

      ROS_INFO("Current position: in frame %s, %f %f %f, goal position: %f %f %f", base_trajectory_controller_frame_id_.c_str(),current_x_,current_y_,current_theta_,goal_x_,goal_y_,goal_theta_);

      // For now plan is a straight line with waypoints about 2.5 cm apart
      double dist_trans = sqrt( pow(goal_x_-current_x_,2) + pow(goal_y_-current_y_,2));        
      double dist_rot = fabs(angles::normalize_angle(goal_theta_-current_theta_));

      int num_intervals = std::max<int>(1,(int) (dist_trans/dist_waypoints_max_));
      num_intervals = std::max<int> (num_intervals, (int) (dist_rot/dist_rot_waypoints_max_));

      int num_path_points = num_intervals + 1;
      double path_theta = atan2(goal_y_-current_y_,goal_x_-current_x_);

      ROS_INFO("Path orientation is: %f",path_theta);
 
      ROS_INFO("Num of intervals for the path is %d", num_intervals);
      path_.set_points_size(num_path_points);

      for(int i=0; i< num_intervals; i++)
      {
        path_.points[i].set_positions_size(3);
        path_.points[i].positions[0] = current_x_ + (double) i * (goal_x_-current_x_)/num_intervals ;
        path_.points[i].positions[1] = current_y_ + (double) i * (goal_y_-current_y_)/num_intervals ;
        path_.points[i].positions[2] =  angles::normalize_angle(current_theta_ + (double) i * angles::normalize_angle(goal_theta_-current_theta_)/num_intervals);
        path_.points[i].time = 0.0;
      }

      path_.points[num_intervals].set_positions_size(3);
      path_.points[num_intervals].positions[0] = goal_x_;
      path_.points[num_intervals].positions[1] = goal_y_;
      path_.points[num_intervals].positions[2] = goal_theta_;

      checkTrajectory(path_,valid_path_,base_trajectory_controller_frame_id_,global_frame_);
      // First criteria is that we have had a sufficiently recent sensor update to trust perception and that we have a valid plan. This latter
      // case is important since we can end up with an active controller that becomes invalid through the planner looking ahead. 
      // We want to be able to stop the robot in that case
      bool planOk = checkWatchDog();

      if(!planOk)
      {
        ROS_ERROR("MoveBaseDoor: Plan not ok");
        return false;
      }
      ROS_INFO("Publishing trajectory on topic: %s with %d points",base_trajectory_controller_topic_.c_str(),valid_path_.points.size());
      if(!goalWithinTolerance())
        ros::Node::instance()->publish(base_trajectory_controller_topic_,valid_path_);         
      else
        ros::Node::instance()->publish(base_trajectory_controller_topic_,empty_path_);         
      return true;
    }
  }
}


int main(int argc, char** argv)
{
  ros::init(argc,argv); 
  ros::Node rosnode("move_base_door");

  ros::highlevel_controllers::MoveBaseDoor node;

  try 
  {
    node.run();
  }
  catch(char const* e)
  {
    std::cout << e << std::endl;
  }
  return(0);
}
