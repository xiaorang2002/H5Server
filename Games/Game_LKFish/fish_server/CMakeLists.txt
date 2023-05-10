cmake_minimum_required(VERSION 2.8)

# set the project name content now.
set(PROJECT_NAME fish_server)

project(${PROJECT_NAME})

# set debug type.
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "set build type to release!!!!!!" FORCE)

#Set compiler info debug add -g 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -std=c++11 -s -Wall -pipe -fno-ident -fPIC -MMD -D_GNU_SOURCE -D_REENTRANT -D__LKPY__")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -std=c++11 -s -Wall -pipe -fno-ident -fPIC -MMD -D_GNU_SOURCE -D_REENTRANT -D__LKPY__")

# dump the message to build type value item now.
message(STATUS, "set build type to LKPY!...")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0 -DDEBUG" )	# 调试包不优化 -O0
set(CMAKE_CXX_FLAGS_RELEASE "-s -O0 -DNDEBUG")	# release包优化


# message debug type.
message(STATUS "build type:${CMAKE_BUILD_TYPE} cxx_flags:${CMAKE_CXX_FLAGS} cflags:${CMAKE_C_FLAGS}")

# print the project message.
message(STATUS "Project Name:${PROJECT_NAME}\r\n")

# get all cpp files content.
file(GLOB CPP_FILES "*.cpp")

#print all the file name
foreach(SOURCE ${CPP_FILES})
	string(REPLACE ${PROJECT_SOURCE_DIR} "." CPP_FILE ${SOURCE})
	message(${CPP_FILE})
endforeach()

# add the include dir.
include_directories(
	/usr/local/include
	/usr/local/include/lsm/
	../../../thirdpart/muduo/
	./Globals
    ../netcore
	../inc
)

# add lib link dir.
link_directories(
	${PROJECT_SOURCE_DIR}/../../lib
	/usr/local/include/lsm/
	/usr/local/include/lsm/iocp/
	/usr/local/include/lsm/vmprotect/
	/usr/local/include/lsm/des/
	/usr/local/include/lsm/xml/
)


# set the execute output data file path.
set(LIBRARY_OUTPUT_PATH "../../build")

# build the special execute file item value now.
add_library(${PROJECT_NAME} SHARED ${CPP_FILES})

message(STATUS "build type:${CMAKE_BUILD_TYPE} cxxflags:${CMAKE_CXX_FLAGS}")

target_link_libraries(${PROJECT_NAME}
	IOCPEvent
	acl_cpp
	acl
	3des8bit
	VMProtectSDK64
	crypto
	pthread
	tinyxml
	dl
)


