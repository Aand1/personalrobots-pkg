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
 *   * Neither the name of Willow Garage nor the names of its
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


/* Author: Patrick Mihelich */

#ifndef ACTION_DETECT_OUTLET_FINE_H
#define ACTION_DETECT_OUTLET_FINE_H

// ROS Stuff
#include <ros/node.h>

// Msgs
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <pr2_robot_actions/DetectOutletState.h>

// Robot Action Stuff
#include <robot_actions/action.h>

// Detection
#include <outlet_detection/outlet_tracker.h>


namespace plugs_core{

class DetectOutletFineAction
  : public robot_actions::Action<geometry_msgs::PointStamped,
                                 geometry_msgs::PoseStamped>
{
public:
  DetectOutletFineAction(ros::Node& node);
  ~DetectOutletFineAction();
  virtual robot_actions::ResultStatus execute(const geometry_msgs::PointStamped& outlet_estimate, geometry_msgs::PoseStamped& feedback);

private:
  ros::Node& node_;
  std::string action_name_;
  std::string head_controller_;
  bool finished_detecting_;

  OutletTracker::OutletTracker* detector_;
  
  
  geometry_msgs::PoseStamped outlet_pose_msg_;
  void foundOutlet();
  
};

}
#endif
