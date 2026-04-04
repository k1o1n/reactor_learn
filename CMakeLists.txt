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

option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)

set(ADACHI_COMMON_COMPILE_OPTIONS
  -Wall
  -Wextra
  -Werror
  -g
  -fno-omit-frame-pointer
)

set(ADACHI_SANITIZER_COMPILE_OPTIONS)
set(ADACHI_SANITIZER_LINK_OPTIONS)

if(ENABLE_TSAN)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "ThreadSanitizer requires Clang or GCC on Linux")
  endif()

  list(APPEND ADACHI_SANITIZER_COMPILE_OPTIONS
    -O1
    -fsanitize=thread
  )
  list(APPEND ADACHI_SANITIZER_LINK_OPTIONS
    -fsanitize=thread
  )
else()
  list(APPEND ADACHI_SANITIZER_COMPILE_OPTIONS
    -O3
  )
endif()

file(GLOB_RECURSE SRC_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

add_library(adachi STATIC ${SRC_SOURCES})
target_include_directories(adachi PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(adachi PRIVATE ${ADACHI_COMMON_COMPILE_OPTIONS} ${ADACHI_SANITIZER_COMPILE_OPTIONS})
# find_package(Threads)
# target_link_libraries(adachi PUBLIC Threads::Threads)

add_executable(test ${CMAKE_CURRENT_SOURCE_DIR}/test/test.cpp)
target_link_libraries(test PUBLIC adachi)
target_compile_options(test PRIVATE ${ADACHI_COMMON_COMPILE_OPTIONS} ${ADACHI_SANITIZER_COMPILE_OPTIONS})
target_link_options(test PRIVATE ${ADACHI_SANITIZER_LINK_OPTIONS})

