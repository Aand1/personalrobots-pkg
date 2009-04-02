//Software License Agreement (BSD License)

//Copyright (c) 2008, Willow Garage, Inc.
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions
//are met:
//
// * Redistributions of source code must retain the above copyright
//   notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
//   copyright notice, this list of conditions and the following
//   disclaimer in the documentation and/or other materials provided
//   with the distribution.
// * Neither the name of Willow Garage, Inc. nor the names of its
//   contributors may be used to endorse or promote products derived
//   from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
//FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
//COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
//INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
//ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//POSSIBILITY OF SUCH DAMAGE.

/**
@mainpage

@htmlinclude manifest.html

\author Marius Muja, Sachin Chitta

@b

 **/

// ROS core
#include <ros/node.h>
// ROS messages
#include <robot_msgs/PointCloud.h>
#include <robot_msgs/Polygon3D.h>
#include <robot_msgs/PolygonalMap.h>

#include <robot_msgs/Point32.h>
#include <robot_msgs/Door.h>
#include <robot_msgs/VisualizationMarker.h>

// Sample Consensus
#include <point_cloud_mapping/sample_consensus/sac.h>
#include <point_cloud_mapping/sample_consensus/msac.h>
#include <point_cloud_mapping/sample_consensus/ransac.h>
#include <point_cloud_mapping/sample_consensus/lmeds.h>
#include <point_cloud_mapping/sample_consensus/sac_model_line.h>
#include <point_cloud_mapping/sample_consensus/sac_model_plane.h>
#include <point_cloud_mapping/sample_consensus/sac_model_oriented_line.h>

// Cloud geometry
#include <point_cloud_mapping/geometry/areas.h>
#include <point_cloud_mapping/geometry/point.h>
#include <point_cloud_mapping/geometry/distances.h>
#include <point_cloud_mapping/geometry/nearest.h>
#include <point_cloud_mapping/geometry/statistics.h>

#include <tf/transform_listener.h>
#include <tf/message_notifier.h>

#include <angles/angles.h>

#include <sys/time.h>

using namespace std;
using namespace robot_msgs;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Comparison operator for a vector of vectors
inline bool compareRegions (const std::vector<int> &a, const std::vector<int> &b)
{
  return (a.size () < b.size ());
}


class DoorStereo
{
  public:

    ros::Node *node_;

    PointCloud cloud_;

    tf::TransformListener *tf_;

    tf::MessageNotifier<robot_msgs::PointCloud>* message_notifier_;


    /********** Parameters that need to be gotten from the param server *******/
    std::string door_msg_topic_, stereo_cloud_topic_;
    int sac_min_points_per_model_;
    double sac_distance_threshold_;
    double eps_angle_, frame_multiplier_;
    int sac_min_points_left_;
    double door_min_width_, door_max_width_;
    robot_msgs::Point32 axis_;

    DoorStereo():message_notifier_(NULL)
    {
      node_ = ros::Node::instance();
      tf_ = new tf::TransformListener(*node_);
      node_->param<std::string>("~p_door_msg_topic_", door_msg_topic_, "door_message");                              // 10 degrees
      node_->param<std::string>("~p_stereo_cloud_topic_", stereo_cloud_topic_, "/stereo/cloud");                              // 10 degrees

      node_->param ("~p_sac_min_points_per_model", sac_min_points_per_model_, 50);  // 100 points at high resolution
      node_->param ("~p_sac_distance_threshold", sac_distance_threshold_, 0.01);     // 3 cm
      node_->param ("~p_eps_angle_", eps_angle_, 2.0);                              // 10 degrees
      node_->param ("~p_frame_multiplier_", frame_multiplier_,1.0);
      node_->param ("~p_sac_min_points_left", sac_min_points_left_, 10);
      node_->param ("~p_door_min_width", door_min_width_, 0.8);                    // minimum width of a door: 0.8m
      node_->param ("~p_door_max_width", door_max_width_, 1.4);                    // maximum width of a door: 1.4m

      double tmp_axis;
      node_->param ("~p_door_axis_x", tmp_axis, 0.0);                    // maximum width of a door: 1.4m
      axis_.x = tmp_axis;
      node_->param ("~p_door_axis_y", tmp_axis, 0.0);                    // maximum width of a door: 1.4m
      axis_.y = tmp_axis;
      node_->param ("~p_door_axis_z", tmp_axis, 1.0);                    // maximum width of a door: 1.4m
      axis_.z = tmp_axis;

      eps_angle_ = angles::from_degrees (eps_angle_);                    // convert to radians
      /*
      double tmp; int tmp2;
      node_->param("~p_door_frame_p1_x", tmp, 1.5); door_msg_.frame_p1.x = tmp;
      node_->param("~p_door_frame_p1_y", tmp, -0.5); door_msg_.frame_p1.y = tmp;
      node_->param("~p_door_frame_p2_x", tmp, 1.5); door_msg_.frame_p2.x = tmp;
      node_->param("~p_door_frame_p2_y", tmp, 0.5); door_msg_.frame_p2.y = tmp;
      node_->param("~p_door_hinge" , tmp2, -1); door_msg_.hinge = tmp2;
      node_->param("~p_door_rot_dir" , tmp2, -1); door_msg_.rot_dir = tmp2;
      door_msg_.header.frame_id = "base_footprint";
      */
      node_->advertise<robot_msgs::VisualizationMarker>( "visualizationMarker", 0 );

      message_notifier_ = new tf::MessageNotifier<robot_msgs::PointCloud> (tf_, node_,  boost::bind(&DoorStereo::stereoCloudCallBack, this, _1), stereo_cloud_topic_.c_str (), "stereo_link", 1);
    };

    ~DoorStereo()
    {
      node_->unadvertise("visualizationMarker");
      delete message_notifier_;
    }

    void stereoCloudCallBack(const tf::MessageNotifier<robot_msgs::PointCloud>::MessagePtr& cloud)
    {
      cloud_ = *cloud;
      ROS_INFO("Received a point cloud with %d points in frame: %s",(int) cloud_.pts.size(),cloud_.header.frame_id.c_str());
      if(cloud_.pts.empty())
      {
        ROS_WARN("Received an empty point cloud");
        return;
      }
      if ((int)cloud_.pts.size() < sac_min_points_per_model_)
        return;


      vector<int> indices;
      vector<int> possible_door_edge_points;
      indices.resize(cloud_.pts.size());

      for(unsigned int i=0; i < cloud_.pts.size(); i++)      //Use all the indices
      {
        indices[i] = i;
      }

      // Find the dominant lines
      vector<vector<int> > inliers;
      vector<vector<double> > coeff;

      vector<Point32> line_segment_min;
      vector<Point32> line_segment_max;

      fitSACLines(&cloud_,indices,inliers,coeff,line_segment_min,line_segment_max);

      //Publish all the lines as visualization markers
      for(unsigned int i=0; i < inliers.size(); i++)
      {
        robot_msgs::VisualizationMarker marker;
        marker.header.frame_id = cloud_.header.frame_id;
        marker.header.stamp = ros::Time((uint64_t)0ULL);
        marker.id = i;
        marker.type = robot_msgs::VisualizationMarker::LINE_STRIP;
        marker.action = robot_msgs::VisualizationMarker::ADD;
        marker.x = 0.0;
        marker.y = 0.0;
        marker.z = 0.0;
        marker.yaw = 0.0;
        marker.pitch = 0.0;
        marker.roll = 0.0;
        marker.xScale = 0.01;
        marker.yScale = 0.1;
        marker.zScale = 0.1;
        marker.alpha = 255;
        marker.r = 0;
        marker.g = 255;
        marker.b = 0;
        marker.set_points_size(2);

        marker.points[0].x = line_segment_min[i].x;
        marker.points[0].y = line_segment_min[i].y;
        marker.points[0].z = line_segment_min[i].z;

        marker.points[1].x = line_segment_max[i].x;
        marker.points[1].y = line_segment_max[i].y;
        marker.points[1].z = line_segment_max[i].z;

        node_->publish( "visualizationMarker", marker );
      }
   }

    void fitSACLines(PointCloud *points, vector<int> indices, vector<vector<int> > &inliers, vector<vector<double> > &coeff, vector<Point32> &line_segment_min, vector<Point32> &line_segment_max)
    {
      Point32 minP, maxP;

      vector<int> inliers_local;

      // Create and initialize the SAC model
       sample_consensus::SACModelOrientedLine *model = new sample_consensus::SACModelOrientedLine ();
       sample_consensus::SAC *sac             = new sample_consensus::RANSAC (model, sac_distance_threshold_);
       sac->setMaxIterations (100);
       model->setDataSet (points, indices);

       robot_msgs::Vector3Stamped axis_transformed;
       robot_msgs::Vector3Stamped axis_original;
       ROS_INFO("Original axis: %f %f %f",axis_.x,axis_.y,axis_.z);

       axis_original.vector.x = axis_.x;
       axis_original.vector.y = axis_.y;
       axis_original.vector.z = axis_.z;
       axis_original.header.frame_id = "base_link";
       axis_original.header.stamp = ros::Time(0.0);

       tf_->transformVector("stereo_link",axis_original,axis_transformed);

       Point32 axis_point_32;
       axis_point_32.x = axis_transformed.vector.x;
       axis_point_32.y = axis_transformed.vector.y;
       axis_point_32.z = axis_transformed.vector.z;

       ROS_INFO("Transformed axis: %f %f %f",axis_point_32.x,axis_point_32.y,axis_point_32.z);


       // visualize the axis
       {
    	   robot_msgs::VisualizationMarker marker;
    	   marker.header.frame_id = cloud_.header.frame_id;
    	   marker.header.stamp = ros::Time((uint64_t)0ULL);
    	   marker.id = 100001;
    	   marker.type = robot_msgs::VisualizationMarker::LINE_STRIP;
    	   marker.action = robot_msgs::VisualizationMarker::ADD;
    	   marker.x = 0.0;
    	   marker.y = 0.0;
    	   marker.z = 0.0;
    	   marker.yaw = 0.0;
    	   marker.pitch = 0.0;
    	   marker.roll = 0.0;
    	   marker.xScale = 0.01;
    	   marker.yScale = 0.1;
    	   marker.zScale = 0.1;
    	   marker.alpha = 255;
    	   marker.r = 0;
    	   marker.g = 255;
    	   marker.b = 0;
    	   marker.set_points_size(2);

    	   marker.points[0].x = 0;
    	   marker.points[0].y = 0;
    	   marker.points[0].z = 0;

    	   marker.points[1].x = axis_transformed.vector.x;
    	   marker.points[1].y = axis_transformed.vector.y;
    	   marker.points[1].z = axis_transformed.vector.z;

    	   node_->publish( "visualizationMarker", marker );
       }


       model->setAxis (axis_point_32);
       model->setEpsAngle (eps_angle_);

      // Now find the best fit line to this set of points and a corresponding set of inliers
      // Prepare enough space
      int number_remaining_points = indices.size();

      while(number_remaining_points > sac_min_points_left_)
      {
        if(sac->computeModel())
        {
          if((int) sac->getInliers().size() < sac_min_points_per_model_)
            break;
          inliers.push_back(sac->getInliers());
          vector<double> model_coefficients;
          sac->computeCoefficients (model_coefficients);
          coeff.push_back (model_coefficients);

          //Find the edges of the line segments
          cloud_geometry::statistics::getLargestDiagonalPoints(*points, inliers.back(), minP, maxP);
          line_segment_min.push_back(minP);
          line_segment_max.push_back(maxP);

          fprintf (stderr, "> Found a model supported by %d inliers: [%g, %g, %g, %g]\n", (int)sac->getInliers ().size (), coeff[coeff.size () - 1][0], coeff[coeff.size () - 1][1], coeff[coeff.size () - 1][2], coeff[coeff.size () - 1][3]);

          // Remove the current inliers in the model
          number_remaining_points = sac->removeInliers ();
        }
      }
    }


    void obtainCloudIndicesSet(robot_msgs::PointCloud *points, vector<int> &indices, robot_msgs::Door door_msg, tf::TransformListener *tf, double frame_multiplier)
    {
      // frames used
      string cloud_frame = points->header.frame_id;
      string door_frame   = door_msg.header.frame_id;

      // Resize the resultant indices to accomodate all data
      indices.resize (points->pts.size ());

      // Transform the X-Y bounds from the door request service into the cloud TF frame
      tf::Stamped<Point32> frame_p1 (door_msg.frame_p1, points->header.stamp, door_frame);
      tf::Stamped<Point32> frame_p2 (door_msg.frame_p2, points->header.stamp, door_frame);
      transformPoint (tf,cloud_frame, frame_p1, frame_p1);
      transformPoint (tf,cloud_frame, frame_p2, frame_p2);

      ROS_INFO ("Start detecting door at points in frame %s [%g, %g, %g] -> [%g, %g, %g]",
                cloud_frame.c_str (), frame_p1.x, frame_p1.y, frame_p1.z, frame_p2.x, frame_p2.y, frame_p2.z);

      // Obtain the bounding box information in the reference frame of the laser scan
      Point32 min_bbox, max_bbox;

      if (frame_multiplier == -1)
      {
        ROS_INFO ("Door frame multiplier set to -1. Using the entire point cloud data.");
        // Use the complete bounds of the point cloud
        cloud_geometry::statistics::getMinMax (*points, min_bbox, max_bbox);
        for (unsigned int i = 0; i < points->pts.size (); i++)
          indices[i] = i;
      }
      else
      {
        // Obtain the actual 3D bounds
        get3DBounds (&frame_p1, &frame_p2, min_bbox, max_bbox, frame_multiplier);

        int nr_p = 0;
        for (unsigned int i = 0; i < points->pts.size (); i++)
        {
          if ((points->pts[i].x >= min_bbox.x && points->pts[i].x <= max_bbox.x) &&
              (points->pts[i].y >= min_bbox.y && points->pts[i].y <= max_bbox.y) &&
              (points->pts[i].z >= min_bbox.z && points->pts[i].z <= max_bbox.z))
          {
            indices[nr_p] = i;
            nr_p++;
          }
        }
        indices.resize (nr_p);
      }
      ROS_INFO ("Number of points in bounds [%f,%f,%f] -> [%f,%f,%f]: %d.",min_bbox.x, min_bbox.y, min_bbox.z, max_bbox.x, max_bbox.y, max_bbox.z, (int)indices.size ());
    }

    void get3DBounds (Point32 *p1, Point32 *p2, Point32 &min_b, Point32 &max_b, int multiplier)
    {
      // Get the door_frame distance in the X-Y plane
      float door_frame = sqrt ( (p1->x - p2->x) * (p1->x - p2->x) + (p1->y - p2->y) * (p1->y - p2->y) );

      float center[2];
      center[0] = (p1->x + p2->x) / 2.0;
      center[1] = (p1->y + p2->y) / 2.0;

      // Obtain the bounds (doesn't matter which is min and which is max at this point)
      min_b.x = center[0] + (multiplier * door_frame) / 2.0;
      min_b.y = center[1] + (multiplier * door_frame) / 2.0;
      min_b.z = -FLT_MAX;

      max_b.x = center[0] - (multiplier * door_frame) / 2.0;
      max_b.y = center[1] - (multiplier * door_frame) / 2.0;
      max_b.z = FLT_MAX;

      // Order min/max
      if (min_b.x > max_b.x)
      {
        float tmp = min_b.x;
        min_b.x = max_b.x;
        max_b.x = tmp;
      }
      if (min_b.y > max_b.y)
      {
        float tmp = min_b.y;
        min_b.y = max_b.y;
        max_b.y = tmp;
      }
    }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/** \brief Transform a given point from its current frame to a given target frame
 * \param tf a pointer to a TransformListener object
 * \param target_frame the target frame to transform the point into
 * \param stamped_in the input point
 * \param stamped_out the output point
 */
    inline void
    transformPoint (tf::TransformListener *tf, const std::string &target_frame,
                    const tf::Stamped< robot_msgs::Point32 > &stamped_in, tf::Stamped< robot_msgs::Point32 > &stamped_out)
    {
      tf::Stamped<tf::Point> tmp;
      tmp.stamp_ = stamped_in.stamp_;
      tmp.frame_id_ = stamped_in.frame_id_;
      tmp[0] = stamped_in.x;
      tmp[1] = stamped_in.y;
      tmp[2] = stamped_in.z;

      tf->transformPoint (target_frame, tmp, tmp);

      stamped_out.stamp_ = tmp.stamp_;
      stamped_out.frame_id_ = tmp.frame_id_;
      stamped_out.x = tmp[0];
      stamped_out.y = tmp[1];
      stamped_out.z = tmp[2];
    }
};


/* ---[ */
int
  main (int argc, char** argv)
{
  ros::init (argc, argv);

  ros::Node n("door_stereo");

  DoorStereo p;

  ROS_INFO("Waiting for tracker to finish");

  n.spin ();
  return (0);
}
/* ]--- */

