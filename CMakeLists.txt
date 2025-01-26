cmake_minimum_required(VERSION 3.5)
project(MCS)


add_executable(MCS src/main.c)
target_include_directories(MCS PUBLIC "${CMAKE_SOURCE_DIR}/inc")
target_sources(MCS PUBLIC 
"${CMAKE_SOURCE_DIR}/src/cJSON.c"
"${CMAKE_SOURCE_DIR}/src/network.c"
"${CMAKE_SOURCE_DIR}/src/scanner.c"
)