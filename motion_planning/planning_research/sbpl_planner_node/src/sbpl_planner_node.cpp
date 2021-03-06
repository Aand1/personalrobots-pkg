
/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *W
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sbpl_planner_node/sbpl_planner_node.h>

using namespace std;
using namespace ros;

SBPLPlannerNode::SBPLPlannerNode()
  : tf_(*ros::Node::instance(),
	true,			// interpolating
	ros::Duration(10)	// max_cache_time   XXXX tune this! (get as param?)
	),
    cost_map_ros_("costmap", tf_),
    cm_getter_(this)
{
  ros::Node::instance()->param("~allocated_time", allocated_time_, 1.0);
  ros::Node::instance()->param("~forward_search", forward_search_, true);

  ros::Node::instance()->param<std::string>("~cost_map_topic", cost_map_topic_, "cost_map");
  ros::Node::instance()->param("~planner_type", planner_type_, string("ARAPlanner"));
  ros::Node::instance()->param("~use_cost_map", use_cost_map_, true);

  ros::Node::instance()->param("~plan_stats_file", plan_stats_file_, string("/tmp/move_base_sbpl.log"));
  ros::Node::instance()->param("~environment_type", environment_type_, string("2D"));

//  ros::Node::instance()->subscribe(cost_map_topic_, cost_map_msg_, &SBPLPlannerNode::costMapCallback,1);
  ros::Node::instance()->advertiseService("~GetPlan", &SBPLPlannerNode::planPath, this);

  initializePlannerAndEnvironment();
};

SBPLPlannerNode::~SBPLPlannerNode()
{
  ros::Node::instance()->unsubscribe(cost_map_topic_);
  ros::Node::instance()->unadvertiseService("~GetPlan");
};

void PrintUsage(char *argv[])
{
  printf("USAGE: %s <cfg file>\n", argv[0]);
}

bool SBPLPlannerNode::initializePlannerAndEnvironment()
{
  // First set up the environment
  try 
  {
    // We're throwing int exit values if something goes wrong, and
    // clean up any new instances in the catch clause. The sentry
    // gets destructed when we go out of scope, so unlock() gets
    // called no matter what.
    // sentry<SBPLPlannerNode> guard(this);
  
    cm_access_.reset(mpglue::createCostmapAccessor(&cm_getter_));
    cm_index_.reset(mpglue::createIndexTransform(&cm_getter_));
    
    if ("2D" == environment_type_)
    {
      env_.reset(mpglue::SBPLEnvironment::create2D(cm_access_, cm_index_, false));
    }
    else if ("2D16" == environment_type_)
    {
      env_.reset(mpglue::SBPLEnvironment::create2D(cm_access_, cm_index_, true));
    }
    else if ("3DKIN" == environment_type_) 
    {
      string const prefix("env3d/");
      //// ignored by SBPL (at least in r9900).
      // double goaltol_x, goaltol_y, goaltol_theta;
      // ros::Node::instance()->param("~move_base/" + prefix + "goaltol_x", goaltol_x, 0.3);
      // ros::Node::instance()->param("~move_base/" + prefix + "goaltol_y", goaltol_y, 0.3);
      // ros::Node::instance()->param("~move_base/" + prefix + "goaltol_theta", goaltol_theta, 30.0);
      double nominalvel_mpersecs, timetoturn45degsinplace_secs;
      ros::Node::instance()->param("~nominalvel_mpersecs", nominalvel_mpersecs, 0.4);
      ros::Node::instance()->param("~timetoturn45degsinplace_secs", timetoturn45degsinplace_secs, 0.6);
      // Could also sanity check the other parameters...
      env_.reset(mpglue::SBPLEnvironment::create3DKIN(cm_access_, cm_index_, footprint_, nominalvel_mpersecs,timetoturn45degsinplace_secs, 0));
    }
    else if ("XYThetaLattice" == environment_type_) 
    {
      FILE *env_config_fp;
      std::string filename = "sbpl_env_cfg_tmp.txt";
      std::string env_config_string;
      ros::Node::instance()->param<std::string>("~env_config_file", env_config_string, " ");
      env_config_fp = fopen(filename.c_str(),"wt");
      fprintf(env_config_fp,"%s",env_config_string.c_str());
      fclose(env_config_fp);

      double nominalvel_mpersecs, timetoturn45degsinplace_secs;
      ros::Node::instance()->param("~nominalvel_mpersecs", nominalvel_mpersecs, 0.4);
      ros::Node::instance()->param("~timetoturn45degsinplace_secs", timetoturn45degsinplace_secs, 0.6);
      // Could also sanity check the other parameters...
      env_.reset(mpglue::SBPLEnvironment::createXYThetaLattice(cm_access_, cm_index_, footprint_, nominalvel_mpersecs,timetoturn45degsinplace_secs, filename, 0));
    }
    else 
    {
      ROS_ERROR("in MoveBaseSBPL ctor: invalid environmentType \"%s\", use 2D, 2D16, 3DKIN, XYThetaLattice, XYThetaLatticeDoor",environment_type_.c_str());
      throw int(2);
    }
	
    boost::shared_ptr<SBPLPlanner> sbplPlanner;
    if ("ARAPlanner" == planner_type_)
    {
      sbplPlanner.reset(new ARAPlanner(env_->getDSI(), forward_search_));
    }
    else if ("ADPlanner" == planner_type_)
    {
      sbplPlanner.reset(new ADPlanner(env_->getDSI(), forward_search_));
    }
    else 
    {
      ROS_ERROR("in MoveBaseSBPL ctor: invalid planner_type_ \"%s\",use ARAPlanner or ADPlanner",planner_type_.c_str());
      throw int(5);
    }
    pWrap_.reset(new mpglue::SBPLPlannerWrap(env_, sbplPlanner));
  }
  catch (int ii) 
  {
    exit(ii);
  }
  return true;
}

void SBPLPlannerNode::costMapCallBack()
{
  lock_.lock();
//  costMapMsgToCostMap(cost_map_msg_);
  lock_.unlock();
}

bool SBPLPlannerNode::replan(const manipulation_msgs::JointTrajPoint &start, const manipulation_msgs::JointTrajPoint &goal, manipulation_msgs::JointTraj &path)
{
  ROS_INFO("[replan] getting fresh copy of costmap");
  lock_.lock();
  cost_map_ros_.getCostmapCopy(cost_map_);
  lock_.unlock();
  
  // XXXX this is where we would cut out the doors in our local copy
  
  ROS_INFO("[replan] replanning...");
  try {
    // Update costs
    for (mpglue::index_t ix(cm_access_->getXBegin()); ix < cm_access_->getXEnd(); ++ix) {
      for (mpglue::index_t iy(cm_access_->getYBegin()); iy < cm_access_->getYEnd(); ++iy) {
	mpglue::cost_t cost;
	if (cm_access_->getCost(ix, iy, &cost)) { // always succeeds though
	  // Note that ompl::EnvironmentWrapper::UpdateCost() will
	  // check if the cost has actually changed, and do nothing
	  // if it hasn't.  It internally maintains a list of the
	  // cells that have actually changed
	  env_->UpdateCost(ix, iy, cost);
	}
      }
    }
    // NOTE we do not have to call pWrap_->flushCostChanges() here
    // because this node only uses sbpl_planner which always checks
    // directly with its SBPLEnvironment, and that got fed in the
    // preceding loop.
    
    // Assume the robot is constantly moving, so always set start.
    // Maybe a bit inefficient, but not as bad as "changing" the
    // goal when it hasn't actually changed.
    pWrap_->setStart(start.positions[0], start.positions[1], start.positions[2]);	
    pWrap_->setGoal(goal.positions[0], goal.positions[0], goal.positions[2]);
	
    // BTW if desired, we could call pWrap_->forcePlanningFromScratch(true)...
	
    // Invoke the planner, updating the statistics in the process.
    // The returned plan might be empty, but it will not contain a
    // null pointer.  On planner errors, the createPlan() method
    // throws a std::exception.
    boost::shared_ptr<mpglue::waypoint_plan_t> plan(pWrap_->createPlan());
	
    if (plan->empty()) 
    {
      ROS_ERROR("No plan found\n");
      return false;
    }
    
    path.points.clear();	// just paranoid
    mpglue::PlanConverter::convertToJointTraj(plan.get(), &path);
    return true;
  }
  catch (std::runtime_error const & ee) {
    ROS_ERROR("runtime_error in makePlan(): %s\n", ee.what());
  }
      
  return false;
}


bool SBPLPlannerNode::planPath(sbpl_planner_node::PlanPathSrv::Request &req, sbpl_planner_node::PlanPathSrv::Response &resp)
{
  ROS_INFO("[planPath] Planning...");
  manipulation_msgs::JointTraj traj;

  if(replan(req.start,req.joint_goal,traj))
  {
    ROS_INFO("Planning successful");
    resp.traj = traj;
    return true;
  }
  return false;
}


int main(int argc, char *argv[])
{
  ros::init(argc,argv); 
  SBPLPlannerNode node();

/*  try 
  {
    node.spin();
  }
  catch(char const* e)
  {
    std::cout << e << std::endl;
    }*/
  return(0);
}

