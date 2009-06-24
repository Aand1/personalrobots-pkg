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

#include <gtest/gtest.h>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include "mechanism_model/robot.h"
#include "mechanism_model/chain.h"
#include "tf_conversions/tf_kdl.h"
#include <kdl/chainfksolverpos_recursive.hpp>
#include "test_helpers.h"

using namespace mechanism;

// 4 Links, but not all in the same chain
class ShortTreeTest : public testing::Test
{
protected:
  ShortTreeTest() : hw(0) {}
  virtual ~ShortTreeTest() {}

  virtual void SetUp()
  {
    model.links_.push_back(quickLink("a", "world", "", tf::Vector3(0,0,0), tf::Vector3(0,0,0)));

    model.joints_.push_back(quickJoint("ab", tf::Vector3(0,1,0)));
    model.links_.push_back(quickLink("b", "a", "ab", tf::Vector3(1,0,0), tf::Vector3(0,0,0)));

    model.joints_.push_back(quickJoint("bc", tf::Vector3(1,0,0)));
    model.links_.push_back(quickLink("c", "b", "bc", tf::Vector3(0,1,0), tf::Vector3(0,0,0)));

    model.joints_.push_back(quickJoint("bd", tf::Vector3(1,0,0)));
    model.links_.push_back(quickLink("d", "b", "bd", tf::Vector3(1,0,0), tf::Vector3(0,0,0)));

    state.reset(new RobotState(&model, &hw));
  }

  virtual void TearDown() {}

  Robot model;
  HardwareInterface hw;
  boost::scoped_ptr<RobotState> state;
};


TEST_F(ShortTreeTest, FKMatchOnTrees)
{
  Chain chain;
  EXPECT_TRUE(chain.init(&model, "a", "d"));
  EXPECT_EQ(0, chain.link_indices_[0]);
  EXPECT_EQ(1, chain.link_indices_[1]);
  EXPECT_EQ(3, chain.link_indices_[2]);
  EXPECT_EQ(0, chain.joint_indices_[0]);
  EXPECT_EQ(2, chain.joint_indices_[1]);

  KDL::Chain kdl;
  chain.toKDL(kdl);
  //for (unsigned int i=0; i<kdl.getNrOfJoints(); i++)
  //  cout << "kdl chain contains joint " << chain.getJointName(i) << endl;


  ASSERT_EQ((unsigned int) 3, kdl.getNrOfSegments());
  ASSERT_EQ((unsigned int) 2, kdl.getNrOfJoints());

  setJoint(state.get(), 0, M_PI/2);
  setJoint(state.get(), 1, M_PI/2);
  setJoint(state.get(), 2, M_PI);


  KDL::JntArray jnts(2);
  chain.getPositions(state->joint_states_, jnts);
}


int main(int argc, char **argv){
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}