project(ttear)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

find_package(SDL REQUIRED)
if(NOT SDL_FOUND)
    message(FATAL_ERROR "SDL not found")
endif(NOT SDL_FOUND)

include_directories(${SDL_INCLUDE_DIR} ./include)

CHECK_INCLUDE_FILE("sys/time.h" HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE("getopt.h" HAVE_GETOPT_H)
CHECK_INCLUDE_FILE("time.h" HAVE_TIME_H)
CHECK_FUNCTION_EXISTS("bzero" HAVE_BZERO)
CHECK_FUNCTION_EXISTS("getopt_long" HAVE_GETOPT_LONG)
CHECK_FUNCTION_EXISTS("gettimeofday" HAVE_GETTIMEOFDAY)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/config.h ${CMAKE_CURRENT_BINARY_DIR}/include/config.h)

add_executable(ttear chars.cpp cpu.cpp extstorage.cpp framebuffer.cpp joysticks.cpp keyboard.cpp main.cpp opengl_framebuffer.cpp options.cpp rom.cpp software_framebuffer.cpp sprites.cpp vdc.cpp vmachine.cpp include/chars.h include/colors.h include/common.h include/cpu.h include/extstorage.h include/framebuffer.h include/iniparser.h include/joysticks.h include/keyboard.h include/opcodes.h include/opengl_framebuffer.h include/options.h include/rom.h include/software_framebuffer.h include/speedlimit.h include/sprites.h include/util.h include/vdc.h include/vmachine.h)
target_link_libraries(ttear ${SDL_LIBRARY})
