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


#include <ros/ros.h>
#include <planning_environment/collision_models.h>
#include <robot_msgs/MechanismState.h>
   
class SelfWatch
{
public:
    
    SelfWatch(void)
    {
	setupCollisionSpace();
	m_mechanismStateSubscriber = m_nodeHandle.subscribe("mechanism_state", 1, &SelfWatch::mechanismStateCallback, this);
    }
    
    void run(void)
    {
	ros::spin();
    }
    
protected:
    
    void setupCollisionSpace(void)
    {
	// increase the robot parts by x%, to detect collisions before they happen	
	m_nodeHandle.param("self_collision_scale_factor", m_scaling, 1.2);
	m_nodeHandle.param("self_collision_padding", m_padding, 0.05);
	
	if (!m_nodeHandle.hasParam("robot_description"))
	    ROS_ERROR("No robot description found");
	else
	{
	    m_envModels = boost::shared_ptr<planning_environment::CollisionModels>(new planning_environment::CollisionModels("robot_description", m_scaling, m_padding));
	    m_kmodel = m_envModels->getKinematicModel();
	    m_collisionSpace = m_envModels->getODECollisionModel();
	    
	    // create a state that can be used to monitor the
	    // changes in the joints of the kinematic model
	    m_robotState = boost::shared_ptr<planning_models::KinematicModel::StateParams>(m_kmodel->newStateParams());
	    
	    // make sure the transforms caused by the planar and
	    // floating joints are identity, to be compatible with
	    // the fact we are considering the robot in its own
	    // frame
	    m_robotState->setInRobotFrame();
	    
	    // get the list of links that are enabled for collision checking
	    std::vector<std::string> links = m_envModels->getCollisionCheckLinks();
	    
	    // print some info, just to easily double-check the loaded data
	    if (links.empty())
		ROS_WARN("No links have been enabled for collision checking");
	    else
	    {
		ROS_INFO("Collision checking enabled for links: ");
		for (unsigned int i = 0 ; i < links.size() ; ++i)
		    ROS_INFO("  %s", links[i].c_str());
	    }
	    
	    std::vector< std::vector<std::string> > groups = m_envModels->getSelfCollisionGroups();
	    
	    int nscgroups = 0;
	    for (unsigned int i = 0 ; i < groups.size() ; ++i)
	    {
		ROS_INFO("Self-collision check group %d", nscgroups);
		for (unsigned int j = 0 ; j < groups[i].size() ; ++j)
		    ROS_INFO("  %s", groups[i][j].c_str());
		nscgroups++;
	    }    
	    
	    if (nscgroups == 0)
		ROS_WARN("No self-collision checking enabled");
	    
	    ROS_INFO("Self-collision monitor is active, with scaling %g, padding %g", m_scaling, m_padding);
	}
    }
    
    void mechanismStateCallback(const robot_msgs::MechanismStateConstPtr &mechanismState)
    {
	bool change = false;
	
	if (m_robotState)
	{
	    static bool firstTime = true;
	    unsigned int n = mechanismState->get_joint_states_size();
	    for (unsigned int i = 0 ; i < n ; ++i)
	    {
		planning_models::KinematicModel::Joint* joint = m_kmodel->getJoint(mechanismState->joint_states[i].name);
		if (joint)
		{
		    if (joint->usedParams == 1)
		    {
			double pos = mechanismState->joint_states[i].position;
			bool this_changed = m_robotState->setParamsJoint(&pos, mechanismState->joint_states[i].name);
			change = change || this_changed;
		    }
		    else
			if (firstTime)
			    ROS_WARN("Incorrect number of parameters: %s (expected %d, had 1)", mechanismState->joint_states[i].name.c_str(), joint->usedParams);
		}
		else
		    if (firstTime)
			ROS_ERROR("Unknown joint: %s", mechanismState->joint_states[i].name.c_str());
	    }
	    firstTime = false;
	}
	if (change)
	    stateUpdate();
    }
    
    void stateUpdate(void)
    {
	// when this function is called, we have a collision space set
	// up and we know the position of the robot has changed
	
	ros::Time start_time = ros::Time::now();
	
	// do forward kinematics to compute the new positions of all robot parts of interest
	m_kmodel->computeTransforms(m_robotState->getParams());

	// ask the collision space to look at the updates
	m_collisionSpace->updateRobotModel();
	
	// get the first contact point
	std::vector<collision_space::EnvironmentModel::Contact> contacts;
	m_collisionSpace->getCollisionContacts(contacts, 1);
	
	if (contacts.size() > 0)
	{
	    ROS_WARN("Collision found in %g seconds", (ros::Time::now() - start_time).toSec());
	    for (unsigned int i = 0 ; i < contacts.size() ; ++i)
	    {
		ROS_INFO("Collision between link '%s' and '%s'", contacts[i].link1->name.c_str(), contacts[i].link2 ? contacts[i].link2->name.c_str() : "ENVIRONMENT");
		ROS_INFO("Contact point (in robot frame): (%g, %g, %g)", contacts[i].pos.x(), contacts[i].pos.y(), contacts[i].pos.z());
		ROS_INFO("Contact normal: (%g, %g, %g)", contacts[i].normal.x(), contacts[i].normal.y(), contacts[i].normal.z());
		ROS_INFO("Contact depth: %g", contacts[i].depth);
	    }
	}
    }
    
    ros::NodeHandle                                                 m_nodeHandle;
    ros::Subscriber                                                 m_mechanismStateSubscriber;
    
    // we don't want to detect a collision after it happened, but this
    // is what collision checkers do, so we scale the robot up by a
    // small factor; when a collision is found between the inflated
    // parts, the robot should take action to preserve itself
    double                                                          m_scaling;
    double                                                          m_padding;
    
    boost::shared_ptr<planning_environment::CollisionModels>        m_envModels;

    // the complete robot state
    boost::shared_ptr<planning_models::KinematicModel::StateParams> m_robotState;

    // the kinematic model
    boost::shared_ptr<planning_models::KinematicModel>              m_kmodel;

    // the collision space
    boost::shared_ptr<collision_space::EnvironmentModel>            m_collisionSpace;
    
};

int main(int argc, char **argv)
{
    ros::init(argc, argv, "self_watch");
    
    SelfWatch sw;    
    sw.run();    
    
    return 0;
}
