cmake_minimum_required(VERSION 3.8)

project(imav_flight_interface)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(px4_msgs REQUIRED)


add_executable(px4_interface_monitor
  src/px4_interface_monitor.cpp
)

ament_target_dependencies(px4_interface_monitor
  rclcpp
  px4_msgs
)


add_executable(offboard_heartbeat
  src/offboard_heartbeat.cpp
)

ament_target_dependencies(offboard_heartbeat
  rclcpp
  px4_msgs
)


add_executable(offboard_stationary
  src/offboard_stationary.cpp
)

ament_target_dependencies(offboard_stationary
  rclcpp
  px4_msgs
)


add_executable(vehicle_command_probe
  src/vehicle_command_probe.cpp
)

ament_target_dependencies(vehicle_command_probe
  rclcpp
  px4_msgs
)


add_executable(arm_disarm_probe
  src/arm_disarm_probe.cpp
)

ament_target_dependencies(arm_disarm_probe
  rclcpp
  px4_msgs
)


add_executable(takeoff_hover_land
  src/takeoff_hover_land.cpp
)

ament_target_dependencies(takeoff_hover_land
  rclcpp
  px4_msgs
)


install(
  TARGETS
    px4_interface_monitor
    offboard_heartbeat
    offboard_stationary
    vehicle_command_probe
    arm_disarm_probe
    takeoff_hover_land

  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
