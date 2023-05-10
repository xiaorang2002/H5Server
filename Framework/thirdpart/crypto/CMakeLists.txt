cmake_minimum_required(VERSION 2.8)

# set the project name now.
set(PROJECT_NAME gameCrypto)

project(${PROJECT_NAME})

#Set compiler info
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -std=c++11 -g -Wall -pipe -fno-ident -MMD -D_GNU_SOURCE -D_REENTRANT")

set(CMAKE_CXX_FLAGS_DEBUG   "-O0" )             # 调试包不优化
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG ")    # release包优化

# print the project message.
message(STATUS "Project Name:${PROJECT_NAME}\r\n")

# get all cpp files.
file(GLOB CPP_FILES "*.cpp")

#print all the file name
foreach(SOURCE ${CPP_FILES})
	string(REPLACE ${PROJECT_SOURCE_DIR} "." CPP_FILE ${SOURCE})
	message(${CPP_FILE})
endforeach()

# add the include dir.
include_directories(
	../inc
)

link_directories(
	${PROJECT_SOURCE_DIR}/../lib
)

# set the output path.
set(LIBRARY_OUTPUT_PATH "../../lib")

# build the special static library item.
add_library(${PROJECT_NAME} STATIC ${CPP_FILES})

target_link_libraries(${PROJECT_NAME}
	crypto
)


