cmake_minimum_required(VERSION 3.25)

project(NiceServer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -std=c++14")
set(source_dir "${PROJECT_SOURCE_DIR}/src/")

file(GLOB_RECURSE source_files "${source_dir}/*.*")

include_directories(
    src/http_connection
    src/server
    src/utils
)

add_executable(NiceServer ${source_files})
target_link_libraries(NiceServer pthread mysqlclient ${Boost_LIBRARIES})
