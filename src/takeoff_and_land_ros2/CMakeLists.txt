cmake_minimum_required(VERSION 3.8)
project(takeoff_and_land_ros2)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)


find_package(MAVSDK REQUIRED)

add_executable(takeoff_and_land_node
    src/takeoff_and_land_node.cpp
)

ament_target_dependencies(takeoff_and_land_node
    rclcpp
    std_msgs
    std_srvs
)

# Link MAVSDK
target_link_libraries(takeoff_and_land_node
    MAVSDK::mavsdk
)

install(TARGETS
    takeoff_and_land_node
    DESTINATION lib/${PROJECT_NAME}
)

# Install launch files
install(
    DIRECTORY launch
    DESTINATION share/${PROJECT_NAME}/
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # the following line skips the linter which checks for copyrights
  # comment the line when a copyright and license is added to all source files
  set(ament_cmake_copyright_FOUND TRUE)
  # the following line skips cpplint (only works in a git repo)
  # comment the line when this package is in a git repo and when
  # a copyright and license is added to all source files
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
