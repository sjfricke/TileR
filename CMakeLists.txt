cmake_minimum_required(VERSION 3.4.0 FATAL_ERROR)

set(NAME TileR)

project(${NAME})

file(GLOB SOURCES "src/*.cpp")

if(${APPLE})
  include_directories(/usr/local/Cellar/ffmpeg/4.0/include/ include )
  link_directories(/usr/local/Cellar/ffmpeg/4.0/lib)
else()
  include_directories(include)
#  include_directories(/usr/include/x86_64-linux-gnu/)
 # link_directories(/usr/lib/x86_64-linux-gnu)
  #set(AV_LIBS_LINK "-lavcodec -lavformat -lavutil")
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${AV_LIBS_LINK}")
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lavformat-ffmpeg -lavutil-ffmpeg")
  #set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/usr/lib/x86_64-linux-gnu -lavcode -lavformat -lavutil")
endif()


# Clang specific stuff
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch-enum")
endif()


add_definitions(-std=c++11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/")

add_executable(${NAME} ${SOURCES})

#if(${APPLE})
  target_link_libraries(${NAME} avcodec avformat avutil)
#else()
  # I don't fruicjdsfajlk know
 # target_link_libraries(${NAME} avcodec-ffmpeg avformat-ffmpeg avutil-ffmpeg)
#endif()
