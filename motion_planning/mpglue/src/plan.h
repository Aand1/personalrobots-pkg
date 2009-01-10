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

#ifndef MPGLUE_PLAN_HPP
#define MPGLUE_PLAN_HPP

#include <std_msgs/Pose2DFloat32.h>
#include <vector>

namespace mpglue {
  
  typedef std::vector<int> raw_sbpl_plan_t;
  typedef std::vector<std_msgs::Pose2DFloat32> waypoint_plan_t;
  
  class Environment;
  class IndexTransform;
  
  
  /**
     Helper class for constructing waypoint_plan_t while updating plan
     statistics.
  */
  class PlanConverter
  {
  public:
    explicit PlanConverter(waypoint_plan_t * plan);
    void addWaypoint(std_msgs::Pose2DFloat32 const & waypoint);
    void addWaypoint(double px, double py, double pth);
    
    double plan_length, tangent_change, direction_change;
    
  private:
    waypoint_plan_t * plan_;
    size_t count_;
    double prevx_, prevy_, prevtan_, prevdir_;
  };
  
  
  /**
     Convert a plan from a raw state ID sequence (as computed by
     SBPLPlanner subclasses) to waypoints that are
     understandable. Optionally provides some statistics on the plan.
  */
  void convertPlan(/** in: how to translate state IDs to std_msgs::Pose2DFloat32 */
		   Environment const & environment,
		   /** in: the raw plan */
		   raw_sbpl_plan_t const & raw,
		   /** out: the converted plan (it is just appended
		       to, not cleared for you) */
		   waypoint_plan_t * plan,
		   /** optional out: the cumulated path length */
		   double * optPlanLengthM,
		   /** optional out: the cumulated change in the path
		       tangential direction (the angle between the
		       x-axis and the delta between two successive
		       waypoints) */
		   double * optTangentChangeRad,
		   /** optional out: the cumulated change in the
		       direction of the waypoints (the delta of
		       std_msgs::Pose2DFloat32::th values).
		       \note This only makes sense for plans that are
		       aware of the robot's heading though. */
		   double * optDirectionChangeRad);
  
  /**
     Convert a plan from an float index-pair sequence (as computed by
     NavFn) to waypoints that are understandable. Optionally provides
     some statistics on the plan.
  */
  void convertPlan(/** in: how to translate state IDs to std_msgs::Pose2DFloat32 */
		   IndexTransform const & itransform,
		   /** in: array of X-coordinates (continuous grid index). */
		   float const * path_x,
		   /** in: array of Y-coordinates (continuous grid index). */
		   float const * path_y,
		   /** in: the length of path_x[] and path_y[]. */
		   int path_len,
		   /** out: the converted plan (it is just appended
		       to, not cleared for you) */
		   waypoint_plan_t * plan,
		   /** optional out: the cumulated path length */
		   double * optPlanLengthM,
		   /** optional out: the cumulated change in the path
		       tangential direction (the angle between the
		       x-axis and the delta between two successive
		       waypoints) */
		   double * optTangentChangeRad,
		   /** optional out: the cumulated change in the
		       direction of the waypoints (the delta of
		       std_msgs::Pose2DFloat32::th values).
		       \note This only makes sense for plans that are
		       aware of the robot's heading though. */
		   double * optDirectionChangeRad);
  
}

#endif // MPGLUE_PLAN_HPP
