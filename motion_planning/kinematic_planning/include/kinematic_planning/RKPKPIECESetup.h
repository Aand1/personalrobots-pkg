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

#ifndef KINEMATIC_PLANNING_RKP_KPIECE_SETUP_
#define KINEMATIC_PLANNING_RKP_KPIECE_SETUP_

#include "kinematic_planning/RKPPlannerSetup.h"
#include <ompl/extension/samplingbased/kinematic/extension/kpiece/LBKPIECE1.h>

namespace kinematic_planning
{
    
    class RKPKPIECESetup : public RKPPlannerSetup
    {
    public:
	
        RKPKPIECESetup(void) : RKPPlannerSetup()
	{
	    name = "KPIECE";	    
	}
	
	virtual ~RKPKPIECESetup(void)
	{
	    if (dynamic_cast<ompl::LBKPIECE1_t>(mp))
	    {
		ompl::ProjectionEvaluator_t pe = dynamic_cast<ompl::LBKPIECE1_t>(mp)->getProjectionEvaluator();
		if (pe)
		    delete pe;
	    }
	}
	
	virtual bool setup(RKPModelBase *model, std::map<std::string, std::string> &options)
	{
	    preSetup(model, options);
	    
	    ompl::LBKPIECE1_t kpiece = new ompl::LBKPIECE1(si);
	    mp                       = kpiece;	
	    
	    if (options.find("range") != options.end())
	    {
		double range = parseDouble(options["range"], kpiece->getRange());
		kpiece->setRange(range);
		ROS_INFO("Range is set to %g", range);
	    }
	    
	    kpiece->setProjectionEvaluator(getProjectionEvaluator(model, options));
	    
	    if (kpiece->getProjectionEvaluator() == NULL)
	    {
		ROS_WARN("Adding %s failed: need to set both 'projection' and 'celldim' for %s", name.c_str(), model->groupName.c_str());
		return false;
	    }
	    else
	    {
		postSetup(model, options);
		return true;
	    }
	}
	
    };

} // kinematic_planning

#endif
    
