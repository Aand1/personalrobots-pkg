cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
rospack(voxel3d)
set(ROS_BUILD_TYPE RelWithDebInfo)

ADD_DEFINITIONS(-msse -msse2)

genmsg()
gensrv()

rospack_add_library(voxel3d
  src/voxel3d.cpp
)

rospack_add_gtest(test/test_kernel test/test_kernel.cpp)
target_link_libraries(test/test_kernel voxel3d)

rospack_add_executable(bin/voxel_node src/voxel_node.cpp)
target_link_libraries(bin/voxel_node voxel3d)

rospack_add_executable(bin/timing src/timing.cpp)
target_link_libraries(bin/timing voxel3d)
