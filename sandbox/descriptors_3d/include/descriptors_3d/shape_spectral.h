#ifndef __D3D_SHAPE_SPECTRAL_H__
#define __D3D_SHAPE_SPECTRAL_H__
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

#include <vector>

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/cvaux.hpp>

#include <descriptors_3d/generic/neighborhood_feature.h>
#include <descriptors_3d/spectral_analysis.h>

// --------------------------------------------------------------
//* ShapeSpectral
/*!
 * \brief A ShapeSpectral descriptor computes features that describe
 *        the local shape of a neighborhood of points.
 *
 * It is based from the Tensor Voting framework from Medioni et al.,
 * "A Computational Framework for Segmentation and Grouping", Elsevier 2000.
 */
// --------------------------------------------------------------
class ShapeSpectral: public Descriptor3D
{
  public:
    // --------------------------------------------------------------
    /*!
     * \brief Instantiates the ShapeSpectral descriptor
     *
     * The features indicate the flat-ness (F), linear-ness (L), and
     * scattered-ness (S) of a local neighborhood around an interest point/region.
     * The features are based on the eigenvalues from the scatter matrix
     * constructed from the neighborhood of points.
     *
     * A curvature (C) feature can optionally be computed
     *
     * The feature vector format is: [S L F (C)]
     */
    // --------------------------------------------------------------
    ShapeSpectral(SpectralAnalysis& spectral_information);

  protected:
    virtual int precompute(const sensor_msgs::PointCloud& data,
                           cloud_kdtree::KdTree& data_kdtree,
                           const cv::Vector<const geometry_msgs::Point32*>& interest_pts);

    virtual int precompute(const sensor_msgs::PointCloud& data,
                           cloud_kdtree::KdTree& data_kdtree,
                           const cv::Vector<const std::vector<int>*>& interest_region_indices);

    // --------------------------------------------------------------
    /*!
     * \brief Computes the bounding box dimensions around each interest point
     *
     * \warning setBoundingBoxRadius() must be called first
     * \warning If computing the bounding box in principle component space, then
     *          setSpectralRadius() or useSpectralInformation() must be called first
     *
     * \see Descriptor3D::compute
     */
    // --------------------------------------------------------------
    virtual void doComputation(const sensor_msgs::PointCloud& data,
                               cloud_kdtree::KdTree& data_kdtree,
                               const cv::Vector<const geometry_msgs::Point32*>& interest_pts,
                               cv::Vector<cv::Vector<float> >& results);

    // --------------------------------------------------------------
    /*!
     * \brief Computes the bounding box dimensions around/for each interest region
     *
     * \warning setBoundingBoxRadius() must be called first
     * \warning If computing the bounding box in principle component space, then
     *          setSpectralRadius() or useSpectralInformation() must be called first
     *
     * \see Descriptor3D::compute
     */
    // --------------------------------------------------------------
    virtual void doComputation(const sensor_msgs::PointCloud& data,
                               cloud_kdtree::KdTree& data_kdtree,
                               const cv::Vector<const std::vector<int>*>& interest_region_indices,
                               cv::Vector<cv::Vector<float> >& results);

    // --------------------------------------------------------------
    /*!
     * \brief Computes the bounding box information of the given neighborhood
     *
     * \param data The overall point cloud data
     * \param neighbor_indices List of indices in data that constitute the neighborhood
     * \param result The vector to hold the computed bounding box dimensions
     */
    // --------------------------------------------------------------
    virtual void computeShapeFeatures(const unsigned int interest_sample_idx, cv::Vector<float>& result) const;

  private:
    SpectralAnalysis* spectral_information_;
    const std::vector<const Eigen::Vector3d*>* eig_vals_;
};

#endif
