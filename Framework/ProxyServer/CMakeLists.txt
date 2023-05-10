#https://www.jianshu.com/p/54292d374584
#https://www.cnblogs.com/coffee520/p/10118852.html

#1.cmake verson，指定cmake版本
cmake_minimum_required(VERSION 3.6)

#2.指定项目的名称，一般和项目的文件夹名称对应
PROJECT(ProxyServer)

MESSAGE ("======ProxyServer build Start======") 

# 开启debug 不然断点调试是无效的，如需去除dubug 只要去掉此行了   -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wunused -m64 -O2")
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")         # Release模式下的编译指令 

# 编译标准
set(CMAKE_CXX_STANDARD 11)

#项目当前目录
SET(PROJECT_ROOT "${PROJECT_SOURCE_DIR}")			
SET(OUTPUT_BINDIR_DIR "${PROJECT_BINARY_DIR}/../../../bin")			#项目输出目录/bin	 
# SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_SOURCE_DIR}/bin)    
# SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${PROJECT_SOURCE_DIR}/lib) 
# SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${OUTPUT_BINDIR_DIR})   
# SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${OUTPUT_BINDIR_DIR})  

MESSAGE("当前目录" ${PROJECT_ROOT})
MESSAGE("输出目录" ${OUTPUT_BINDIR_DIR})
MESSAGE("BIN目录" ${PROJECT_BINARY_DIR})

#同级源文件目录
AUX_SOURCE_DIRECTORY("." Proxy_DIR_SRCS)
AUX_SOURCE_DIRECTORY("../../proto" PROTO_DIR_SRCS)

# 添加同级目录与子目录头文件
INCLUDE_DIRECTORIES(
	"./"
	"../../"
	"../../proto" 
	"../../public"  
	"../../public/RedisClient" 
	"../../thirdpart"
	"../include/Json/include"
	"../include"
	"../include/hiredis-vip"
	"./include"
	"../include"
	"../include/zookeeper"
	"../include/zookeeperclient"
	"/usr/local/include" 
	"/usr/include/mysql-cppconn-8"
	"/usr/local/include/google/protobuf"
)

# # 添加外部子目录,并输出同级目录文件夹
# # 增加RedisClient静态库
# add_subdirectory(${PROJECT_ROOT}/../../public/RedisClient redis.out) #redis.out
# # 增加zookeeperclient静态库
# add_subdirectory(${PROJECT_ROOT}/../../public/zookeeperclient zk.out) #zk.out
# 增加public公共目录下静态库
add_subdirectory(${PROJECT_ROOT}/../../public public_static.out) #输出目录为publicDir.out
# add_subdirectory(examples EXCLUDE_FROM_ALL examples/venv)

#设置库目录,放在这里,必须要放到ADD_EXECUTABLE前面,后续才可使用相对路径
SET(Proxy_BINARY_DIR "../libs" "/usr/lib" "/usr/local/lib") #"/usr/local/lib64/" 

#动态链接库或静态链接库的搜索路径，相当于gcc的-L参数,添加需要链接的库文件目录
#必须要放到ADD_EXECUTABLE前面，因为这个命令只对后续命令生效。
LINK_DIRECTORIES(${Proxy_BINARY_DIR})#增加link库目录

#打印目录内容
FOREACH(F ${Proxy_BINARY_DIR})
    MESSAGE(${F})
ENDFOREACH(F)

#5.set environment variable，设置环境变量，编译用到的源文件全部都要放到这里，否则编译能够通过，但是执行的时候会出现各种问题，比如"symbol lookup error xxxxx , undefined symbol"
SET(Proxy_TEMP_PATH ${Proxy_DIR_SRCS} ${PROTO_DIR_SRCS} )
#打印SRCS内容
FOREACH(F ${Proxy_TEMP_PATH})
    MESSAGE(${F})
ENDFOREACH(F)

#6.添加要编译的可执行文件
ADD_EXECUTABLE(${PROJECT_NAME} ${Proxy_TEMP_PATH})

#设置引用库文件   
SET(LIBRARIES
	boost_locale boost_date_time boost_filesystem boost_system boost_thread boost_regex
	curl ssl crypto snappy -pthread rt m z dl cpr protobuf json #zookeeperclient zookeeper_mt
	hiredis-vip muduo  mongocxx bsoncxx mongoc-1.0 public_static #RedisClient
)

#7.添加可执行文件所需要的库，比如我们用到了libm.so（命名规则：lib+name+.so），就添加该库的名称，添加链接库，相同于指定-l参数
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES} libzookeeper_mt.a libgameCrypto.a)  	#将可执行文件与so_name连接成最终文件demo

# 8.自定义输出目录Custom output directory

# SET_TARGET_PROPERTIES(utility  PROPERTIES    #表示生成的执行文件所在路径
# RUNTIME_OUTPUT_DIRECTORY> "${PROJECT_SOURCE_DIR}/lib")
# set_target_properties(rviz_image_view
#                       PROPERTIES OUTPUT_NAME image_view
#                       PREFIX "")

# if(IS_DIRECTORY ${OUTPUT_BINDIR_DIR} )
if(NOT OUTPUT_BINDIR_DIR)
	message(FATAL_ERROR "无效目录 'OUTPUT_BINDIR_DIR'=${OUTPUT_BINDIR_DIR}")
else()
	MESSAGE("文件输出目录:"${OUTPUT_BINDIR_DIR})
endif()

# Custom output directory
set_target_properties(${PROJECT_NAME} #python_module_library
  PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINDIR_DIR})#${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_PYTHON_DESTINATION})


# 9.安装目录
# install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${OUTPUT_BINDIR_DIR})

# install(TARGETS ${PROJECT_NAME}
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
# )


MESSAGE ("======ProxyServer build End======") 
