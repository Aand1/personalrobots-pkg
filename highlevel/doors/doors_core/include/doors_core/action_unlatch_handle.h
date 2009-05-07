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

/* Author: Wim Meeusen */

#ifndef ACTION_UNLATCH_HANDLE_H
#define ACTION_UNLATCH_HANDLE_H

#include <ros/node.h>
#include <door_msgs/Door.h>
#include <robot_msgs/TaskFrameFormalism.h>
#include <tf/tf.h>
#include <tf/transform_listener.h>
#include <robot_srvs/MoveToPose.h>
#include <kdl/frames.hpp>
#include <robot_actions/action.h>
#include <boost/thread/mutex.hpp>


namespace door_handle_detector{


class UnlatchHandleAction: public robot_actions::Action<door_msgs::Door, door_msgs::Door>
{
public:
  UnlatchHandleAction(ros::Node& node);
  ~UnlatchHandleAction();

  virtual robot_actions::ResultStatus execute(const door_msgs::Door& goal, door_msgs::Door& feedback);

private:
  void tffCallback();

  ros::Node& node_;

  robot_msgs::TaskFrameFormalism tff_stop_, tff_handle_, tff_door_;

  robot_msgs::Twist tff_msg_, tff_state_;
  bool tff_state_received_;
  boost::mutex tff_mutex_;

};


}



#endif
