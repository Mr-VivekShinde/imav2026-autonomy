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

install(
  TARGETS px4_interface_monitor
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
