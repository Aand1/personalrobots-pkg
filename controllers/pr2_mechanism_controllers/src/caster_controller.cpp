/*
 * Copyright (c) 2008, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
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

/*
 * Author: Stuart Glaser
 */

#include "pr2_mechanism_controllers/caster_controller.h"
#include "pluginlib/class_list_macros.h"

PLUGINLIB_REGISTER_CLASS(CasterController, controller::CasterController, controller::Controller)

namespace controller {

const double CasterController::WHEEL_RADIUS = 0.079;
const double CasterController::WHEEL_OFFSET = 0.049;

CasterController::CasterController()
  : steer_velocity_(0), drive_velocity_(0)
{
}

CasterController::~CasterController()
{
}

bool CasterController::init(
  pr2_mechanism::RobotState *robot,
  const std::string &caster_joint,
  const std::string &wheel_l_joint, const std::string &wheel_r_joint,
  const control_toolbox::Pid &caster_pid, const control_toolbox::Pid &wheel_pid)
{
  caster_ = robot->getJointState(caster_joint);
  if (!caster_)
  {
    fprintf(stderr, "Error: Caster joint \"%s\" does not exist\n", caster_joint.c_str());
    return false;
  }

  if (!caster_vel_.init(robot, caster_joint, caster_pid))
    return false;
  if (!wheel_l_vel_.init(robot, wheel_l_joint, wheel_pid))
    return false;
  if (!wheel_r_vel_.init(robot, wheel_r_joint, wheel_pid))
    return false;

  return true;
}

bool CasterController::initXml(pr2_mechanism::RobotState *robot, TiXmlElement *config)
{
  TiXmlElement *jel = config->FirstChildElement("joints");
  if (!jel)
  {
    fprintf(stderr, "Error: CasterController could not find joints element\n");
    return false;
  }

  const char *caster_joint = jel->Attribute("caster");
  const char *wheel_l_joint = jel->Attribute("wheel_l");
  const char *wheel_r_joint = jel->Attribute("wheel_r");
  if (!caster_joint)
  {
    fprintf(stderr, "Error: CasterController was not given a caster joint\n");
    return false;
  }
  if (!wheel_l_joint)
  {
    fprintf(stderr, "Error: CasterController was not given a wheel_l joint\n");
    return false;
  }
  if (!wheel_r_joint)
  {
    fprintf(stderr, "Error: CasterController was not given a wheel_r joint\n");
    return false;
  }

  control_toolbox::Pid caster_pid, wheel_pid;
  TiXmlElement *cpel = config->FirstChildElement("caster_pid");
  TiXmlElement *wpel = config->FirstChildElement("wheel_pid");
  if (!cpel)
  {
    fprintf(stderr, "Error: CasterController was not given a caster_pid element\n");
    return false;
  }
  if (!caster_pid.initXml(cpel))
    return false;
  if (!wpel)
  {
    fprintf(stderr, "Error: CasterController was not given a wheel_pid element\n");
    return false;
  }
  if (!wheel_pid.initXml(wpel))
    return false;

  return init(robot, caster_joint, wheel_l_joint, wheel_r_joint, caster_pid, wheel_pid);
}

bool CasterController::init(pr2_mechanism::RobotState *robot, const ros::NodeHandle &n)
{
  node_ = n;
  assert(robot);

  // Reads in the joints to control

  std::string caster_joint_name, wheel_l_joint_name, wheel_r_joint_name;
  if (!node_.getParam("joints/caster", caster_joint_name))
  {
    ROS_ERROR("No caster joint given (namespace: %s)", node_.getNamespace().c_str());
    return false;
  }
  if (!node_.getParam("joints/wheel_l", wheel_l_joint_name))
  {
    ROS_ERROR("No wheel_l joint given (namespace: %s)", node_.getNamespace().c_str());
    return false;
  }
  if (!node_.getParam("joints/wheel_r", wheel_r_joint_name))
  {
    ROS_ERROR("No wheel_r joint given (namespace: %s)", node_.getNamespace().c_str());
    return false;
  }
  if (!(caster_ = robot->getJointState(caster_joint_name)))
  {
    ROS_ERROR("Caster joint \"%s\" does not exist (namespace: %s)",
              caster_joint_name.c_str(), node_.getNamespace().c_str());
    return false;
  }

  // Prepares the namespaces for the velocity controllers

  XmlRpc::XmlRpcValue caster_pid, wheel_pid;
  node_.getParam("caster_pid", caster_pid);
  node_.getParam("wheel_pid", wheel_pid);

  ros::NodeHandle
    caster_node(n, "caster"),
    wheel_l_node(n, "wheel_l"),
    wheel_r_node(n, "wheel_r");

  caster_node.setParam("type", std::string("JointVelocityController"));
  caster_node.setParam("joint", caster_joint_name);
  caster_node.setParam("pid", caster_pid);

  wheel_l_node.setParam("type", std::string("JointVelocityController"));
  wheel_l_node.setParam("joint", wheel_l_joint_name);
  wheel_l_node.setParam("pid", wheel_pid);

  wheel_r_node.setParam("type", std::string("JointVelocityController"));
  wheel_r_node.setParam("joint", wheel_r_joint_name);
  wheel_r_node.setParam("pid", wheel_pid);

  assert(robot);
  if (!caster_vel_.init(robot, caster_node)) return false;
  if (!wheel_l_vel_.init(robot, wheel_l_node)) return false;
  if (!wheel_r_vel_.init(robot, wheel_r_node)) return false;

  return true;
}

void CasterController::update()
{
  caster_vel_.setCommand(steer_velocity_);

  double wd = drive_velocity_ / WHEEL_RADIUS;  // Angular velocity due to driving
  double ws = (WHEEL_OFFSET / WHEEL_RADIUS) * steer_velocity_;  // Angular velocity due to steering
  wheel_r_vel_.setCommand(wd + ws);
  wheel_l_vel_.setCommand(wd - ws);

  caster_vel_.update();
  wheel_l_vel_.update();
  wheel_r_vel_.update();
}

}
