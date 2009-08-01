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

#include <descriptors_3d/bounding_box_raw.h>

using namespace std;

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
BoundingBoxRaw::BoundingBoxRaw(double bbox_radius)
{
  result_size_ = 3;
  result_size_defined_ = true;

  neighborhood_radius_ = bbox_radius;
  neighborhood_radius_defined_ = true;
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
int BoundingBoxRaw::precompute(const robot_msgs::PointCloud& data,
                               cloud_kdtree::KdTree& data_kdtree,
                               const cv::Vector<const robot_msgs::Point32*>& interest_pts)
{
  return 0;
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
int BoundingBoxRaw::precompute(const robot_msgs::PointCloud& data,
                               cloud_kdtree::KdTree& data_kdtree,
                               const cv::Vector<const std::vector<int>*>& interest_region_indices)
{
  return 0;
}

// --------------------------------------------------------------
/* See function definition */
// --------------------------------------------------------------
void BoundingBoxRaw::computeNeighborhoodFeature(const robot_msgs::PointCloud& data,
                                                const vector<int>& neighbor_indices,
                                                const unsigned int interest_sample_idx,
                                                cv::Vector<float>& result) const
{
  result.resize(result_size_);
  const unsigned int nbr_neighbors = neighbor_indices.size();

  // --------------------------
  // Check for special case when no points in the bounding box as will initialize
  // the min/max extremas using the first point below
  if (nbr_neighbors == 0)
  {
    ROS_WARN("BoundingBox::computeBoundingBoxFeatures() No points to form bounding box");
    for (size_t i = 0 ; i < result_size_ ; i++)
    {
      result[i] = 0.0;
    }
    return;
  }

  // --------------------------
  // Initialize extrema values to the first coordinate in the interest region
  float min_x = data.pts[neighbor_indices[0]].x;
  float min_y = data.pts[neighbor_indices[0]].y;
  float min_z = data.pts[neighbor_indices[0]].z;
  float max_x = min_x;
  float max_y = min_y;
  float max_z = min_z;

  // --------------------------
  // Loop over remaining points in region and update extremas
  for (unsigned int i = 1 ; i < nbr_neighbors ; i++)
  {
    // x
    float curr_coord = data.pts[neighbor_indices[i]].x;
    if (curr_coord < min_x)
    {
      min_x = curr_coord;
    }
    if (curr_coord > max_x)
    {
      max_x = curr_coord;
    }
    // y
    curr_coord = data.pts[neighbor_indices[i]].y;
    if (curr_coord < min_y)
    {
      min_y = curr_coord;
    }
    if (curr_coord > max_y)
    {
      max_y = curr_coord;
    }
    // z
    curr_coord = data.pts[neighbor_indices[i]].z;
    if (curr_coord < min_z)
    {
      min_z = curr_coord;
    }
    if (curr_coord > max_z)
    {
      max_z = curr_coord;
    }
  }

  // --------------------------
  result[0] = max_x - min_x;
  result[1] = max_y - min_y;
  result[2] = max_z - min_z;
}
