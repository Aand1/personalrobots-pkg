/*********************************************************************
*
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
*   * Neither the name of Willow Garage, Inc. nor the names of its
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
*
* Author: Eitan Marder-Eppstein
*********************************************************************/
#ifndef NAV_ROBOT_ACTIONS_BASE_GLOBAL_PLANNER_
#define NAV_ROBOT_ACTIONS_BASE_GLOBAL_PLANNER_

#include <robot_msgs/PoseStamped.h>
#include <costmap_2d/costmap_2d_ros.h>
#include <loki/Factory.h>
#include <loki/Sequence.h>

namespace nav_robot_actions {
  class BaseGlobalPlanner{
    public:
      virtual bool makePlan(const robot_msgs::PoseStamped& start, 
          const robot_msgs::PoseStamped& goal, std::vector<robot_msgs::PoseStamped>& plan) = 0;

    protected:
      BaseGlobalPlanner(){}
  };

  //create an associated factory
  typedef Loki::SingletonHolder
  <
    Loki::Factory< BaseGlobalPlanner, std::string, 
      Loki::Seq< std::string, 
      costmap_2d::Costmap2DROS&  > >,
    Loki::CreateUsingNew,
    Loki::LongevityLifetime::DieAsSmallObjectParent
  > BGPFactory;

#define ROS_REGISTER_BGP(c) \
  nav_robot_actions::BaseGlobalPlanner* ROS_New_##c(std::string name, \
      costmap_2d::Costmap2DROS& costmap_ros){ \
    return new c(name, costmap_ros); \
  }  \
  class RosBGP##c { \
    public: \
    RosBGP##c() \
    { \
      nav_robot_actions::BGPFactory::Instance().Register(#c, ROS_New_##c); \
    } \
    ~RosBGP##c() \
    { \
      nav_robot_actions::BGPFactory::Instance().Unregister(#c); \
    } \
  }; \
  static RosBGP##c ROS_BGP_##c;
};

#endif