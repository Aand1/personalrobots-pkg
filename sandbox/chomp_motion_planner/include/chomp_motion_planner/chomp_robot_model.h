/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage, Inc.
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

/** \author Mrinal Kalakrishnan */

#ifndef CHOMP_ROBOT_MODEL_H_
#define CHOMP_ROBOT_MODEL_H_


#include <chomp_motion_planner/treefksolverjointposaxis.hpp>
#include <chomp_motion_planner/chomp_collision_point.h>
#include <ros/ros.h>
#include <planning_environment/models/robot_models.h>
#include <kdl/tree.hpp>
#include <kdl/chain.hpp>

#include <map>
#include <vector>
#include <string>
#include <Eigen/Core>
#include <cstdlib>

namespace chomp
{

/**
 * \brief Contains all the information needed for CHOMP planning.
 *
 * Initializes the robot models for CHOMP
 * from the "robot_description" parameter, in the same way that
 * planning_environment::RobotModels does it.
 */
class ChompRobotModel
{
public:

  /**
   * \brief Contains information about a single joint for CHOMP planning
   */
  struct ChompJoint
  {
    const KDL::Joint* kdl_joint_;                               /**< Pointer to the KDL joint in the tree */
    int kdl_joint_index_;                                       /**< Index for use in a KDL joint array */
    int chomp_joint_index_;                                     /**< Joint index for CHOMP */
    std::string joint_name_;                                    /**< Name of the joint */
    std::string link_name_;                                     /**< Name of the corresponding link (from planning.yaml) */
    bool wrap_around_;                                          /**< Does this joint wrap-around? */
    bool has_joint_limits_;                                     /**< Are there joint limits? */
    double joint_limit_min_;                                    /**< Minimum joint angle value */
    double joint_limit_max_;                                    /**< Maximum joint angle value */
    double joint_update_limit_;                                 /**< Maximum amount the joint value can be updated in an iteration */
  };

  /**
   * \brief Contains information about a planning group
   */
  struct ChompPlanningGroup
  {
    std::string name_;                                          /**< Name of the planning group */
    int num_joints_;                                            /**< Number of joints used in planning */
    std::vector<ChompJoint> chomp_joints_;                      /**< Joints used in planning */
    std::vector<std::string> link_names_;                       /**< Links used in planning */
    std::vector<std::string> collision_link_names_;             /**< Links used in collision checking */
    std::vector<ChompCollisionPoint> collision_points_;         /**< Ordered list of collision checking points (from root to tip) */

    /**
     * Gets a random state vector within the joint limits
     */
    template <typename Derived>
    void getRandomState(Eigen::MatrixBase<Derived>& state_vec) const;

    /**
     * Adds the collision point to this planning group, if any of the joints in this group can
     * control the collision point in some way. Also converts the ChompCollisionPoint::parent_joints
     * vector into group joint indexes
     */
    void addCollisionPoint(ChompCollisionPoint& collision_point, ChompRobotModel& robot_model);
  };

  ChompRobotModel();
  virtual ~ChompRobotModel();

  /**
   * \brief Initializes the robot models for CHOMP
   *
   * \return true if successful, false if not
   */
  bool init();

  /**
   * \brief Gets the planning group corresponding to the group name
   */
  const ChompPlanningGroup* getPlanningGroup(std::string group_name) const;

  /**
   * \brief Gets the planning_environment::RobotModels class
   */
  const planning_environment::RobotModels* getRobotModels() const;

  /**
   * \brief Gets the number of joints in the KDL tree
   */
  int getNumKDLJoints() const;

  /**
   * \brief Gets the KDL tree
   */
  const KDL::Tree* getKDLTree() const;

  /**
   * \brief Gets the KDL joint number from the URDF joint name
   *
   * \return -1 if the joint name is not found
   */
  int urdfNameToKdlNumber(std::string urdf_name) const;

  /**
   * \brief Gets the URDF joint name from the KDL joint number
   *
   * \return "" if the number does not have a name
   */
  const std::string kdlNumberToUrdfName(int kdl_number) const;

  const KDL::TreeFkSolverJointPosAxis* getForwardKinematicsSolver() const;

  const std::string& getReferenceFrame() const;

private:
  ros::NodeHandle node_handle_;                                 /**< ROS Node handle */
  planning_environment::RobotModels *robot_models_;             /**< Robot model */

  KDL::Tree kdl_tree_;                                          /**< The KDL tree of the entire robot */
  int num_kdl_joints_;                                          /**< Total number of joints in the KDL tree */
  std::map<std::string, std::string> joint_segment_mapping_;    /**< Joint -> Segment mapping for KDL tree */
  std::map<std::string, std::string> segment_joint_mapping_;    /**< Segment -> Joint mapping for KDL tree */
  std::vector<std::string> kdl_number_to_urdf_name_;            /**< Mapping from KDL joint number to URDF joint name */
  std::map<std::string, int> urdf_name_to_kdl_number_;          /**< Mapping from URDF joint name to KDL joint number */
  std::map<std::string, ChompPlanningGroup> planning_groups_;   /**< Planning group information */
  KDL::TreeFkSolverJointPosAxis *fk_solver_;                    /**< Forward kinematics solver for the tree */
  double collision_clearance_default_;                          /**< Default clearance for all collision links */
  std::string reference_frame_;                                 /**< Reference frame for all kinematics operations */
  std::map<std::string, std::vector<ChompCollisionPoint> > link_collision_points_;    /**< Collision points associated with every link */

  void addCollisionPointsFromLinkRadius(std::string link_name, double radius, double clearance);
};

/////////////////////////////// inline functions follow ///////////////////////////////////

inline const ChompRobotModel::ChompPlanningGroup* ChompRobotModel::getPlanningGroup(std::string group_name) const
{
  std::map<std::string, ChompRobotModel::ChompPlanningGroup>::const_iterator it = planning_groups_.find(group_name);
  if (it == planning_groups_.end())
    return NULL;
  else
    return &(it->second);
}

inline const planning_environment::RobotModels* ChompRobotModel::getRobotModels() const
{
  return robot_models_;
}

inline int ChompRobotModel::getNumKDLJoints() const
{
  return num_kdl_joints_;
}

inline const KDL::Tree* ChompRobotModel::getKDLTree() const
{
  return &kdl_tree_;
}

inline int ChompRobotModel::urdfNameToKdlNumber(std::string urdf_name) const
{
  std::map<std::string, int>::const_iterator it = urdf_name_to_kdl_number_.find(urdf_name);
  if (it!=urdf_name_to_kdl_number_.end())
    return it->second;
  else
    return -1;
}

inline const std::string ChompRobotModel::kdlNumberToUrdfName(int kdl_number) const
{
  if (kdl_number<0 || kdl_number>=num_kdl_joints_)
    return std::string("");
  else
    return kdl_number_to_urdf_name_[kdl_number];
}

inline const KDL::TreeFkSolverJointPosAxis* ChompRobotModel::getForwardKinematicsSolver() const
{
  return fk_solver_;
}

inline const std::string& ChompRobotModel::getReferenceFrame() const
{
  return reference_frame_;
}

template <typename Derived>
void ChompRobotModel::ChompPlanningGroup::getRandomState(Eigen::MatrixBase<Derived>& state_vec) const
{
  for (int i=0; i<num_joints_; i++)
  {
    double min = chomp_joints_[i].joint_limit_min_;
    double max = chomp_joints_[i].joint_limit_max_;
    if (!chomp_joints_[i].has_joint_limits_)
    {
      min = -M_PI/2.0;
      max = M_PI/2.0;
    }
    state_vec(i) = ((((double)rand())/RAND_MAX) * (max-min)) + min;
  }
}


} // namespace chomp
#endif /* CHOMP_ROBOT_MODEL_H_ */
