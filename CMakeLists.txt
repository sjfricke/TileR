cmake_minimum_required(VERSION 3.4.0 FATAL_ERROR)

set(NAME TileR)

project(${NAME})

include_directories(include)
file(GLOB SOURCES "src/*.cpp")

set(AV_LIBS_LINK "-lavcodec -lavformat -lavutil")

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${AV_LIBS_LINK}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lavcodec -lavformat -lavutil")

# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()

add_definitions(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

add_executable(${NAME} ${SOURCES})

target_link_libraries(${NAME} avcodec avformat avutil)
