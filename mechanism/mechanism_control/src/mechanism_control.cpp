///////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2008, Willow Garage, Inc.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//   * Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright
//     notice, this list of conditions and the following disclaimer in the
//     documentation and/or other materials provided with the distribution.
//   * Neither the name of Willow Garage, Inc. nor the names of its
//     contributors may be used to endorse or promote products derived from
//     this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//////////////////////////////////////////////////////////////////////////////
/*
 * Author: Stuart Glaser
 */

#include "mechanism_control/mechanism_control.h"
#include <boost/thread/thread.hpp>
#include "ros/console.h"

using namespace mechanism;
using namespace boost::accumulators;

MechanismControl::MechanismControl(HardwareInterface *hw) :
  state_(NULL), hw_(hw), initialized_(0), publisher_("/diagnostics", 1), please_remove_(-1), removed_(NULL)
{
  memset(controllers_, 0, MAX_NUM_CONTROLLERS * sizeof(void*));
  model_.hw_ = hw;

  diagnostics_.max1_.resize(60);
  diagnostics_.max_ = 0;
  for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
  {
    diagnostics_.controllers_[i].max1_.resize(60);
    diagnostics_.controllers_[i].max_ = 0;
  }
}

MechanismControl::~MechanismControl()
{
  // Destroy all controllers
  for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
  {
    if (controllers_[i] != NULL)
      delete controllers_[i];
  }

  if (state_)
    delete state_;
}

bool MechanismControl::initXml(TiXmlElement* config)
{
  model_.initXml(config);
  state_ = new RobotState(&model_, hw_);

  initialized_ = true;
  return true;
}

controller::Controller* MechanismControl::getControllerByName(std::string name)
{
  for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
    if (controller_names_[i] == name)
     return controllers_[i];

  return NULL;
}

#define ADD_STRING_FMT(lab, fmt, ...) \
  s.label = (lab); \
  { char buf[1024]; \
    snprintf(buf, sizeof(buf), fmt, ##__VA_ARGS__); \
    s.value = buf; \
  } \
  strings.push_back(s)

void MechanismControl::publishDiagnostics()
{
  if (publisher_.trylock())
  {
    int active = 0;
    TimeStatistics a;

    vector<robot_msgs::DiagnosticStatus> statuses;
    vector<robot_msgs::DiagnosticValue> values;
    vector<robot_msgs::DiagnosticString> strings;
    robot_msgs::DiagnosticStatus status;
    robot_msgs::DiagnosticValue v;
    robot_msgs::DiagnosticString s;

    status.name = "Mechanism Control";
    status.level = 0;
    status.message = "OK";

    for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
    {
      if (controllers_[i] != NULL)
      {
        ++active;
        double m = extract_result<tag::max>(diagnostics_.controllers_[i].acc_);
        diagnostics_.controllers_[i].max1_.push_back(m);
        diagnostics_.controllers_[i].max_ = std::max(m, diagnostics_.controllers_[i].max_);
        ADD_STRING_FMT(controller_names_[i], "%.4f (avg) %.4f (stdev) %.4f (max) %.4f (1-min max) %.4f (life max)", mean(diagnostics_.controllers_[i].acc_)*1e6, sqrt(variance(diagnostics_.controllers_[i].acc_))*1e6, m*1e6, *std::max_element(diagnostics_.controllers_[i].max1_.begin(), diagnostics_.controllers_[i].max1_.end())*1e6, diagnostics_.controllers_[i].max_*1e6);
        /* Clear accumulator */
        diagnostics_.controllers_[i].acc_ = a;
      }
    }

    double m = extract_result<tag::max>(diagnostics_.acc_);
    diagnostics_.max1_.push_back(m);
    diagnostics_.max_ = std::max(m, diagnostics_.max_);
    ADD_STRING_FMT("Overall", "%.4f (avg) %.4f (stdev) %.4f (max) %.4f (1-min max) %.4f (life max)", mean(diagnostics_.acc_)*1e6, sqrt(variance(diagnostics_.acc_))*1e6, m*1e6, *std::max_element(diagnostics_.max1_.begin(), diagnostics_.max1_.end())*1e6, diagnostics_.max_*1e6);
    ADD_STRING_FMT("Active controllers", "%d", active);

    status.set_values_vec(values);
    status.set_strings_vec(strings);
    statuses.push_back(status);
    publisher_.msg_.set_status_vec(statuses);
    publisher_.unlockAndPublish();

    /* Clear accumulator */
    diagnostics_.acc_ = a;
  }
}

// Must be realtime safe.
void MechanismControl::update()
{
  static int count = 0;

  state_->propagateState();
  state_->zeroCommands();

  // Update all controllers
  double start_update = realtime_gettime();
  for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
  {
    if (controllers_[i] != NULL)
    {
      double start = realtime_gettime();
      controllers_[i]->update();
      double end = realtime_gettime();
      diagnostics_.controllers_[i].acc_(end - start);
    }
  }
  double end_update = realtime_gettime();
  diagnostics_.acc_(end_update - start_update);

  state_->enforceSafety();
  state_->propagateEffort();

  if (++count == 1000)
  {
    count = 0;
    publishDiagnostics();
  }

  // If there's a controller to remove, we take it out of the controllers array.
  if (please_remove_ >= 0)
  {
    removed_ = controllers_[please_remove_];
    controllers_[please_remove_] = NULL;
    please_remove_ = -1;
  }
}

void MechanismControl::getControllerNames(std::vector<std::string> &controllers)
{
  controllers_lock_.lock();
  for (int i = 0; i < MAX_NUM_CONTROLLERS; i++)
  {
    if (controllers_[i] != NULL)
    {
      controllers.push_back(controller_names_[i]);
    }
  }
  controllers_lock_.unlock();
}

bool MechanismControl::addController(controller::Controller *c, const std::string &name)
{
  boost::mutex::scoped_lock lock(controllers_lock_);

  if (getControllerByName(name))
  {
    ROS_ERROR("A controller with the name %s already exists", name.c_str());
    return false;
  }

  for (int i = 0; i < MAX_NUM_CONTROLLERS; i++)
  {
    if (controllers_[i] == NULL)
    {
      controllers_[i] = c;
      controller_names_[i] = name;
      return true;
    }
  }

  ROS_ERROR("No room for new controller: %s", name.c_str());
  return false;
}


bool MechanismControl::spawnController(const std::string &type,
                                       const std::string &name,
                                       TiXmlElement *config)
{
  controller::Controller *c = controller::ControllerFactory::Instance().CreateObject(type);
  if (c == NULL)
  {
    ROS_ERROR("Could spawn controller %s because controller type %s does not exist",
              name.c_str(), type.c_str());
    return false;
  }

  ROS_INFO("Spawning %s", name.c_str());

  if (!c->initXml(state_, config) ||
      !addController(c, name))
  {
    delete c;
    return false;
  }

  return true;
}

bool MechanismControl::killController(const std::string &name)
{
  bool found = false;
  controllers_lock_.lock();
  removed_ = NULL;
  for (int i = 0; i < MAX_NUM_CONTROLLERS; ++i)
  {
    if (controllers_[i] && controller_names_[i] == name)
    {
      found = true;
      please_remove_ = i;
      ROS_INFO("Kill: controller %s found", name.c_str());
      break;
    }
  }

  if (found)
  {
    while (removed_ == NULL)
      usleep(10000);

    ROS_INFO("Kill: controller %s removed", name.c_str());

    delete removed_;
    removed_ = NULL;

    ROS_INFO("Kill: controller %s destroyed", name.c_str());
  }
  else
  {
    ROS_ERROR("Kill failed: no controller named %s exists", name.c_str());
  }

  controllers_lock_.unlock();
  return found;
}


MechanismControlNode::MechanismControlNode(MechanismControl *mc)
  : mc_(mc), cycles_since_publish_(0),
    mechanism_state_topic_("mechanism_state"),
    publisher_(mechanism_state_topic_, 1),
    transform_publisher_("tf_message", 5)
{
  assert(mc != NULL);
  assert(mechanism_state_topic_);
  if ((node_ = ros::Node::instance()) == NULL) {
    int argc = 0;
    char** argv = NULL;
    ros::init(argc, argv);
    node_ = new ros::Node("mechanism_control", ros::Node::DONT_HANDLE_SIGINT);
  }
}

MechanismControlNode::~MechanismControlNode()
{
}

bool MechanismControlNode::initXml(TiXmlElement *config)
{
  if (!mc_->initXml(config))
    return false;
  publisher_.msg_.set_joint_states_size(mc_->model_.joints_.size());
  publisher_.msg_.set_actuator_states_size(mc_->hw_->actuators_.size());

  // Counts the number of transforms
  int num_transforms = 0;
  for (unsigned int i = 0; i < mc_->model_.links_.size(); ++i)
  {
    if (mc_->model_.links_[i]->parent_name_ != std::string("world"))
      ++num_transforms;
  }
  transform_publisher_.msg_.set_transforms_size(num_transforms);


  // Advertise services
  node_->advertiseService("list_controllers", &MechanismControlNode::listControllers, this);
  list_controllers_guard_.set("list_controllers");
  node_->advertiseService("list_controller_types", &MechanismControlNode::listControllerTypes, this);
  list_controller_types_guard_.set("list_controller_types");
  node_->advertiseService("spawn_controller", &MechanismControlNode::spawnController, this);
  spawn_controller_guard_.set("spawn_controller");
  node_->advertiseService("kill_controller", &MechanismControlNode::killController, this);
  kill_controller_guard_.set("kill_controller");

  return true;
}

void MechanismControlNode::update()
{
  mc_->update();

  if (++cycles_since_publish_ >= CYCLES_PER_STATE_PUBLISH)
  {
    cycles_since_publish_ = 0;
    if (publisher_.trylock())
    {
      assert(mc_->model_.joints_.size() == publisher_.msg_.get_joint_states_size());
      for (unsigned int i = 0; i < mc_->model_.joints_.size(); ++i)
      {
        robot_msgs::JointState *out = &publisher_.msg_.joint_states[i];
        mechanism::JointState *in = &mc_->state_->joint_states_[i];
        out->name = mc_->model_.joints_[i]->name_;
        out->position = in->position_;
        out->velocity = in->velocity_;
        out->applied_effort = in->applied_effort_;
        out->commanded_effort = in->commanded_effort_;
        out->is_calibrated = in->calibrated_;
      }

      for (unsigned int i = 0; i < mc_->hw_->actuators_.size(); ++i)
      {
        robot_msgs::ActuatorState *out = &publisher_.msg_.actuator_states[i];
        ActuatorState *in = &mc_->hw_->actuators_[i]->state_;
        out->name = mc_->hw_->actuators_[i]->name_;
        out->encoder_count = in->encoder_count_;
        out->position = in->position_;
        out->timestamp = in->timestamp_;
        out->encoder_velocity = in->encoder_velocity_;
        out->velocity = in->velocity_;
        out->calibration_reading = in->calibration_reading_;
        out->calibration_rising_edge_valid = in->calibration_rising_edge_valid_;
        out->calibration_falling_edge_valid = in->calibration_falling_edge_valid_;
        out->last_calibration_rising_edge = in->last_calibration_rising_edge_;
        out->last_calibration_falling_edge = in->last_calibration_falling_edge_;
        out->is_enabled = in->is_enabled_;
        out->run_stop_hit = in->run_stop_hit_;
        out->last_requested_current = in->last_requested_current_;
        out->last_commanded_current = in->last_commanded_current_;
        out->last_measured_current = in->last_measured_current_;
        out->last_requested_effort = in->last_requested_effort_;
        out->last_commanded_effort = in->last_commanded_effort_;
        out->last_measured_effort = in->last_measured_effort_;
        out->motor_voltage = in->motor_voltage_;
        out->num_encoder_errors = in->num_encoder_errors_;
      }
      publisher_.msg_.time = mc_->hw_->current_time_;

      publisher_.unlockAndPublish();
    }


    // Frame transforms
    if (transform_publisher_.trylock())
    {
      //assert(mc_->model_.links_.size() == transform_publisher_.msg_.get_quaternions_size());
      int ti = 0;
      for (unsigned int i = 0; i < mc_->model_.links_.size(); ++i)
      {
        if (mc_->model_.links_[i]->parent_name_ == std::string("world"))
          continue;

        tf::Vector3 pos = mc_->state_->link_states_[i].rel_frame_.getOrigin();
        tf::Quaternion quat = mc_->state_->link_states_[i].rel_frame_.getRotation();
        std_msgs::TransformStamped &out = transform_publisher_.msg_.transforms[ti++];

        out.header.stamp.fromSec(mc_->hw_->current_time_);
        out.header.frame_id = mc_->model_.links_[i]->name_;
        out.parent_id = mc_->model_.links_[i]->parent_name_;
        out.transform.translation.x = pos.x();
        out.transform.translation.y = pos.y();
        out.transform.translation.z = pos.z();
        out.transform.rotation.w = quat.w();
        out.transform.rotation.x = quat.x();
        out.transform.rotation.y = quat.y();
        out.transform.rotation.z = quat.z();
      }

      transform_publisher_.unlockAndPublish();
    }
  }
}

bool MechanismControlNode::listControllerTypes(
  robot_srvs::ListControllerTypes::Request &req,
  robot_srvs::ListControllerTypes::Response &resp)
{
  (void) req;
  std::vector<std::string> types = controller::ControllerFactory::Instance().RegisteredIds();
  resp.set_types_vec(types);
  return true;
}

bool MechanismControlNode::spawnController(
  robot_srvs::SpawnController::Request &req,
  robot_srvs::SpawnController::Response &resp)
{
  TiXmlDocument doc;
  doc.Parse(req.xml_config.c_str());

  std::vector<uint8_t> oks;
  std::vector<std::string> names;

  TiXmlElement *config = doc.RootElement();
  if (!config)
  {
    ROS_ERROR("The XML given to SpawnController could not be parsed");
    return false;
  }
  if (config->ValueStr() != "controllers" &&
      config->ValueStr() != "controller")
  {
    ROS_ERROR("The XML given to SpawnController must have either \"controller\" or \
\"controllers\" as the root tag");
    return false;
  }

  if (config->ValueStr() == "controllers")
  {
    config = config->FirstChildElement("controller");
  }

  for (; config; config = config->NextSiblingElement("controller"))
  {
    bool ok = true;

    if (!config->Attribute("type"))
    {
      ROS_ERROR("Could not spawn a controller because no type was given");
      ok = false;
    }
    else if (!config->Attribute("name"))
    {
      ROS_ERROR("Could not spawn a controller because no name was given");
      ok = false;
    }
    else
    {
      ok = mc_->spawnController(config->Attribute("type"), config->Attribute("name"), config);
    }

    oks.push_back(ok);
    names.push_back(config->Attribute("name") ? config->Attribute("name") : "<unnamed>");
  }

  resp.set_ok_vec(oks);
  resp.set_name_vec(names);

  return true;
}

bool MechanismControlNode::listControllers(
  robot_srvs::ListControllers::Request &req,
  robot_srvs::ListControllers::Response &resp)
{
  std::vector<std::string> controllers;

  (void) req;
  mc_->getControllerNames(controllers);
  resp.set_controllers_vec(controllers);
  return true;
}

bool MechanismControlNode::killController(
  robot_srvs::KillController::Request &req,
  robot_srvs::KillController::Response &resp)
{
  resp.ok = mc_->killController(req.name);
  return true;
}
