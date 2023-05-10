// Copyright (c) 2019, Landy
// All rights reserved.

#ifndef MUDUO_WEBSOCKET_RESPONSE_H
#define MUDUO_WEBSOCKET_RESPONSE_H

#include <muduo/base/copyable.h>
#include <memory>
#include <vector>
#include <stdint.h>

namespace Landy
{

using namespace std;

typedef enum
{
    WCT_MINDATA = -20,      // 0x0：标识一个中间数据包
    WCT_TXTDATA = -19,      // 0x1：标识一个text类型数据包
    WCT_BINDATA = -18,      // 0x2：标识一个binary类型数据包
    WCT_DISCONN = -17,      // 0x8：标识一个断开连接类型数据包
    WCT_PING = -16,     // 0x8：标识一个断开连接类型数据包
    WCT_PONG = -15,     // 0xA：表示一个pong类型数据包
    WCT_ERR = -1,
    WCT_NULL = 0
}Websocket_CommunicationType;


class WebSocketResponse : public muduo::copyable
{
 public:

    static int WebSocket_enPackage(const uint8_t* data, int dataLen, vector<uint8_t> &package,
                                   bool isMask, Websocket_CommunicationType type);

    static int WebSocket_dePackage(const uint8_t* data, int dataLen, vector<uint8_t> &package, int &decDataLen);


};

}

#endif
