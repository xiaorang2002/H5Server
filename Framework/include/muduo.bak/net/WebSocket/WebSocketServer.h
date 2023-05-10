// Copyright (c) 2019, Landy
// All rights reserved.

#ifndef MUDUO_WEBSOCKET_SERVER_H
#define MUDUO_WEBSOCKET_SERVER_H 

#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h> 

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>
#include <functional> 

namespace Landy
{

using namespace std;
using namespace muduo;
using namespace muduo::net;



class WebSocketRequest;
class WebSocketResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
/// It is synchronous, just like Java Servlet.
class WebSocketServer : boost::noncopyable
{
 public:
    typedef function<void (const TcpConnectionPtr& conn, string &ip)> WebSocketConnectedCallback;
    typedef function<void (const TcpConnectionPtr& conn)> WebSocketCloseCallback;
    typedef function<void (const TcpConnectionPtr& conn, vector<uint8_t> &buf, Timestamp receiveTime)> WebSocketMessageCallback;

// typedef std::function<void (const TcpConnectionPtr&,Buffer*,Timestamp)> MessageCallback;

  WebSocketServer(EventLoop* loop,
             const InetAddress& listenAddr,
             const string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  ~WebSocketServer();  // force out-line dtor, for scoped_ptr members.

  EventLoop* getLoop() const { return m_TcpServer.getLoop(); }


  void setTcpConnectionCallback(const TcpConnectionCallback& cb)
  { m_TcpConnectionCallback = cb; }

  void setWebSocketConnectedCallback(const WebSocketConnectedCallback& cb)
  { m_WebSocketConnectedCallback = cb; }

  void setWebSocketCloseCallback(const WebSocketCloseCallback& cb)
  { m_WebSocketCloseCallback = cb; }

  void setWebSocketMessageCallback(const WebSocketMessageCallback& cb)
  { m_WebSocketMessageCallback = cb; }


  void setThreadNum(int numThreads)
  {
    m_TcpServer.setThreadNum(numThreads);
  }

  void start();

  bool SendData(const TcpConnectionPtr& conn, vector<uint8_t> &data);
  bool SendData(const TcpConnectionPtr& conn, uint8_t* data, int len);

 private:
  void onTcpConnection(const TcpConnectionPtr& conn);
  bool onHandShake(const TcpConnectionPtr& conn, boost::shared_ptr<muduo::net::Buffer>& buf, Timestamp receiveTime, string &ip);
  void onWebSocketMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime);
//  void onWebSocketConnected(const TcpConnectionPtr& conn);

//  void onWebSocketClose(const TcpConnectionPtr& conn);
//  void onWebSocketHandShake(const TcpConnectionPtr& conn, const WebSocketRequest&, WebSocketResponse*);

public:
  TcpServer m_TcpServer;

private:

  TcpConnectionCallback m_TcpConnectionCallback;
  WebSocketConnectedCallback m_WebSocketConnectedCallback;
  WebSocketCloseCallback m_WebSocketCloseCallback;
  WebSocketMessageCallback m_WebSocketMessageCallback;

};

}


#endif
