/*
 * Copyright (c) 2008 Radu Bogdan Rusu <rusu -=- cs.tum.edu>
 *
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
 *
 * $Id: semantic_point_annotator.cpp 8082 2008-12-15 00:40:22Z veedee $
 *
 */

/**
@mainpage

@htmlinclude manifest.html

\author Radu Bogdan Rusu

@b semantic_point_annotator annotates 3D point clouds with semantic labels.

 **/

// ROS core
#include <ros/node.h>
// ROS messages
#include <std_msgs/PointCloud.h>
#include <std_msgs/Polygon3D.h>
#include <std_msgs/PolygonalMap.h>

// Sample Consensus
#include <sample_consensus/sac.h>
#include <sample_consensus/msac.h>
#include <sample_consensus/sac_model_plane.h>

// Cloud geometry
#include <cloud_geometry/areas.h>
#include <cloud_geometry/point.h>
#include <cloud_geometry/distances.h>
#include <cloud_geometry/nearest.h>

#include <sys/time.h>

using namespace std;
using namespace std_msgs;

class SemanticPointAnnotator : public ros::node
{
  public:

    // ROS messages
    PointCloud cloud_, cloud_annotated_;
    Point32 z_axis_;

    // Parameters
    int sac_min_points_per_model_;
    double sac_distance_threshold_, eps_angle_;

    double rule_floor_, rule_ceiling_, rule_wall_;
    double rule_table_min_, rule_table_max_;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    SemanticPointAnnotator () : ros::node ("semantic_point_annotator")
    {
      param ("~rule_floor", rule_floor_, 0.1);           // Rule for FLOOR
      param ("~rule_ceiling", rule_ceiling_, 2.0);       // Rule for CEILING
      param ("~rule_table_min", rule_table_min_, 0.5);   // Rule for MIN TABLE
      param ("~rule_table_max", rule_table_max_, 1.5);   // Rule for MIN TABLE
      param ("~rule_wall", rule_wall_, 2.0);             // Rule for WALL

      param ("~p_sac_min_points_per_model", sac_min_points_per_model_, 20);   // 20 points
      param ("~p_sac_distance_threshold", sac_distance_threshold_, 0.03);     // 3 cm 
      param ("~p_eps_angle_", eps_angle_, 10.0);                              // 10 degrees

      eps_angle_ = (eps_angle_ * M_PI / 180.0);     // conver to radians

      subscribe ("cloud_normals", cloud_, &SemanticPointAnnotator::cloud_cb, 1);

      advertise<PointCloud> ("cloud_annotated", 1);

      z_axis_.x = 0; z_axis_.y = 0; z_axis_.z = 1;

      cloud_annotated_.chan.resize (3);
      //cloud_annotated_.chan[0].name = "intensities";
      cloud_annotated_.chan[0].name = "r";
      cloud_annotated_.chan[1].name = "g";
      cloud_annotated_.chan[2].name = "b";
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    virtual ~SemanticPointAnnotator () { }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void
      fitSACPlane (PointCloud *points, vector<int> *indices, vector<vector<int> > &inliers, vector<vector<double> > &coeff)
    {
      if ((int)indices->size () < sac_min_points_per_model_)
        return;

      // Create and initialize the SAC model
      sample_consensus::SACModelPlane *model = new sample_consensus::SACModelPlane ();
      sample_consensus::SAC *sac             = new sample_consensus::MSAC (model, sac_distance_threshold_);
      model->setDataSet (points, *indices);

      PointCloud pts (*points);
      int nr_points_left = indices->size ();
      while (nr_points_left > sac_min_points_per_model_)
      {
        // Search for the best plane
        if (sac->computeModel ())
        {
          // Obtain the inliers and the planar model coefficients
          inliers.push_back (sac->getInliers ());
          coeff.push_back (sac->computeCoefficients ());
          fprintf (stderr, "> Found a model supported by %d inliers: [%g, %g, %g, %g]\n", sac->getInliers ().size (),
                   coeff[coeff.size () - 1][0], coeff[coeff.size () - 1][1], coeff[coeff.size () - 1][2], coeff[coeff.size () - 1][3]);

          // Project the inliers onto the model
          model->projectPointsInPlace (sac->getInliers (), coeff[coeff.size () - 1]);

          // Remove the current inliers in the model
          nr_points_left = sac->removeInliers ();
        }
      }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Callback
    void cloud_cb ()
    {
      Point32 robot_origin;
      robot_origin.x = robot_origin.y = robot_origin.z = 0.0;

      ROS_INFO ("Received %d data points.", cloud_.pts.size ());

      cloud_annotated_.header = cloud_.header;

      int nx = cloud_geometry::getChannelIndex (cloud_, "nx");
      int ny = cloud_geometry::getChannelIndex (cloud_, "ny");
      int nz = cloud_geometry::getChannelIndex (cloud_, "nz");

      if ( (cloud_.chan.size () < 3) || (nx == -1) || (ny == -1) || (nz == -1) )
      {
        ROS_ERROR ("This PointCloud message does not contain normal information!");
        return;
      }

      timeval t1, t2;
      gettimeofday (&t1, NULL);

      // Select points whose normals are parallel with the Z-axis
      vector<int> z_axis_indices, xy_axis_indices;
      cloud_geometry::getPointIndicesAxisParallelNormals (&cloud_, nx, ny, nz, eps_angle_, z_axis_, z_axis_indices);
      cloud_geometry::getPointIndicesAxisPerpendicularNormals (&cloud_, nx, ny, nz, eps_angle_, z_axis_, xy_axis_indices);

      // Find the dominant planes
      vector<vector<int> > inliers_parallel, inliers_perpendicular;
      vector<vector<double> > coeff;
      // Find all planes parallel with XY
      fitSACPlane (&cloud_, &z_axis_indices, inliers_parallel, coeff);
      // Find all planes perpendicular to XY
      fitSACPlane (&cloud_, &xy_axis_indices, inliers_perpendicular, coeff);

      // Mark points in the output cloud
      int total_p = 0, nr_p = 0;
      for (unsigned int i = 0; i < inliers_parallel.size (); i++)
        total_p += inliers_parallel[i].size ();
      for (unsigned int i = 0; i < inliers_perpendicular.size (); i++)
        total_p += inliers_perpendicular[i].size ();

      cloud_annotated_.pts.resize (total_p);
      cloud_annotated_.chan[0].vals.resize (total_p);
      cloud_annotated_.chan[1].vals.resize (total_p);
      cloud_annotated_.chan[2].vals.resize (total_p);

      // Get all planes parallel to the floor (perpendicular to Z)
      for (unsigned int i = 0; i < inliers_parallel.size (); i++)
      {
        // Compute a distance from 0,0,0 to the plane
        double distance = cloud_geometry::distances::pointToPlaneDistance (robot_origin, coeff[i]);

        double r = 1.0, g = 1.0, b = 1.0;
        // Test for floor
        if (distance < rule_floor_)
        {
          r = 0.6; g = 0.67; b = 0.01;
        }
        // Test for ceiling
        if (distance > rule_ceiling_)
        {
          r = 0.8; g = 0.63; b = 0.33;
        }
        // Test for tables
        if (distance > rule_table_min_ && distance < rule_table_max_)
        {
          r = 0.0; g = 1.0; b = 0.0;
        }

        for (unsigned int j = 0; j < inliers_parallel[i].size (); j++)
        {
          cloud_annotated_.pts[nr_p].x = cloud_.pts.at (inliers_parallel[i].at (j)).x;
          cloud_annotated_.pts[nr_p].y = cloud_.pts.at (inliers_parallel[i].at (j)).y;
          cloud_annotated_.pts[nr_p].z = cloud_.pts.at (inliers_parallel[i].at (j)).z;
          //cloud_annotated_.chan[0].vals[i] = intensity_value;
          cloud_annotated_.chan[0].vals[nr_p] = r;
          cloud_annotated_.chan[1].vals[nr_p] = g;
          cloud_annotated_.chan[2].vals[nr_p] = b;
          nr_p++;
        }
      }

      // Get all planes perpendicular to the floor (parallel to XY)
      for (unsigned int i = 0; i < inliers_perpendicular.size (); i++)
      {
//        float intensity_value = rand () / (RAND_MAX + 1.0);    // Get a random value for the intensity
        // Get the minimum and maximum bounds of the plane
        Point32 minP, maxP;
        cloud_geometry::getMinMax (cloud_, inliers_perpendicular[i], minP, maxP);

        double r, g, b;
        r = g = b = 1.0;

        // Test for wall
        //if (-coeff[i][3] / coeff[i][2] < rule_wall_)
        if (maxP.z > rule_wall_)
        {
          r = rand () / (RAND_MAX + 1.0);
          g = rand () / (RAND_MAX + 1.0);
          b = rand () / (RAND_MAX + 1.0);
          r = r * .3;
          b = b * .3 + .7;
          g = g * .3;
        }
        for (unsigned int j = 0; j < inliers_perpendicular[i].size (); j++)
        {
          cloud_annotated_.pts[nr_p].x = cloud_.pts.at (inliers_perpendicular[i].at (j)).x;
          cloud_annotated_.pts[nr_p].y = cloud_.pts.at (inliers_perpendicular[i].at (j)).y;
          cloud_annotated_.pts[nr_p].z = cloud_.pts.at (inliers_perpendicular[i].at (j)).z;
          //cloud_annotated_.chan[0].vals[i] = intensity_value;
          cloud_annotated_.chan[0].vals[nr_p] = r;
          cloud_annotated_.chan[1].vals[nr_p] = g;
          cloud_annotated_.chan[2].vals[nr_p] = b;
          nr_p++;
        }
      }

      gettimeofday (&t2, NULL);
      double time_spent = t2.tv_sec + (double)t2.tv_usec / 1000000.0 - (t1.tv_sec + (double)t1.tv_usec / 1000000.0);
      ROS_INFO ("Number of points with normals approximately parallel to the Z axis: %d (%g seconds).", z_axis_indices.size (), time_spent);

      publish ("cloud_annotated", cloud_annotated_);
    }
};

/* ---[ */
int
  main (int argc, char** argv)
{
  ros::init (argc, argv);

  SemanticPointAnnotator p;
  p.spin ();

  ros::fini ();

  return (0);
}
/* ]--- */

