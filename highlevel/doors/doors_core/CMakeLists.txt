cmake_minimum_required(VERSION 2.4.6)

include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
#set(ROS_BUILD_TYPE RelWithDebInfo)
set(ROS_BUILD_TYPE Debug)
rospack(doors_core)
gensrv()
genmsg()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# executive interface
rospack_add_executable(action_runner
                       src/action_check_path.cpp 
                       src/action_detect_door.cpp 
                       src/action_detect_handle.cpp
                       src/action_detect_handle_no_camera.cpp
                       src/action_grasp_handle.cpp
                       src/action_touch_door.cpp
                       src/action_push_door.cpp
                       src/action_open_door.cpp
                       src/action_unlatch_handle.cpp
                       src/action_release_handle.cpp
		       src/action_runner.cpp
                      )


# test
rospack_add_executable(trigger_check_path
                       test/trigger_check_path.cpp
		       src/action_check_path.cpp
                      )

rospack_add_executable(trigger_doors_detector 
                       test/trigger_doors_detector.cpp
		       src/action_detect_door.cpp
		       src/action_detect_handle.cpp
                      )

rospack_add_executable(trigger_push_door
                       test/trigger_push_door.cpp
		       src/action_push_door.cpp
                      )

rospack_add_executable(trigger_doors_opener
                       test/trigger_doors_opener.cpp
		       src/action_open_door.cpp
                      )

rospack_add_executable(trigger_handle_release
                       test/trigger_handle_release.cpp
		       src/action_release_handle.cpp
                      )

rospack_add_executable(test_executive
		       test/test_executive.cpp
                      )

#rospack_add_executable(test_planner
#		       test/test_planner.cpp
#                      )

rospack_add_executable(move_base_door
                       src/move_base_door_action.cpp
		       src/door_reactive_planner.cpp
                      )
