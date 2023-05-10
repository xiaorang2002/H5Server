/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/

 
/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/
/***************************************************************/


#ifndef FUN_CLASS_H
#define FUN_CLASS_H 

#include <map>
#include <string>
#include <list>
#include <deque>
#include <vector>
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <iostream>
#include <math.h>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>
#include <sstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <ctype.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>
#include <cpr/cpr.h>

#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>
#include <boost/thread.hpp>
#include <boost/regex.hpp>

#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"
#include "muduo/net/EventLoopThreadPool.h"
#include "muduo/net/Buffer.h"
#include "muduo/net/InetAddress.h" 
#include "muduo/net/http/HttpContext.h"
#include "muduo/net/http/HttpRequest.h"
#include "muduo/net/http/HttpResponse.h"
#include "muduo/net/TimerId.h" 
#include "muduo/base/AsyncLogging.h"
#include "muduo/base/TimeZone.h" 
#include "muduo/base/Mutex.h"
#include "muduo/base/ThreadLocal.h"
#include "muduo/base/ThreadPool.h"
#include "muduo/base/ThreadLocalSingleton.h"
#include "muduo/base/Logging.h"

#include "zookeeperclient/zookeeperclient.h"
// #include "RedisClient/RedisClient.h"
// #include "RedisClient/redlock.h"

#include "./public/ErrorCodeSet.h"
#include "./public/cryptopublic.h"
#include "./public/urlcodec.h"
 
#include "RegisteServer.h"

using namespace std;
using namespace muduo;

using namespace boost::gregorian;
using namespace boost::posix_time;
using namespace boost::local_time; 
using namespace bsoncxx::types;

using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;

class CFunClass
{
   public:
    CFunClass(RegisteServer* loginserver);  // 这是构造函数

	void startInit();
 
private:
	// 初始化Redis
	bool InitRedisCluster(std::string ipaddr, std::string password);
	// 初始化数据库
	bool InitMongoDB(std::string url);
	// 加载配置
	bool LoadInitConfig();

	int64_t nLastTickTime =0;
	int64_t nLastnReqCount =0;
	int32_t nLogLevelCopy =0;
	// 
	int64_t nSecondTick_ = 0;

	bool isStartServer;   
	bool isdecrypt_;   
	bool m_bInvaildServer;    
 
};
  

#endif // FUN_CLASS_H
