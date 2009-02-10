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

/**
 * @mainpage
 *
 * @htmlinclude manifest.html
 *
 * @b move_base is...
 *
 * <hr>
 *
 *  @section usage Usage
 *  @verbatim
 *  $ move_base
 *  @endverbatim
 *
 * <hr>
 *
 * @section topic ROS topics
 *
 * Subscribes to (name/type):
 * - @b 
 *
 * Publishes to (name / type):
 * - @b 
 *
 *  <hr>
 *
 * @section parameters ROS parameters
 *
 * - None
 **/

#include <fstream>
#include <highlevel_controllers/move_base.hh>
#include <topological_map/roadmap_bottleneck_graph.h>

using topological_map::GridCell;

namespace ros {
namespace highlevel_controllers {


/**
 * @brief Specialization for topological map planner
 */
class MoveBaseTopological: public MoveBase {
public:
  MoveBaseTopological(char *input_file_name);

private:

  /**
   * @brief Builds a plan from current state to goal state
   */
  virtual bool makePlan();

  topological_map::RoadmapBottleneckGraph graph_;
};


    
MoveBaseTopological::MoveBaseTopological(char *input_file_name) :
  MoveBase(), graph_()
{
  graph_.readFromFile (input_file_name);
  graph_.initializeRoadmap();
  initialize();
  ROS_DEBUG ("Finished initializing MoveBaseTopological roadmap");

  // Debug: output the roadmap graph
  ofstream stream;
  stream.open ("roadmaps.ppm");
  graph_.outputPpm(stream, 3);
  // ofstream destructor will close the stream
}



bool MoveBaseTopological::makePlan(){
   
  try {
	
    // Lock the state message and obtain current position and goal
    GridCell start, goal;

    // TODO get this constant from somewhere
    const float resolution=.05; 

    lock();
    graph_.setCostmap(getCostMap().getMap());
    unlock();
    
    stateMsg.lock();
    start.first=(int)floor(stateMsg.pos.y/resolution);
    start.second=(int)floor(stateMsg.pos.x/resolution);

    goal.second=(int)floor(stateMsg.goal.x/resolution);
    goal.first=(int)floor(stateMsg.goal.y/resolution);
    stateMsg.unlock();


    // Invoke the planner
    vector<GridCell> solution=graph_.findOptimalPath(start, goal);

    std::list<deprecated_msgs::Pose2DFloat32> newPlan;
    ROS_DEBUG ("Topological planner found plan with waypoints: ");
    for (unsigned int i=0; i<solution.size(); i++) {
      double wx=solution[i].second*resolution;
      double wy=(solution[i].first)*resolution;
      deprecated_msgs::Pose2DFloat32 step;
      step.x = wx;
      step.y = wy;
      newPlan.push_back(step);
      ROS_DEBUG_STREAM (solution[i] << " ");
    }

    updatePlan(newPlan);
    
    return true;
  }
  catch (std::runtime_error const & ee) {
    ROS_ERROR("runtime_error in makePlan(): %s\n", ee.what());
  }

  return false;
}

} // namespace highlevel_controllers

} // namespace ros


int main(int argc, char** argv)
{
  ros::init(argc,argv); 

  assert(argc>1);

  ros::highlevel_controllers::MoveBaseTopological node (argv[1]);

  try {
    node.run();
  }
  catch(char const* e){
    std::cout << e << std::endl;
  }

  

  return(0);
}


