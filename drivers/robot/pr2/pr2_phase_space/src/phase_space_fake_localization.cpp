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

//! \author Sachin Chitta

/****
 * This node takes the MocapSnapshot packet and repackages into a form that can be used with the odometry
 */

#include "ros/node.h"

// Messages
#include "mocap_msgs/MocapSnapshot.h"
#include "mocap_msgs/MocapMarker.h"
#include "mocap_msgs/MocapBody.h"

#include "geometry_msgs/Transform.h"
#include "nav_msgs/Odometry.h"
#include "nav_msgs/Odometry.h"

#include <tf/tf.h>
#include <tf/transform_broadcaster.h>


namespace pr2_phase_space
{

/**
 * \brief Repackages phase_space_snapshot data as odometry and ground truth data
 *
 * Parameters
 * - @b "~base_id"                : @b [int] the PhaseSpace ID of the rigid body corresponding to the base
 * - @b "~publish_transform"      : @b [bool] true: Publish the map-base transform. false: Don't publish this
 * - @b "~publish_localized_pose" : @b [bool] true: Publish the localized_pose.     false: Don't publish this
 **/
class PhaseSpaceLocalization : public ros::Node
{
public :

  PhaseSpaceLocalization() : ros::Node("phase_space_fake_localization")
  {
    param("~publish_localized_pose", publish_localized_pose_, true) ;
    param("~publish_transform", publish_transform_, true) ;
    param("~base_id", base_id_, 1) ;

    advertise<nav_msgs::Odometry>("localizedpose", 1);
    advertise<nav_msgs::Odometry>("base_pose_ground_truth", 1) ;

    m_tfServer = new tf::TransformBroadcaster();

    subscribe("phase_space_snapshot", snapshot_, &PhaseSpaceLocalization::snapshotCallback, 10) ;

    publish_success_count_ = 0 ;
    publish_attempt_count_ = 0 ;
  }

  ~PhaseSpaceLocalization()
  {
    unsubscribe("phase_space_snapshot") ;
    unadvertise("localizedpose") ;
    unadvertise("base_pose_ground_truth") ;
    if (m_tfServer)
      delete m_tfServer; 
  }

  void snapshotCallback()
  {
    //printf("%u  ", snapshot_.frameNum) ;
    for (unsigned int i=0; i<snapshot_.get_bodies_size(); i++)                      // Iterate over every rigid body we see in PhaseSpace
    {
      if (snapshot_.bodies[i].id == base_id_)                                       // Check if we found the robot base in the list of rigid bodies
      {
        const mocap_msgs::MocapBody& body = snapshot_.bodies[0] ;

        // Build Transform Message
        tf::Transform mytf;
        tf::transformMsgToTF(body.pose,mytf);

        if (publish_transform_)
          m_tfServer->sendTransform(mytf,m_currentPos.header.stamp,"base","map");

        // Build Localized Pose Message
        m_currentPos.header = snapshot_.header;
        m_currentPos.header.frame_id = "map";

        m_currentPos.pose.pose.position.x = body.pose.translation.x;
        m_currentPos.pose.pose.position.y = body.pose.translation.y;

        double yaw,pitch,roll;
        mytf.getBasis().getEulerZYX(yaw,pitch,roll);
        m_currentPos.pose.pose.orientation= tf::createQuaternionMsgFromYaw(yaw);

        if (publish_localized_pose_)
          publish("localizedpose", m_currentPos) ;

        // Build Ground Truth Message
        nav_msgs::Odometry m_ground_truth ;
        m_ground_truth.header = snapshot_.header ;
        m_ground_truth.pose.pose.position.x = body.pose.translation.x ;
        m_ground_truth.pose.pose.position.y = body.pose.translation.y ;
        m_ground_truth.pose.pose.position.z = body.pose.translation.z ;
        m_ground_truth.pose.pose.orientation = body.pose.rotation ;

        publish("base_pose_ground_truth", m_ground_truth) ;

        publish_success_count_++ ;

        break ;                                                                       // Can exit the for loop, since we already found the base
      }
    }
    publish_attempt_count_++ ;
    if (snapshot_.frameNum % 480 == 0)
    {
      printf("Saw the base and published in %3u/%3u of the most recent messages\n", publish_success_count_, publish_attempt_count_) ;
      publish_success_count_ = 0 ;
      publish_attempt_count_ = 0 ;
    }
  }

private:

  mocap_msgs::MocapSnapshot snapshot_;
  nav_msgs::Odometry m_currentPos;
  tf::TransformBroadcaster *m_tfServer;
  unsigned int publish_success_count_ ;
  unsigned int publish_attempt_count_ ;

  int base_id_ ;
  bool publish_localized_pose_ ;
  bool publish_transform_ ;

} ;

}

using namespace pr2_phase_space ;

int main(int argc, char** argv)
{
  
  printf("Initializing Ros...") ;
  ros::init(argc, argv);
  printf("Done\n") ;

  PhaseSpaceLocalization phase_space_loc;

  phase_space_loc.spin() ;

  

  printf("**** Exiting ****\n") ;

  return 0 ;
}
