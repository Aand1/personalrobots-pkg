
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

/*! \mainpage
 *  \htmlinclude manifest.html
 */

#include <ros/ros.h>
#include "image_msgs/Image.h"
#include "calibration_msgs/DenseLaserSnapshot.h"

using namespace std ;
using namespace ros ;


/**
 * Builds a ROS image from an exisiting DenseLaserSnapshot. Extremely useful as a debugging tool
 */
class LaserImager
{
public:

  LaserImager()
  {
    sub_ = n_.subscribe("dense_laser_snapshot", 1, &LaserImager::snapshotCallback, this) ;
    intensity_pub_ = n_.advertise<image_msgs::Image> ("dense_laser_intensity", 1) ;
    range_pub_ = n_.advertise<image_msgs::Image> ("dense_laser_intensity", 1) ;

  }

  void snapshotCallback(const calibration_msgs::DenseLaserSnapshotConstPtr& msg)
  {
    image_msgs::Image image ;
    const unsigned int N = msg->num_scans * msg->readings_per_scan ;


    image.header.stamp = msg->header.stamp ;
    image.depth = "uint8" ;
    image.encoding = "mono" ;

    image.uint8_data.layout.dim.resize(3) ;
    image.uint8_data.layout.dim[0].label = "height" ;
    image.uint8_data.layout.dim[0].size   = msg->num_scans ;
    image.uint8_data.layout.dim[0].stride = N ;

    image.uint8_data.layout.dim[1].label = "width" ;
    image.uint8_data.layout.dim[1].size   = msg->readings_per_scan ;
    image.uint8_data.layout.dim[1].stride = msg->readings_per_scan ;

    image.uint8_data.layout.dim[2].label = "elem" ;
    image.uint8_data.layout.dim[2].size   = 1 ;
    image.uint8_data.layout.dim[2].stride = 1 ;

    image.uint8_data.layout.data_offset = 0 ;

    image.uint8_data.data.resize(msg->num_scans*msg->readings_per_scan) ;

    for(unsigned int i=0; i<N; i++)
    {
      image.uint8_data.data[i] = (unsigned int) ((fmin(3000, fmax(2000, msg->intensities[i])) - 2000) / 1000.0 * 255) ;
    }
    intensity_pub_.publish(image) ;
  }

private:
  NodeHandle n_ ;
  Subscriber sub_ ;
  Publisher intensity_pub_ ;
  Publisher range_pub_ ;

};




int main(int argc, char** argv)
{
  ros::init(argc, argv, "laser_imager") ;

  LaserImager imager ;

  ros::spin() ;

  return 0 ;
}
