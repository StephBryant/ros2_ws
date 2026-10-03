cmake_minimum_required(VERSION 3.8)
project(cmd_vel_monitor)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ROS 2 Humble 使用 C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_msgs REQUIRED)

# add the library (optional)
add_library(${PROJECT_NAME} SHARED src/cmd_vel_monitor.cpp)
target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
ament_target_dependencies(${PROJECT_NAME} rclcpp geometry_msgs)

# the executable
add_executable(cmd_vel_monitor_node src/main.cpp)
target_link_libraries(cmd_vel_monitor_node ${PROJECT_NAME})

# the safety node executable
# 不链接 ${PROJECT_NAME}（safety_node 没用库里的类），三个依赖全部显式声明
add_executable(safety_node src/safety_node.cpp)
ament_target_dependencies(safety_node rclcpp geometry_msgs std_msgs)

# the monitor node executable
add_executable(monitor_node src/monitor_node.cpp)
ament_target_dependencies(monitor_node rclcpp geometry_msgs std_msgs)

# install
install(DIRECTORY include/
  DESTINATION include)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(TARGETS cmd_vel_monitor_node
  DESTINATION lib/${PROJECT_NAME})
install(TARGETS safety_node
  DESTINATION lib/${PROJECT_NAME})
install(TARGETS monitor_node
  DESTINATION lib/${PROJECT_NAME})

# 安装参数文件和 launch 文件到 share 目录，
# 这样装完之后 get_package_share_directory() 才能找到它们
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME})
install(PROGRAMS launch/bringup.launch.py
  DESTINATION share/${PROJECT_NAME}/launch)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_export_include_directories(include)
ament_export_libraries(${PROJECT_NAME})
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(rclcpp)

ament_package()