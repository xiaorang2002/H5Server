#https://www.jianshu.com/p/54292d374584
#https://www.cnblogs.com/coffee520/p/10118852.html
#1.cmake verson，指定cmake版本
cmake_minimum_required(VERSION 3.6)

#2.project name，指定项目的名称，一般和项目的文件夹名称对应
PROJECT(zookeeperclient)

MESSAGE ("======zookeeperclient Start======") 

# 开启debug 不然断点调试是无效的，如需去除dubug 只要去掉此行了
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

# Configure Source & Binary Directories ---------------------------------------
SET (PROJECT_ROOT "${PROJECT_SOURCE_DIR}") #项目当前目录
 

#3.添加同级目录与子目录头文件
INCLUDE_DIRECTORIES( 
	"/usr/include"
	"/usr/local/include"
	"/usr/local/include/zookeeper"
	)

#4.查找同级源文件目录,并将名称保存到 DIR_SRCS 变量
AUX_SOURCE_DIRECTORY(. DIR_ZK_SRCS)
FOREACH(F ${DIR_ZK_SRCS})
    MESSAGE(${F})
ENDFOREACH(F)



# 添加同级子目录（如有同级其它目录则开放）
# add_subdirectory(hello)
 
# 添加外部子目录,并输出同级目录文件夹test.out （如有其它依赖则开放）
# add_subdirectory(${PROJECT_SOURCE_DIR}/../../test test.out)

#必须要放到ADD_EXECUTABLE前面，因为这个命令只对后续命令生效。
SET(ZK_BIN_DIR  "../libs" "/usr/lib" "/usr/local/lib")
LINK_DIRECTORIES(${ZK_BIN_DIR})#增加redis为link目录

# 输出库
add_library(${PROJECT_NAME} ${DIR_ZK_SRCS})

# 连接依赖（如有其它依赖则开放）hiredis-vip
# target_link_libraries(${PROJECT_NAME})
 
MESSAGE ("======zookeeperclient End======") 