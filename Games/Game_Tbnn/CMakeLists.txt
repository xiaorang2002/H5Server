#https://www.jianshu.com/p/54292d374584
#https://www.cnblogs.com/coffee520/p/10118852.html

#1.cmake verson，指定cmake版本
cmake_minimum_required(VERSION 3.6)

#2.指定项目的名称，一般和项目的文件夹名称对应
PROJECT(Game_tbnn)

MESSAGE ("======Game_tbnn build Start======") 

# 开启debug 不然断点调试是无效的，如需去除dubug 只要去掉此行了  -w  -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
# -w 禁止所有警告 http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m64 -O2 -fmax-errors=100") 
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
MESSAGE("SO输出目录" ${PROJECT_BINARY_DIR})

#同级源文件目录
AUX_SOURCE_DIRECTORY("." Game_DIR_SRCS)
# AUX_SOURCE_DIRECTORY("../../proto" PROTO_DIR_SRCS)
# 增加部分文件
SET(PROTO_DIR_SRCS 
"../../proto/Tbnn.Message.pb.cc" 
# "../../proto/Common.Message.pb.cc" 
# "../../proto/Game.Common.pb.cc" 
# "../../proto/GameServer.Message.pb.cc" 
# "../../proto/HallServer.Message.pb.cc" 
)

SET(PUBLIC_DIR_SRCS 
"../../public/GlobalFunc.cpp" 
)

# 添加同级目录与子目录头文件
INCLUDE_DIRECTORIES(
	"./"
	"../../"
	"../../proto" 
	"../../public"  
	"../../Framework"
	"../../Framework/include"
	"../../Framework/GameServer"
	"../../Framework/include/Json/include"
	"/usr/local/include" 
	"/usr/local/include/google/protobuf"
)

# # 添加外部子目录,并输出同级目录文件夹
# # 增加RedisClient静态库
# add_subdirectory(${PROJECT_ROOT}/../../public/RedisClient redis.out) #redis.out
# # 增加zookeeperclient静态库
# add_subdirectory(${PROJECT_ROOT}/../../public/zookeeperclient zk.out) #zk.out
# 增加public公共目录下静态库
# add_subdirectory(${PROJECT_ROOT}/../../public public_static.out) #输出目录为publicDir.out
# add_subdirectory(examples EXCLUDE_FROM_ALL examples/venv)

#设置库目录,放在这里,必须要放到ADD_EXECUTABLE前面,后续才可使用相对路径
SET(Game_BINARY_DIR "../../Framework/libs" "/usr/lib" "/usr/local/lib") #"/usr/local/lib64/" 

#动态链接库或静态链接库的搜索路径，相当于gcc的-L参数,添加需要链接的库文件目录
#必须要放到ADD_EXECUTABLE前面，因为这个命令只对后续命令生效。
LINK_DIRECTORIES(${Game_BINARY_DIR})#增加link库目录

#打印目录内容
FOREACH(F ${Game_BINARY_DIR})
    MESSAGE(${F})
ENDFOREACH(F)

#5.set environment variable，设置环境变量，编译用到的源文件全部都要放到这里，否则编译能够通过，但是执行的时候会出现各种问题
SET(Game_TEMP_SRCS_PATH ${Game_DIR_SRCS} ${PROTO_DIR_SRCS} ${PUBLIC_DIR_SRCS})
#打印SRCS内容
FOREACH(F ${Game_TEMP_SRCS_PATH})
    MESSAGE(${F})
ENDFOREACH(F)

#6.添加要编译的可执行文件
# ADD_EXECUTABLE(${PROJECT_NAME} ${Game_TEMP_SRCS_PATH})

# three kinds of libraries:
# 1. shared, i.e., dynamic library，动态库文件扩展名常为 "*.so"；
# 2. static, i.e., static library， 静态库文件扩展名常为 "*.a";
# 而且，我们通常希望静态库和动态库文件名保持一致，只是扩展名不同；
# 3. module, this parameter is valid only when dyld is supported;
# otherwise, will be considered as shared
# 添加动态库，关键词为shared，你不需要写全libhello.so，
# 只需要填写hello即可，cmake系统会自动为你生成 libhello.X
ADD_LIBRARY (${PROJECT_NAME} SHARED ${Game_TEMP_SRCS_PATH}) #SHARED 动态库so

# 添加静态库，关键词为static，
# ADD_LIBRARY (hello STATIC ${LIBHELLO_SRC})
# 仍然用hello作为target名时，是不能成功创建所需的静态库的，
# 因为hello作为一个target是不能重名的， 故把上面的hello修改为hello_static
# 同理，你不需要写全libhello_static.a
# 只需要填写hello即可，cmake系统会自动为你生成 libhello_static.X
# ADD_LIBRARY (${PROJECT_NAME}_static STATIC ${Game_TEMP_SRCS_PATH})#STATIC 静态库.a

# 按照一般的习惯，静态库名字跟动态库名字应该是一致的，只是扩展名不同；
# 即：静态库名为 libhello.a； 动态库名为libhello.so ；
# 所以，希望 "hello_static" 在输出时，不是"hello_static"，而是以"hello"的名字显示，故设置如下：
# SET_TARGET_PROPERTIES ("${PROJECT_NAME}_static" PROPERTIES OUTPUT_NAME ${PROJECT_NAME})

# GET_TARGET_PROPERTY (OUTPUT_VALUE "${PROJECT_NAME}_static" OUTPUT_NAME)
# MESSAGE (STATUS "This is the hello_static OUTPUT_NAME: " ${OUTPUT_VALUE})

# 按照规则，动态库是应该包含一个版本号的，
# VERSION指代动态库版本，SOVERSION指代API版本。
# SET_TARGET_PROPERTIES (${PROJECT_NAME} PROPERTIES VERSION 1.1 SOVERSION 1.0)
 
#设置引用库文件   
SET(LIBRARIES
	snappy boost_locale boost_date_time boost_filesystem boost_system boost_thread boost_regex
	curl ssl crypto snappy -pthread rt m z dl cpr protobuf glog 
)

#7.添加可执行文件所需要的库，比如我们用到了libm.so（命名规则：lib+name+.so），就添加该库的名称，添加链接库，相同于指定-l参数
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES} libJson.a libmuduo.a)  	#将可执行文件与so_name连接成最终文件demo

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
set_target_properties(${PROJECT_NAME}  PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${OUTPUT_BINDIR_DIR})
set_target_properties(${PROJECT_NAME}  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR_DIR})
#   PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${OUTPUT_BINDIR_DIR})#${CATKIN_DEVEL_PREFIX}/${CATKIN_PACKAGE_PYTHON_DESTINATION})

# 9.安装目录
# install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${OUTPUT_BINDIR_DIR})

# install(TARGETS ${PROJECT_NAME}
#   ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
#   RUNTIME DESTINATION ${CATKIN_GLOBAL_BIN_DESTINATION}
# )

MESSAGE ("======Game_tbnn build End======") 
