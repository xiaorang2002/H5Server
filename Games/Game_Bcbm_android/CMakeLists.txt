#1.cmake verson，指定cmake版本
cmake_minimum_required(VERSION 3.6)

#2.指定项目的名称，一般和项目的文件夹名称对应
PROJECT(Game_bcbm_score_android)	#--------------------------------------------------------------------------此处要修改为对应文件名称（如bcbm Game_bcbm_score_android）

MESSAGE ("======${PROJECT_NAME} build Start======") 

# 开启debug 不然断点调试是无效的，如需去除dubug 只要去掉此行了  -w  -O0 -Wall -g -ggdb")  
# -w 禁止所有警告 http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -m64 -O2 -fmax-errors=100") 
# SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")  # Debug模式下的编译指令
# SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")         # Release模式下的编译指令 

# 编译标准
set(CMAKE_CXX_STANDARD 11)

# 3.项目当前目录
SET(PROJECT_ROOT "${PROJECT_SOURCE_DIR}")			
SET(OUTPUT_BINDIR_DIR "${PROJECT_BINARY_DIR}/../../../bin")			#项目输出目录/bin	  

# 3.1 同级源文件目录
AUX_SOURCE_DIRECTORY("." Game_DIR_SRCS)
# AUX_SOURCE_DIRECTORY("../../proto" PROTO_DIR_SRCS)
# 增加部分文件
SET(PROTO_DIR_SRCS "../../proto/BenCiBaoMa.Message.pb.cc")	#--------------------------------------------此处要修改为对应文件名称（如Xsgj）
SET(PUBLIC_DIR_SRCS "../../public/GlobalFunc.cpp")

# 4. 添加同级目录与子目录头文件
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

# 4.1 添加外部子目录,并输出同级目录文件夹 public
# add_subdirectory(${PROJECT_ROOT}/../../public public_static.out) #输出目录为publicDir.out

#设置库目录,放在这里,必须要放到ADD_EXECUTABLE前面,后续才可使用相对路径
SET(Game_BINARY_DIR "../../Framework/libs" "/usr/lib" "/usr/local/lib") #"/usr/local/lib64/" 

# 4.2 动态链接库或静态链接库的搜索路径，相当于gcc的-L参数,添加需要链接的库文件目录
#必须要放到ADD_EXECUTABLE前面，因为这个命令只对后续命令生效。
LINK_DIRECTORIES(${Game_BINARY_DIR})#增加link库目录

# 5.设置环境变量，编译用到的源文件全部都要放到这里，否则编译能够通过，但是执行的时候会出现各种问题
SET(Game_TEMP_SRCS_PATH ${Game_DIR_SRCS} ${PROTO_DIR_SRCS} ${PUBLIC_DIR_SRCS})

#6.添加要编译的可执行文件
# ADD_EXECUTABLE(${PROJECT_NAME} ${Game_TEMP_SRCS_PATH})

# three kinds of libraries:
# 6.1 添加动态库，关键词为shared，你不需要写全libhello.so，
ADD_LIBRARY (${PROJECT_NAME} SHARED ${Game_TEMP_SRCS_PATH}) #SHARED 动态库so

# 6.2 添加静态库，关键词为static，
# 只需要填写hello即可，cmake系统会自动为你生成 libhello_static.X
# ADD_LIBRARY (${PROJECT_NAME}_static STATIC ${Game_TEMP_SRCS_PATH})#STATIC 静态库.a

# 6.3 设置引用库文件   
SET(LIBRARIES
	snappy boost_locale boost_date_time boost_filesystem boost_system boost_thread boost_regex
	curl ssl crypto snappy -pthread rt m z dl cpr protobuf glog 
)

#7.添加可执行文件所需要的库，比如我们用到了libm.so（命名规则：lib+name+.so），就添加该库的名称，添加链接库，相同于指定-l参数
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBRARIES} libJson.a libmuduo.a)  	#将可执行文件与so_name连接成最终文件demo

# if(IS_DIRECTORY ${OUTPUT_BINDIR_DIR} )
if(NOT OUTPUT_BINDIR_DIR)
	message(FATAL_ERROR "无效目录 'OUTPUT_BINDIR_DIR'=${OUTPUT_BINDIR_DIR}")
else()
	MESSAGE("文件输出目录:" ${OUTPUT_BINDIR_DIR})
endif()

# 8.自定义输出目录Custom output directory
set_target_properties(${PROJECT_NAME}  PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${OUTPUT_BINDIR_DIR})

MESSAGE ("======${PROJECT_NAME} build End======") 
