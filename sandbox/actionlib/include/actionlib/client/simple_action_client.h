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

#ifndef ACTIONLIB_SIMPLE_ACTION_CLIENT_H_
#define ACTIONLIB_SIMPLE_ACTION_CLIENT_H_

#include "ros/ros.h"
#include "actionlib/client/action_client.h"
#include "actionlib/client/simple_goal_state.h"

namespace actionlib
{

/**
 * \brief A Simple client implementation of the ActionInterface which supports only one goal at a time
 *
 * The SimpleActionClient wraps the exisitng ActionClient, and exposes a limited set of easy-to-use hooks
 * for the user. Note that the concept of GoalHandles has been completely hidden from the user, and that
 * they must query the SimplyActionClient directly in order to monitor a goal.
 */
template <class ActionSpec>
class SimpleActionClient
{

public:
  /**
   * \brief Simple constructor
   *
   * Constructs a SingleGoalActionClient and sets up the necessary ros topics for the ActionInterface
   * \param name The action name. Defines the namespace in which the action communicates
   */
  SimpleActionClient(const std::string& name) : ac_(name)
  {
    initClient();
  }

  /**
   * \brief Constructor with namespacing options
   *
   * Constructs a SingleGoalActionClient and sets up the necessary ros topics for
   * the ActionInterface, and namespaces them according the a specified NodeHandle
   * \param n The node handle on top of which we want to namespace our action
   * \param name The action name. Defines the namespace in which the action communicates
   */
  SimpleActionClient(const ros::NodeHandle& n, const std::string& name) : ac_(n, name)
  {
    initClient();
  }

  /**
   * \brief Sends a goal to the ActionServer, and also registers callbacks
   *
   * If a previous goal is already active when this is called. We simply forget
   * about that goal and start tracking the new goal. Not cancel requests are made.
   * \param done_cb     Callback that gets called on transitions to Done
   * \param active_cb   Callback that gets called on transitions to Active
   * \param feedback_cb Callback that gets called whenever feedback for this goal is received
   */
  void sendGoal(const Goal& goal,
                SimpleDoneCallback     result_cb   = SimpleDoneCallback(),
                SimpleActiveCallback   active_cb   = SimpleActiveCallback(),
                SimpleFeedbackCallback feedback_cb = SimpleFeedbackCallback());

  /**
   * \brief Blocks until this goal transitions to done
   */
  void waitForGoalToFinish();

  /**
   * \brief Get the current state of the goal: [PENDING], [ACTIVE], or [DONE]
   */
  SimpleGoalState getGoalState();

  /**
   * \brief Get the terminal state information for this goal
   *
   * Possible States Are: RECALLED, REJECTED, PREEMPTED, ABORTED, SUCCEEDED, LOST
   * This call only makes sense if SimpleGoalState==DONE. This will sends ROS_WARNs if we're not in DONE
   * \return The terminal state
   */
  TerminalState getTerminalState();

  /**
   * \brief Cancel all goals currently running on the action server
   *
   * This preempts all goals running on the action server at the point that
   * this message is serviced by the ActionServer.
   */
  void cancelAllGoals();

  /**
   * \brief Cancel all goals that were stamped at and before the specified time
   * \param time All goals stamped at or before `time` will be canceled
   */
  void cancelGoalsAtAndBeforeTime(const ros::Time& time);

  /**
   * \brief Cancel the goal that we are currently pursuing
   */
  void cancelCurrentGoal();

  /**
   * \brief Stops tracking the state of the current goal. Unregisters this goal's callbacks
   *
   * This is useful if we want to make sure we stop calling our callbacks before sending a new goal.
   * Note that this does not cancel the goal, it simply stops looking for status info about this goal.
   */
  void stopTrackingGoal();

private:
  typedef ActionClient<ActionSpec> ActionClientT;
  ActionClientT ac_;

};

}

#endif // ACTIONLIB_SINGLE_GOAL_ACTION_CLIENT_H_
