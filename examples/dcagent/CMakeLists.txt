#projects genearte by cmaketools 

#project(dcagent-base CXX)
project(dcagent CXX)
set(dcagent-base_VERSION 0.0.1)

#min version
cmake_minimum_required(VERSION 2.6)

# building must be in not source

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" BUILDING_IN_SOURCE)

if(BUILDING_IN_SOURCE)
	message(FATAL_ERROR "compile dir must not be source dir , please remove 'CMakeCache.txt' in current dir , then create a building dir in which dir you can exec commands like this 'cmake <src dir> [options]' ")
endif()

# compile option
option(DEBUG "Debug mode" 0)
option(PCH "Use precompiled headers" 0)


SET( CMAKE_VERBOSE_MAKEFILE off)

if(DEBUG)
	set(CMAKE_BUILD_TYPE Debug)
	message("Build in debug-mode : Yes (default)")
else()
	set(CMAKE_BUILD_TYPE Release)
	message("Build in debug-mode : No")
endif()

set(CMAKE_COMM_FLAGS "")
#debug common
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_COMM_FLAGS} -g3 -ggdb3 -Wall -Wfatal-errors -Wextra  -rdynamic -ldl -lm -lrt -pthread")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  ${CMAKE_COMM_FLAGS} -g3 -ggdb3 -Wall -Wfatal-errors -Wextra -std=c++11 -rdynamic -ldl -lm -lrt -pthread")

# release mode
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CMAKE_COMM_FLAGS} -O2 -g   -rdynamic -ldl -lm -lrt -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${CMAKE_COMM_FLAGS} -O2 -g -std=c++11  -rdynamic -ldl -lm -lrt -pthread")

# output dir
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# install dir
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})

add_subdirectory(base)
add_subdirectory(utility)


