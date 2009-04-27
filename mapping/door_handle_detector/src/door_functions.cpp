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

#include <door_handle_detector/door_functions.h>

using namespace ros;
using namespace std;
using namespace tf;


namespace door_handle_detector{

bool transformTo(const tf::Transformer& tf, const string& goal_frame, const robot_msgs::Door& door_in, robot_msgs::Door& door_out, const std::string& fixed_frame)
{
  door_out = door_in;
  if (!transformPointTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.frame_p1, door_out.frame_p1, fixed_frame)) return false;
  if (!transformPointTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.frame_p2, door_out.frame_p2, fixed_frame)) return false;
  if (!transformPointTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.door_p1, door_out.door_p1, fixed_frame)) return false;
  if (!transformPointTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.door_p2, door_out.door_p2, fixed_frame)) return false;
  if (!transformPointTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.handle, door_out.handle, fixed_frame)) return false;
  if (!transformVectorTo(tf, door_in.header.frame_id, goal_frame, door_in.header.stamp, door_in.normal, door_out.normal, fixed_frame)) return false;
  door_out.header.frame_id = goal_frame;
  return true;
}



bool transformPointTo(const tf::Transformer& tf, const string& source_frame, const string& goal_frame, const Time& time,
                      const robot_msgs::Point32& point_in, robot_msgs::Point32& point_out, const std::string& fixed_frame)
{
  ros::Duration timeout = Duration().fromSec(2.0);
  ros::Time time_now = ros::Time::now();
  if (!tf.canTransform(source_frame, fixed_frame, time, timeout)) return false;
  if (!tf.canTransform(goal_frame, fixed_frame, time_now, timeout)) return false;
  tf::Stamped<tf::Point> pnt(tf::Vector3(point_in.x, point_in.y, point_in.z), time, source_frame);
  tf.transformPoint(goal_frame, time_now, pnt, fixed_frame, pnt);
  point_out.x = pnt[0];
  point_out.y = pnt[1];
  point_out.z = pnt[2];

  return true;
}

bool transformVectorTo(const tf::Transformer& tf, const string& source_frame, const string& goal_frame, const Time& time,
                       const robot_msgs::Vector3& point_in, robot_msgs::Vector3& point_out, const std::string& fixed_frame)
{
  ros::Duration timeout = Duration().fromSec(2.0);
  ros::Time time_now = ros::Time::now();
  if (!tf.canTransform(source_frame, fixed_frame, time, timeout)) return false;
  if (!tf.canTransform(goal_frame, fixed_frame, time_now, timeout)) return false;
  tf::Stamped<tf::Point> pnt(tf::Vector3(point_in.x, point_in.y, point_in.z), time, source_frame);
  tf.transformVector(goal_frame, time_now, pnt, fixed_frame, pnt);
  point_out.x = pnt[0];
  point_out.y = pnt[1];
  point_out.z = pnt[2];

  return true;
}


std::ostream& operator<< (std::ostream& os, const robot_msgs::Door& d)
{
  os << "Door message in " << d.header.frame_id << " at time " << d.header.stamp.toSec() << endl;
  os << " - frame (" 
     << d.frame_p1.x << " " << d.frame_p1.y << " "<< d.frame_p1.z << ") -- (" 
     << d.frame_p2.x << " " << d.frame_p2.y << " "<< d.frame_p2.z << ")" << endl;

  os << " - boundary (" 
     << d.door_p1.x << " " << d.door_p1.y << " "<< d.door_p1.z << ") -- (" 
     << d.door_p2.x << " " << d.door_p2.y << " "<< d.door_p2.z << ")" << endl;

  os << " - handle (" 
     << d.handle.x << " " << d.handle.y << " "<< d.handle.z << ")" << endl;

  os << " - normal (" 
     << d.normal.x << " " << d.normal.y << " "<< d.normal.z << ")" << endl;

  os << " - latch_state " << d.latch_state << endl;
  os << " - hinge side " << d.hinge << endl;
  os << " - rot_dir " << d.rot_dir << endl;
  os << " - angle " << d.angle << endl;

  return os;
}


}
