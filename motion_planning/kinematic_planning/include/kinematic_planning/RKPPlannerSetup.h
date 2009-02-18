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

#ifndef KINEMATIC_PLANNING_RKP_PLANNER_SETUP_
#define KINEMATIC_PLANNING_RKP_PLANNER_SETUP_

#include "kinematic_planning/RKPModelBase.h"
#include "kinematic_planning/RKPStateValidator.h"
#include "kinematic_planning/RKPSpaceInformation.h"
#include "kinematic_planning/RKPProjectionEvaluators.h"
#include "kinematic_planning/RKPDistanceEvaluators.h"

#include <ompl/base/Planner.h>
#include <ompl/extension/samplingbased/kinematic/PathSmootherKinematic.h>

#include <ros/console.h>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <cassert>

#include <vector>
#include <string>
#include <map>

namespace kinematic_planning
{
    
    class RKPPlannerSetup
    {
    public:
	
	RKPPlannerSetup(void)
	{
	    mp = NULL;
	    si = NULL;
	    svc = NULL;
	    smoother = NULL;
	}
	
	virtual ~RKPPlannerSetup(void)
	{
	    if (mp)
		delete mp;
	    if (svc)
		delete svc;
	    for (std::map<std::string, ompl::SpaceInformation::StateDistanceEvaluator_t>::iterator j = sde.begin(); j != sde.end() ; ++j)
		if (j->second)
		    delete j->second;
	    if (smoother)
		delete smoother;
	    if (si)
		delete si;
	}
	
	/** For each planner definition, define the set of distance metrics it can use */
	virtual void setupDistanceEvaluators(void)
	{
	    assert(si);
	    sde["L2Square"] = new ompl::SpaceInformationKinematic::StateKinematicL2SquareDistanceEvaluator(si);
	}
	
	virtual ompl::ProjectionEvaluator_t getProjectionEvaluator(RKPModelBase *model, const std::map<std::string, std::string> &options) const
	{
	    std::map<std::string, std::string>::const_iterator pit = options.find("projection");
	    std::map<std::string, std::string>::const_iterator cit = options.find("celldim");
	    ompl::ProjectionEvaluator_t pe = NULL;
	    
	    if (pit != options.end() && cit != options.end())
	    {
		std::string proj = pit->second;
		boost::trim(proj);
		std::string celldim = cit->second;
		boost::trim(celldim);
		
		if (proj.substr(0, 4) == "link")
		{
		    std::string linkName = proj.substr(4);
		    boost::trim(linkName);
		    pe = new LinkPositionProjectionEvaluator(model, linkName);
		}
		else
		{
		    std::vector<unsigned int> projection;
		    std::stringstream ss(proj);
		    while (ss.good())
		    {
			unsigned int comp;
			ss >> comp;
			projection.push_back(comp);
		    }
		    pe = new ompl::OrthogonalProjectionEvaluator(projection);
		}
		
		std::vector<double> cdim;
		std::stringstream ss(celldim);
		while (ss.good())
		{
		    double comp;
		    ss >> comp;
		    cdim.push_back(comp);
		}
		
		pe->setCellDimensions(cdim);
		ROS_INFO("Projection is set to %s", proj.c_str());
		ROS_INFO("Cell dimensions set to %s", celldim.c_str());
	    }
	    
	    return pe;
	}

	virtual void preSetup(RKPModelBase *model, std::map<std::string, std::string> &options)
	{
	    ROS_INFO("Adding %s instance for motion planning: %s", name.c_str(), model->groupName.c_str());
	    
	    si       = new SpaceInformationRKPModel(model);
	    svc      = new StateValidityPredicate(model);
	    si->setStateValidityChecker(svc);
	    
	    smoother = new ompl::PathSmootherKinematic(si);
	    smoother->setMaxSteps(50);
	    smoother->setMaxEmptySteps(4);
	    
	}
	
	virtual void postSetup(RKPModelBase *model, std::map<std::string, std::string> &options)
	{
	    setupDistanceEvaluators();
	    si->setup();
	    mp->setup();	    
	}
	
	virtual bool setup(RKPModelBase *model, std::map<std::string, std::string> &options) = 0;
	
	std::string                                                             name;	
	ompl::Planner_t                                                         mp;
	ompl::SpaceInformationKinematic_t                                       si;
	ompl::SpaceInformation::StateValidityChecker_t                          svc;
	std::map<std::string, ompl::SpaceInformation::StateDistanceEvaluator_t> sde;
	ompl::PathSmootherKinematic_t                                           smoother;

    protected:
	
	double parseDouble(const std::string &value, double def)
	{
	    try
	    {
		return boost::lexical_cast<double>(value);
	    }
	    catch (...)
	    {
		return def;
	    }
	}
	
    };

    
} // kinematic_planning

#endif
    
