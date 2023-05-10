// Copyright (c) 2019, Landy
// All rights reserved.
//

#include "WebSocketServer.h"
#include <muduo/base/Logging.h>

#include "WebSocketContext.h"
#include "WebSocketRequest.h"
#include "WebSocketResponse.h"


using namespace muduo;
using namespace muduo::net;

namespace Landy
{

//void defaultWebSocketCallback(const WebSocketRequest&, WebSocketResponse* resp)
//{
//  resp->setStatusCode(WebSocketResponse::k404NotFound);
//  resp->setStatusMessage("Not Found");
//  resp->setCloseConnection(true);
//}


WebSocketServer::WebSocketServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name,
                       TcpServer::Option option)
  : m_TcpServer(loop, listenAddr, name, option)
//    webSocketCallback_(Landy::defaultWebSocketCallback)
{
  m_TcpServer.setConnectionCallback(
      bind(&WebSocketServer::onTcpConnection, this, placeholders::_1));
  m_TcpServer.setMessageCallback(
      bind(&WebSocketServer::onWebSocketMessage, this, placeholders::_1, placeholders::_2, placeholders::_3));
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::start()
{
  LOG_WARN << "WebSocketServer[" << m_TcpServer.name()
    << "] starts listenning on " << m_TcpServer.ipPort();
  m_TcpServer.start();
}

void WebSocketServer::onTcpConnection(const TcpConnectionPtr& conn)
{
    conn->setWebSocketStatus(TcpConnection::kWebSocketClosed);
    if(m_TcpConnectionCallback)
        m_TcpConnectionCallback(conn);
}

void WebSocketServer::onWebSocketMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    if(!conn->isWebSocketConnected())
    {
        if(buf->readableBytes() > 4)
        {
            const char *crlfcrlf = buf->findCRLFCRLF();
            if(crlfcrlf)
            {
                int len = crlfcrlf-buf->peek()+4;
                boost::shared_ptr<muduo::net::Buffer> buffer(new muduo::net::Buffer(len));
                buffer->append(buf->peek(), static_cast<size_t>(len));
                buf->retrieveAll();

                string ip;
                if(onHandShake(conn, buffer, receiveTime, ip))
                {
                    conn->setWebSocketStatus(TcpConnection::kWebSocketConnected);
                    if(m_WebSocketConnectedCallback)
                        m_WebSocketConnectedCallback(conn, ip);
                }
            }
        }
    }else
    {
        vector<uint8_t> data;
        int decodeDataLen = 0;
        int ret = WCT_BINDATA;
        do
        {
            decodeDataLen = 0;
            ret = WebSocketResponse::WebSocket_dePackage((uint8_t *)buf->peek(), buf->readableBytes(), data, decodeDataLen);
            buf->retrieve(decodeDataLen);

            switch (ret)
            {
            case WCT_DISCONN:  //disconnect
                {
                    conn->setWebSocketStatus(TcpConnection::kWebSocketClosed);
                    if(m_WebSocketCloseCallback)
                        m_WebSocketCloseCallback(conn);
                    vector<uint8_t> sendData;
                    int sendLen = WebSocketResponse::WebSocket_enPackage(&data[0], data.size(), sendData, true, WCT_DISCONN);
                    conn->send(&sendData[0], sendLen);
//                    conn->forceClose();
                }
                break;
            case WCT_BINDATA:  //data
            case WCT_TXTDATA:
                if(m_WebSocketMessageCallback)
                    m_WebSocketMessageCallback(conn, data, receiveTime);
                break;
            case WCT_PING:
                {
                    vector<uint8_t> sendData;
                    int sendLen = WebSocketResponse::WebSocket_enPackage(&data[0], data.size(), sendData, true, WCT_PONG);
                    conn->send(&sendData[0], sendLen);
                }
                break;
            default:
                break;
            }
//            LOG_DEBUG << "ret="<<ret<<" data="<<(char *)data.data();
//            vector<uint8_t> dataToSend;
//            int len = WebSocketResponse::WebSocket_enPackage(data, dataToSend, 0, WCT_TXTDATA);
//            conn->send(&dataToSend[0], len);
        }while(ret != WCT_ERR || decodeDataLen != 0);
    }
}

bool WebSocketServer::SendData(const TcpConnectionPtr& conn, vector<uint8_t> &data)
{
    SendData(conn, &data[0], data.size());
    return true;
}

bool WebSocketServer::SendData(const TcpConnectionPtr& conn, uint8_t* data, int len)
{
    vector<uint8_t> dataToSend;
    int sendLen = WebSocketResponse::WebSocket_enPackage(data, len, dataToSend, false, WCT_BINDATA);
    conn->send(&dataToSend[0], sendLen);
    return true;
}

bool WebSocketServer::onHandShake(const TcpConnectionPtr& conn,
                                  boost::shared_ptr<muduo::net::Buffer> &buf,
                                  Timestamp receiveTime, string &ip)
{
    WebSocketContext context;
    if(!context.parseRequest(buf, receiveTime))
    {
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        conn->shutdown();
        return false;
    }

    if(context.gotAll())
    {
        string response;
        //填充http响应头信息
        response = "HTTP/1.1 101 Switching Protocols\r\n";
        response += "Upgrade: websocket\r\n";
        response += "Connection: Upgrade\r\n";
        response += "Sec-WebSocket-Version: 13\r\n";
//        response += "Sec-WebSocket-Extensions: " + context.request().getHeader("Sec-WebSocket-Extensions") + "\r\n";
//        response += "Sec-WebSocket-Extensions: x-webkit-deflate-frame\r\n";
//        response += "Sec-WebSocket-Extensions: permessage-deflate\r\n";
//        response += "Sec-WebSocket-Extensions: deflate-stream\r\n";
        response += "Sec-WebSocket-Accept: ";
        const string SecWebSocketKey = context.request().getHeader("Sec-WebSocket-Key");
        string serverKey = WebSocketContext::getAcceptKey(SecWebSocketKey);
        response += serverKey + "\r\n";
        response += "Date: " + WebSocketContext::now() + "\r\n\r\n";
        ip = context.request().getHeader("X-Forwarded-For");
        if(ip.empty())
            ip = conn->peerAddress().toIp();
        conn->send(response);
//        if(m_WebSocketConnectedCallback)
//            m_WebSocketConnectedCallback(conn);
        return true;
    }
    return false;
}

}
