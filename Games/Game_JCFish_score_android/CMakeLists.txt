cmake_minimum_required(VERSION 2.8)

#Set project info
set(PROJECT_NAME JCFish_score_android)
project(${PROJECT_NAME})

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

#Set compiler info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -shared -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++11 -g -shared -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0" )             # 调试包不优化
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")    # releaseO2优化


IF (CMAKE_SYSTEM_NAME MATCHES "Darwin" AND APPLE)
	set(CMAKE_MACOSX_RPATH 1)
ENDIF ()
#add include library path
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/../include
	${CMAKE_CURRENT_SOURCE_DIR}/../include/acl
	${CMAKE_CURRENT_SOURCE_DIR}/../include/glog_linux
	${CMAKE_CURRENT_SOURCE_DIR}/../NetModule)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib)

#Get all cpp files
file(GLOB CPP_FILES "*.cpp")


SET(LIBRARY_OUTPUT_PATH "../../bin")
#add_library(${PROJECT_NAME} STATIC ${CPP_FILES})
add_library(${PROJECT_NAME} SHARED ${CPP_FILES})

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
	target_link_libraries(${PROJECT_NAME}
			)
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Darwin" AND APPLE)
	target_link_libraries(${PROJECT_NAME}
			libglog.a
			libnetcore.a
			libacl_cpp.a
			libacl.a
			pthread
			libiconv.a
			)
ENDIF ()
