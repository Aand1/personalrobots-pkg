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

#ifndef CHOMP_PLANNER_NODE_H_
#define CHOMP_PLANNER_NODE_H_

#include <ros/ros.h>
#include <motion_planning_srvs/KinematicPlan.h>
#include <chomp_motion_planner/chomp_robot_model.h>

namespace chomp
{

/**
 * \brief ROS Node which responds to motion planning requests using the CHOMP algorithm.
 */
class ChompPlannerNode
{
public:
  /**
   * \brief Default constructor
   */
  ChompPlannerNode();

  /**
   * \brief Destructor
   */
  virtual ~ChompPlannerNode();

  /**
   * \brief Initialize the node
   *
   * \return true if successful, false if not
   */
  bool init();

  /**
   * \brief Runs the node
   *
   * \return 0 on clean exit
   */
  int run();

  /**
   * \brief Main entry point for motion planning (callback for the plan_kinematic_path service)
   */
  bool planKinematicPath(motion_planning_srvs::KinematicPlan::Request &req, motion_planning_srvs::KinematicPlan::Response &res);

private:
  ros::NodeHandle node_handle_;                         /**< ROS Node handle */
  ros::ServiceServer plan_kinematic_path_service_;      /**< The planning service */

  ChompRobotModel chomp_robot_model_;                   /**< Chomp Robot Model */

};

}

#endif /* CHOMP_PLANNER_NODE_H_ */