cmake_minimum_required(VERSION 2.4.6)
include($ENV{ROS_ROOT}/core/rosbuild/rosbuild.cmake)
rospack(people_aware_nav)
gensrv()
genmsg()

rospack_add_boost_directories()

rospack_add_executable(bin/person_pos_sender src/person_node.cpp)
rospack_add_executable(bin/is_person_on_path src/is_person_on_path.cpp)
rospack_link_boost(bin/is_person_on_path thread)

rospack_add_lisp_executable(bin/hallway_move people-aware-nav lanes:main)

rospack_add_executable(bin/move_base_constrained src/move_base_constrained.cpp)
rospack_link_boost(bin/move_base_constrained thread)

rospack_add_executable(bin/head_controller src/head_controller.cpp)
rospack_link_boost(bin/head_controller thread)

rospack_add_library (navfn_constrained src/navfn_constrained.cpp)


