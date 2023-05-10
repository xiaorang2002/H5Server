
cmake_minimum_required(VERSION 3.6)

PROJECT(tony)

# set(SOURCE ./main.cpp)
#4.source directory，源文件目录
AUX_SOURCE_DIRECTORY(. DIR_TONY_SRCS) 
FOREACH(F ${DIR_TONY_SRCS})
    MESSAGE(${F})
ENDFOREACH(F)

# ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_TONY_SRCS})
add_library(${PROJECT_NAME} ${DIR_TONY_SRCS})