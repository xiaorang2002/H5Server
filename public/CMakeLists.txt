
cmake_minimum_required(VERSION 3.6)

PROJECT(public_static)


# 编译标准
set(CMAKE_CXX_STANDARD 11)

#3.添加同级目录与子目录头文件
INCLUDE_DIRECTORIES(
	"./"
	"../"
	"./RedisClient"
	"./zookeeperclient"
    "../Framework/include"
	# "/usr/include"
    # "/usr/local/include"
)
    
# 添加同级子目录
# 增加RedisClient静态库
# add_subdirectory(RedisClient) #redis.out ${PROJECT_SOURCE_DIR}/RedisClient
# 增加zookeeperclient静态库
# add_subdirectory(zookeeperclient) #zk.out
# 添加外部子目录,并输出同级目录文件夹
# add_subdirectory(hello hello.out)
# add_subdirectory(world world.out) 

# 添加源文件方法一
# set(SOURCE ./main.cpp)
#4.source directory，源文件目录
#AUX_SOURCE_DIRECTORY(. DIR_PUBLIC_SRCS) 
AUX_SOURCE_DIRECTORY(./RedisClient DIR_PUBLIC_REDIS_SRCS) 
AUX_SOURCE_DIRECTORY(./zookeeperclient DIR_PUBLIC_ZK_SRCS) 
AUX_SOURCE_DIRECTORY(./TraceMsg DIR_PUBLIC_Trace_SRCS) 
AUX_SOURCE_DIRECTORY(. DIR_PUBLIC_SRCS) 



# FOREACH(F ${DIR_PUBLIC_REDIS_SRCS})
#     MESSAGE(${F})
# ENDFOREACH(F)

# FOREACH(F ${DIR_PUBLIC_ZK_SRCS})
#     MESSAGE(${F})
# ENDFOREACH(F)

# 添加源文件方法二
# set(DIR_PUBLIC_SRCS 
#     # ./RedisClient/RedisClient.cpp
#     # ./RedisClient/RedLock.cpp
#     # ./zookeeperclient/zookeeperclient.cpp
#     # ./zookeeperclient/zookeeperclientutils.cpp
#     # ./zookeeperclient/zookeeperlocker.cpp
#     # ./zookeeperclient/zoo_lock.c
#     GlobalFunc.cpp 
#     GameGlobalFunc.cpp 
#     IPFinder.cpp 
#     MyEventLoopThreadPool.cc 
#     TaskService.cpp 
#     ThreadLocalSingletonMongoDBClient.cpp 
#     base64.cpp 
#     crypto.cpp 
#     mymd5.cpp 
#     urlcodec.cpp 
#     weights.cpp
# )

# FOREACH(F ${DIR_PUBLIC_SRCS})
#     MESSAGE(${F})
# ENDFOREACH(F)

#5.set environment variable，设置环境变量，编译用到的源文件全部都要放到这里，否则编译能够通过，但是执行的时候会出现各种问题，比如"symbol lookup error xxxxx , undefined symbol"
SET(PUBLIC_TEMP_PATH ${DIR_PUBLIC_SRCS} ${DIR_PUBLIC_REDIS_SRCS} ${DIR_PUBLIC_ZK_SRCS} ${DIR_PUBLIC_Trace_SRCS})
MESSAGE("PUBLIC_TEMP_PATH目录")
FOREACH(F ${PUBLIC_TEMP_PATH})
    MESSAGE(${F})
ENDFOREACH(F)

# ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})
add_library(${PROJECT_NAME} ${PUBLIC_TEMP_PATH})
 
# 连接依赖（如有其它依赖则开放）crypto ssl
target_link_libraries(${PROJECT_NAME} cpr hiredis-vip) # RedisClient zookeeperclient
 


