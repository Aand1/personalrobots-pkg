/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2009, Willow Garage
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

#include <descriptors_3d/generic/orientation_generic.h>

using namespace std;

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
OrientationGeneric::OrientationGeneric()
{
  result_size_ = 1;
  result_size_defined_ = true;
}

OrientationGeneric::~OrientationGeneric()
{
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
void OrientationGeneric::doComputation(const sensor_msgs::PointCloud& data,
                                       cloud_kdtree::KdTree& data_kdtree,
                                       const cv::Vector<const geometry_msgs::Point32*>& interest_pts,
                                       cv::Vector<cv::Vector<float> >& results)
{
  // ----------------------------------------
  // Iterate over each interest point, grab neighbors within bbox radius,
  // then compute bounding box
  size_t nbr_interest_pts = interest_pts.size();
  for (size_t i = 0 ; i < nbr_interest_pts ; i++)
  {
    computeOrientation(i, results[i]);
  }
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
void OrientationGeneric::doComputation(const sensor_msgs::PointCloud& data,
                                       cloud_kdtree::KdTree& data_kdtree,
                                       const cv::Vector<const std::vector<int>*>& interest_region_indices,
                                       cv::Vector<cv::Vector<float> >& results)
{
  // ----------------------------------------
  // Iterate over each interest point, grab neighbors within bbox radius,
  // then compute bounding box
  size_t nbr_interest_regions = interest_region_indices.size();
  for (size_t i = 0 ; i < nbr_interest_regions ; i++)
  {
    computeOrientation(i, results[i]);
  }
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
inline void OrientationGeneric::computeOrientation(const unsigned int interest_sample_idx,
                                                   cv::Vector<float>& result) const
{
  const Eigen::Vector3d* curr_local_direction = (*local_directions_)[interest_sample_idx];
  if (curr_local_direction != NULL)
  {
    // Invariant: local and reference directions are both unit
    float cos_theta = curr_local_direction->dot(reference_direction_);
    if (cos_theta < 0.0)
    {
      cos_theta = curr_local_direction->dot(reference_direction_flipped_);
    }

    result.resize(result_size_);
    result[0] = cos_theta;
  }
}

