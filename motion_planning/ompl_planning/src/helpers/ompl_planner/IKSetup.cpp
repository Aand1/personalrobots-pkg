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

#include "ompl_planning/planners/IKSetup.h"
#include <ros/console.h>

ompl_planning::IKSetup::IKSetup(void)
{
    ompl_model = NULL;
    gaik = NULL;
}

ompl_planning::IKSetup::~IKSetup(void)
{
    if (gaik)
	delete gaik;
    if (ompl_model)
	delete ompl_model;
}

void ompl_planning::IKSetup::setup(planning_environment::PlanningMonitor *planningMonitor, const std::string &groupName,
				   boost::shared_ptr<planning_environment::RobotModels::PlannerConfig> &options)
{
    ROS_DEBUG("Adding IK instance for '%s'", groupName.c_str());
    ompl_model = new ompl_ros::ModelKinematic(planningMonitor, groupName);
    ompl_model->configure();
    
    gaik = new ompl::kinematic::GAIK(dynamic_cast<ompl::kinematic::SpaceInformationKinematic*>(ompl_model->si));
    
    if (options->hasParam("max_improve_steps"))
    {
	gaik->setMaxImproveSteps(options->getParamInt("max_improve_steps", gaik->getMaxImproveSteps()));
	ROS_DEBUG("Max improve steps is set to %u", gaik->getMaxImproveSteps());
    }
    
    if (options->hasParam("range"))
    {
	gaik->setRange(options->getParamDouble("range", gaik->getRange()));
	ROS_DEBUG("Range is set to %g", gaik->getRange());
    }

    if (options->hasParam("pool_size"))
    {
	gaik->setPoolSize(options->getParamInt("pool_size", gaik->getPoolSize()));
	ROS_DEBUG("Pool size is set to %u", gaik->getPoolSize());
    }

    if (options->hasParam("pool_expansion_size"))
    {
	gaik->setPoolExpansionSize(options->getParamInt("pool_expansion_size", gaik->getPoolExpansionSize()));
	ROS_DEBUG("Pool expansion size is set to %u", gaik->getPoolExpansionSize());
    }
    
    if (options->hasParam("max_improve_steps"))
    {
	gaik->setMaxImproveSteps(options->getParamInt("max_improve_steps", gaik->getMaxImproveSteps()));
	ROS_DEBUG("Max improve steps is set to %u", gaik->getMaxImproveSteps());
    }
    
    ompl_model->si->setup();
}
