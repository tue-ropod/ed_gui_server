cmake_minimum_required(VERSION 2.8.3)
project(ed_gui_server)

find_package(catkin REQUIRED COMPONENTS
  ed
  geometry_msgs
  message_generation
  rgbd
  tf
  urdf
  visualization_msgs
)

find_package(Boost REQUIRED COMPONENTS filesystem)

add_message_files(
  FILES
    Color.msg
    EntityInfo.msg
    EntityInfos.msg
    Mesh.msg
    Area.msg
    EntityMeshAndAreas.msg
    Polygon.msg
)

add_service_files(
  FILES
    GetEntityInfo.srv
    Interact.srv
    QueryEntities.srv
    QueryMeshes.srv
)

generate_messages(
  DEPENDENCIES
    geometry_msgs
)

catkin_package(
  # INCLUDE_DIRS include
  # LIBRARIES bla
  # CATKIN_DEPENDS other_catkin_pkg
  # DEPENDS system_lib
  CATKIN_DEPENDS ed geometry_msgs message_runtime rgbd tf urdf visualization_msgs
)

include_directories(
  ${catkin_INCLUDE_DIRS}
)

add_library(ed_gui_server_plugin
  src/gui_server_plugin.cpp
  src/robot.cpp
)
target_link_libraries(ed_gui_server_plugin ${catkin_LIBRARIES})
add_dependencies(ed_gui_server_plugin ${catkin_EXPORTED_TARGETS})
add_dependencies(ed_gui_server_plugin ${PROJECT_NAME}_generate_messages_cpp)

add_executable(ed_rviz_publisher tools/rviz_publisher.cpp)
target_link_libraries(ed_rviz_publisher ${catkin_LIBRARIES})
add_dependencies(ed_rviz_publisher ${catkin_EXPORTED_TARGETS})
add_dependencies(ed_rviz_publisher ${PROJECT_NAME}_generate_messages_cpp)
