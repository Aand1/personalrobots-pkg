cmake_minimum_required(VERSION 2.4.6)
set (ROS_BUILD_TYPE Release)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
rospack(sbpl_door_planner)
gensrv()

include_directories(${PROJECT_SOURCE_DIR}/code)
rospack_add_library(sbpl_door_planner
	            src/discrete_space_information/navxythetadoor/door_base_collision_cost.cpp
                    src/discrete_space_information/navxythetadoor/environment_navxythetadoor.cpp
                   )

#rospack_add_library(sbpl_door_planner
#	            src/discrete_space_information/navxythetadoor/door_base_collision_cost.cpp
#                   )

# Build executables in the bin directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

rospack_add_executable(planDoorPath src/discrete_space_information/navxythetadoor/door_base_collision_cost.cpp
                                    src/discrete_space_information/navxythetadoor/environment_navxythetadoor.cpp
                                    src/test/main.cpp)
target_link_libraries(planDoorPath sbpl_door_planner)


#rospack_add_executable(test_collision_cost src/discrete_space_information/navxythetadoor/test_door_base_collision_cost.cpp)
#target_link_libraries(test_collision_cost sbpl_door_planner)


#rospack_add_executable(doorBaseCollision src/discrete_space_information/navxythetadoor/door_base_collision_cost.cpp)
