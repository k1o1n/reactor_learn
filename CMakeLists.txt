cmake_minimum_required(VERSION 3.19)

project(
  reactor
  VERSION 0.0.1
  DESCRIPTION "epoll封装成reactor模式简易服务器"
  LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE SRC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(adachi STATIC ${SRC_SOURCES})
target_include_directories(adachi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)
target_link_libraries(test PUBLIC adachi)