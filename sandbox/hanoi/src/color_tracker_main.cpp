#include <ros/ros.h>

#include "ColorTracker.h"

int main(int argc, char **argv)
{
  ros::init(argc, argv, "color_tracker");
  ros::NodeHandle nh;

  ColorTracker ct(nh);

  ros::spin();
}
