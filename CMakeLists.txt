cmake_minimum_required(VERSION 3.25)

project(NiceServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -g -std=c++14")
set(source_dir "${PROJECT_SOURCE_DIR}/src/")

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy-assets.cmake
)

file(GLOB_RECURSE source_files "${source_dir}/*.*")

include_directories(
    src/http_connection
    src/server
    src/utils
    src/pool
)

add_executable(NiceServer ${source_files})
add_dependencies(NiceServer copy_assets)
target_link_libraries(NiceServer pthread mysqlclient)
