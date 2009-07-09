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

#ifndef CHOMP_COLLISION_POINT_H_
#define CHOMP_COLLISION_POINT_H_

#include <kdl/frames.hpp>
#include <vector>

namespace chomp
{

class ChompCollisionPoint
{
public:
  ChompCollisionPoint(std::vector<int>& parent_joints, double radius, double clearance,
      int segment_number, KDL::Vector& position);
  virtual ~ChompCollisionPoint();

  bool isParentJoint(int joint) const;
  double getRadius() const;
  double getClearance() const;
  int getSegmentNumber() const;
  const KDL::Vector& getPosition() const;

private:
  std::vector<int> parent_joints_;      /**< Which joints can influence the motion of this point */
  double radius_;                       /**< Radius of the sphere */
  double clearance_;                    /**< Extra clearance required while optimizing */
  int segment_number_;                  /**< Which segment does this point belong to */
  KDL::Vector position_;                /**< Vector of this point in the frame of the above segment */
};

inline bool ChompCollisionPoint::isParentJoint(int joint) const
{
  return parent_joints_[joint]==1;
}

inline double ChompCollisionPoint::getRadius() const
{
  return radius_;
}

inline double ChompCollisionPoint::getClearance() const
{
  return clearance_;
}

inline int ChompCollisionPoint::getSegmentNumber() const
{
  return segment_number_;
}

inline const KDL::Vector& ChompCollisionPoint::getPosition() const
{
  return position_;
}


}

#endif /* CHOMP_COLLISION_POINT_H_ */
