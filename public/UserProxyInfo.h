#ifndef USERPROXYINFO_H
#define USERPROXYINFO_H


#include <muduo/net/EventLoop.h>
#include <muduo/net/Buffer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/ThreadPool.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpClient.h>

#include <muduo/net/http/HttpServer.h>
#include <muduo/net/http/HttpRequest.h>
#include <muduo/net/http/HttpResponse.h>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/function.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>
#include <boost/random.hpp>


using namespace std;
using namespace muduo;
using namespace muduo::net;




namespace Landy
{




typedef boost::weak_ptr<muduo::net::TcpConnection> WeakTcpConnectionPtr;

class UserProxyInfo : public muduo::copyable
{
public:
  explicit UserProxyInfo(const WeakTcpConnectionPtr& weakConn)
    : m_weakConn(weakConn)
  {
        m_aesKey = "";
        m_ip = "";
        m_userId = -1;
  }

  ~UserProxyInfo()
  {
  }

  void setAESKey(string key) { m_aesKey = key; }
  string getAESKey() { return m_aesKey; }

  void setUserId(int32_t userId) { m_userId = userId; }
  int32_t getUserId() { return m_userId; }

  void setIP(string ip) { m_ip = ip; }
  string getIP() { return m_ip; }


  WeakTcpConnectionPtr m_weakConn;

private:
  string               m_aesKey;
  string               m_ip;
  int32_t              m_userId;
};

typedef shared_ptr<UserProxyInfo> UserProxyInfoPtr;
typedef weak_ptr<UserProxyInfo> WeakUserProxyInfoPtr;


}


#endif // USERPROXYINFO_H
