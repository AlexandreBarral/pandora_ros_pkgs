cmake_minimum_required(VERSION 2.8.3)
project(pandora_explorer)

find_package(catkin REQUIRED
    COMPONENTS
      roscpp
      visualization_msgs
      actionlib
      pandora_exploration_msgs
      geometry_msgs
      move_base_msgs
      costmap_2d
      nav_msgs
      pluginlib
      nav_core
      angles
      tf
      roslint
)


catkin_package(
#  DEPENDS
  CATKIN_DEPENDS
    roscpp
    visualization_msgs
    actionlib
    pandora_exploration_msgs
    geometry_msgs
    move_base_msgs
    costmap_2d
    nav_msgs
    pluginlib
    nav_core
    angles
    tf
#  INCLUDE_DIRS
#    include
#  LIBRARIES
)

include_directories(
    include
    ${catkin_INCLUDE_DIRS}
)


add_library(frontier_searchers src/map_frontier_search.cpp)
target_link_libraries(frontier_searchers ${catkin_LIBRARIES})

add_library(frontier_path_generators
    src/navfn_frontier_path_generator.cpp
    src/navfn_service_frontier_path_generator.cpp
)
target_link_libraries(frontier_path_generators ${catkin_LIBRARIES})

add_library(frontier_cost_functions
    src/cost_functions/distance_cost_function.cpp
    src/cost_functions/size_cost_function.cpp
    src/cost_functions/alignment_cost_function.cpp
    src/cost_functions/visited_cost_function.cpp
    )
target_link_libraries(frontier_cost_functions ${catkin_LIBRARIES})

add_library(frontier_goal_selector src/frontier_goal_selector.cpp)
target_link_libraries(frontier_goal_selector
    ${catkin_LIBRARIES}
    frontier_searchers
    frontier_path_generators
    frontier_cost_functions
)

add_executable(exploration_controller
  src/exploration_controller.cpp
  src/exploration_controller_node.cpp)
add_dependencies(exploration_controller ${catkin_EXPORTED_TARGETS})
target_link_libraries(exploration_controller
    ${catkin_LIBRARIES}
    frontier_goal_selector
)

## Roslint
set(ROSLINT_CPP_OPTS
  "--filter=-build/include_what_you_use, -whitespace/comments, -whitespace/blank_line, -whitespace/braces")
file(GLOB_RECURSE ${PROJECT_NAME}_LINT_SRCS
  RELATIVE ${PROJECT_SOURCE_DIR}
    include/pandora_explorer/*.h
    include/pandora_explorer/cost_functions/*.h
    src/*.cpp
    src/cost_functions/*.cpp
    )
list(REMOVE_ITEM ${PROJECT_NAME}_LINT_SRCS
  RELATIVE ${PROJECT_SOURCE_DIR}
    include/pandora_explorer/costmap_tools.h
    include/pandora_explorer/map_frontier_search.h
    include/pandora_explorer/frontier.h
    src/map_frontier_search.cpp
    )
roslint_cpp(${${PROJECT_NAME}_LINT_SRCS})

if(CATKIN_ENABLE_TESTING)
  add_subdirectory(test)
endif()
