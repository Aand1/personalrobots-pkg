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

/** \author Ioan Sucan */

#ifndef OMPL_PLANNING_REQUEST_HANDLER_
#define OMPL_PLANNING_REQUEST_HANDLER_

#include "ompl_planning/Model.h"
#include <ros/ros.h>
#include <motion_planning_srvs/MotionPlan.h>

/** \brief Main namespace */
namespace ompl_planning
{    
   
    /** \brief This class represents a basic request to a motion
	planner. */

    class RequestHandler
    {
    public:
	
	RequestHandler(void)
	{
	    debug_ = false;
	    px_ = py_ = pz_ = -1;
	}
	
	~RequestHandler(void)
	{
	}
	
	/** \brief Enable debug mode. Display markers consisting of a 1-D orthogonal projection of states */
	void enableDebugMode(int idx)
	{
	    enableDebugMode(idx, -1, -1);
	}

	/** \brief Enable debug mode. Display markers consisting of a 2-D orthogonal projection of states */
	void enableDebugMode(int idx1, int idx2)
	{
	    enableDebugMode(idx1, idx2, -1);
	}

	/** \brief Enable debug mode. Display markers consisting of a 3-D orthogonal projection of states */
	void enableDebugMode(int idx1, int idx2, int idx3);
	
	/** \brief Disable debug mode */
	void disableDebugMode(void);
	
	/** \brief Check if the request is valid */
	bool isRequestValid(ModelMap &models, motion_planning_srvs::MotionPlan::Request &req);

	/** \brief Check and compute a motion plan. Return true if the plan was succesfully computed */
	bool computePlan(ModelMap &models, const planning_models::StateParams *start, motion_planning_srvs::MotionPlan::Request &req, motion_planning_srvs::MotionPlan::Response &res);

	/** \brief Print the planner setup settings as debug messages */
	void printSettings(PlannerSetup *psetup);	
	
    private:

	struct Solution
	{
	    ompl::base::Path *path;
	    double            difference;
	    bool              approximate;
	};	
	
	/** \brief Set up all the data needed by motion planning based on a request and lock the planner setup
	 *  using this data */
	void configure(const planning_models::StateParams *startState, motion_planning_srvs::MotionPlan::Request &req, PlannerSetup *psetup);
	
	/** \brief Compute the actual motion plan. Return true if computed plan was trivial (start state already in goal region) */
	bool callPlanner(PlannerSetup *psetup, int times, double allowed_time, Solution &sol);

	/** \brief Fill the response with solution data */
	void fillResult(PlannerSetup *psetup, const planning_models::StateParams *start, motion_planning_srvs::MotionPlan::Response &res, const Solution &sol);

	/** \brief Fix the input states, if they are not valid */
	bool fixInputStates(PlannerSetup *psetup, double value, unsigned int count);
	
	/** \brief Send visualization markers */
	void display(PlannerSetup *psetup);
	
	/** \brief If true, broadcast tree structure as a set of visualization markers */
	bool debug_;
	
	/** \brief The orthogonal projection to take when displaying states */
	int  px_, py_, pz_;
	
	ros::Publisher  displayPublisher_;	
	ros::NodeHandle nh_;
	
    };
    
} // ompl_planning

#endif
