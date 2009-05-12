/*
 *  Gazebo - Outdoor Multi-Robot Simulator
 *  Copyright (C) 2003
 *     Nate Koenig & Andrew Howard
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 @mainpage
   Desc: RosSimIface plugin for maanipulating objects in Gazebo
   Author: John Hsu
   Date: 24 Sept 2008
   SVN info: $Id$
 @htmlinclude manifest.html
 @b RosSimIface plugin reads ROS PoseWithRatesStamped messages
 */

#include <algorithm>
#include <assert.h>

#include <gazebo_plugin/ros_sim_iface.h>

#include <gazebo/Sensor.hh>
#include <gazebo/Model.hh>
#include <gazebo/Global.hh>
#include <gazebo/XMLConfig.hh>
#include <gazebo/Simulator.hh>
#include <gazebo/gazebo.h>
#include <gazebo/GazeboError.hh>
#include <gazebo/ControllerFactory.hh>

using namespace gazebo;

GZ_REGISTER_DYNAMIC_CONTROLLER("ros_sim_iface", RosSimIface);

////////////////////////////////////////////////////////////////////////////////
// Constructor
RosSimIface::RosSimIface(Entity *parent)
    : Controller(parent)
{
  this->myParent = parent;

  if (!this->myParent)
    gzthrow("RosSimIface controller requires an Entity as its parent");


  rosnode = ros::g_node; // comes from where?
  int argc = 0;
  char** argv = NULL;
  if (rosnode == NULL)
  {
    ros::init(argc,argv);
    rosnode = new ros::Node("ros_gazebo",ros::Node::DONT_HANDLE_SIGINT);
    ROS_DEBUG("Starting node in simiface");
  }

  Param::Begin(&this->parameters);
  this->topicNameP = new ParamT<std::string>("topicName","simiface_pose", 0);
  this->frameNameP = new ParamT<std::string>("frameName","map", 0);
  this->modelNameP = new ParamT<std::string>("modelName","pr2_model", 1);
  this->xyzP  = new ParamT<Vector3>("xyz" ,Vector3(0,0,0), 0);
  this->rpyP  = new ParamT<Vector3>("rpy" ,Vector3(0,0,0), 0);
  this->velP  = new ParamT<Vector3>("vel" ,Vector3(0,0,0), 0);
  this->angVelP  = new ParamT<Vector3>("angVel" ,Vector3(0,0,0), 0);
  Param::End();
}

////////////////////////////////////////////////////////////////////////////////
// Destructor
RosSimIface::~RosSimIface()
{

  delete this->topicNameP;
  delete this->frameNameP;
  delete this->modelNameP;
  delete this->xyzP;
  delete this->rpyP;
  delete this->velP;
  delete this->angVelP;

}

////////////////////////////////////////////////////////////////////////////////
// Load the controller
void RosSimIface::LoadChild(XMLConfigNode *node)
{
  this->topicNameP->Load(node);
  this->frameNameP->Load(node);
  this->modelNameP->Load(node);
  this->xyzP->Load(node);
  this->rpyP->Load(node);
  this->velP->Load(node);
  this->angVelP->Load(node);

  this->topicName = this->topicNameP->GetValue();
  this->frameName = this->frameNameP->GetValue();
  this->modelName = this->modelNameP->GetValue();
  this->xyz = this->xyzP->GetValue();
  this->rpy = this->rpyP->GetValue();
  this->vel = this->velP->GetValue();
  this->angVel = this->angVelP->GetValue();

}

////////////////////////////////////////////////////////////////////////////////
// Initialize the controller
void RosSimIface::InitChild()
{

  ROS_DEBUG("ros simiface subscribing to %s", this->topicName.c_str());
  rosnode->subscribe( this->topicName, poseMsg, &RosSimIface::UpdateObjectPose, this, 10);

}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void RosSimIface::UpdateObjectPose()
{

  this->lock.lock();
  // copy data into pose
  this->poseMsg.header.frame_id = this->frameName;
  this->poseMsg.header.stamp.fromSec(floor(Simulator::Instance()->GetSimTime()));

  gazebo::Client *client = new gazebo::Client();
  gazebo::SimulationIface *simIface = new gazebo::SimulationIface();

  int serverId = 0;

  /// Connect to the libgazebo server
  try
  {
    client->ConnectWait(serverId, GZ_CLIENT_ID_USER_FIRST);
  }
  catch (gazebo::GazeboError e)
  {
    std::cout << "Gazebo error: Unable to connect\n" << e << "\n";
    //return -1;
  }

  /// Open the Simulation Interface
  try
  {
    simIface->Open(client, "default");
  }
  catch (std::string e)
  {
    std::cout << "Gazebo error: Unable to connect to the sim interface\n" << e << "\n";
    //return -1;
  }

  //simIface->data->reset = 1;

  simIface->Lock(1);

  gazebo::SimulationRequestData *request = &(simIface->data->requests[simIface->data->requestCount++]);
  request->type = gazebo::SimulationRequestData::SET_POSE3D;
  memcpy(request->modelName, this->modelName.c_str(), this->modelName.size());
  request->modelPose.pos.x = this->poseMsg.pos.position.x;
  request->modelPose.pos.y = this->poseMsg.pos.position.y;
  request->modelPose.pos.z = this->poseMsg.pos.position.z;
  Quatern quat = Quatern(this->poseMsg.pos.orientation.w,
                         this->poseMsg.pos.orientation.x,
                         this->poseMsg.pos.orientation.y,
                         this->poseMsg.pos.orientation.z);
  Vector3 euler = quat.GetAsEuler();
  request->modelPose.roll  = euler.x;
  request->modelPose.pitch = euler.y;
  request->modelPose.yaw   = euler.z;

  simIface->Unlock();

  // Example of resetting the simulator
  /*simIface->Lock(1);
  simIface->data->reset = 1;
  simIface->Unlock();
  */

  usleep(1000000);

  simIface->Close();
  delete simIface;

  this->lock.unlock();
}

////////////////////////////////////////////////////////////////////////////////
// Update the controller
void RosSimIface::UpdateChild()
{



}

////////////////////////////////////////////////////////////////////////////////
// Finalize the controller
void RosSimIface::FiniChild()
{
  rosnode->unsubscribe(this->topicName);
}


