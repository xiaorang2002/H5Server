// Copyright (c) 2019, Landy
// All rights reserved.


#include <stdio.h>
#include <cstring>

#include "WebSocketResponse.h"


namespace Landy
{

/*******************************************************************************
 * 名称: webSocket_getRandomString
 * 功能: 生成随机字符串
 * 形参: *buf：随机字符串存储到
 *              len : 生成随机字符串长度
 * 返回: 无
 * 说明: 无
 ******************************************************************************/
void webSocket_getRandomString(unsigned char *buf, unsigned int len)
{
    unsigned int i;
    unsigned char temp;
    srand((int)time(0));
    for(i = 0; i < len; i++)
    {
        temp = (unsigned char)(rand()%256);
        if(temp == 0)   // 随机数不要0, 0 会干扰对字符串长度的判断
            temp = 128;
        buf[i] = temp;
    }
}


/*******************************************************************************
 * 名称: webSocket_enPackage
 * 功能: websocket数据收发阶段的数据打包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
 * 形参: *data：准备发出的数据
 *          dataLen : 长度
 *        *package : 打包后存储地址
 *        packageMaxLen : 存储地址可用长度
 *          isMask : 是否使用掩码     1要   0 不要
 *          type : 数据类型, 由打包后第一个字节决定, 这里默认是数据传输, 即0x81
 * 返回: 打包后的长度(会比原数据长2~16个字节不等)      <=0 打包失败
 * 说明: 无
 ******************************************************************************/
int WebSocketResponse::WebSocket_enPackage(const uint8_t* data, int dataLen, vector<uint8_t> &package,
                                                 bool isMask, Websocket_CommunicationType type)
{
    unsigned char maskKey[4] = {0};    // 掩码
    unsigned char temp1, temp2;
    int count;
    unsigned int i = 0, len = 0;

    package.resize(dataLen + 64);

    if(type == WCT_MINDATA)
        package[i++] = 0x00;
    else if(type == WCT_TXTDATA)
        package[i++] = 0x81;
    else if(type == WCT_BINDATA)
        package[i++] = 0x82;
    else if(type == WCT_DISCONN)
        package[i++] = 0x88;
    else if(type == WCT_PING)
        package[i++] = 0x89;
    else if(type == WCT_PONG)
        package[i++] = 0x8A;
    else
        return -1;

    // LOG_INFO << " >>> WebSocket_enPackage,isMask[" << isMask<<"]";
   
    len += 1;
    //
    if(isMask){
        package[i++] = 0x80;
        len += 1;
    }
    else{
        package[i] = 0;
    }
    
    // len += 1;
    //
    if(dataLen < 126)
    {
        package[i++] |= (dataLen&0x7F);
        len += 1;
    }else if(dataLen < 65536)
    {
        package[i++] |= 0x7E;
        package[i++] = (char)((dataLen >> 8) & 0xFF);
        package[i++] = (unsigned char)((dataLen >> 0) & 0xFF);
        len += 3;
    }else if(dataLen < 0xFFFFFFFF)
    {
        package[i++] |= 0x7F;
        package[i++] = 0; //(char)((dataLen >> 56) & 0xFF);   // 数据长度变量是 unsigned int dataLen, 暂时没有那么多数据
        package[i++] = 0; //(char)((dataLen >> 48) & 0xFF);
        package[i++] = 0; //(char)((dataLen >> 40) & 0xFF);
        package[i++] = 0; //(char)((dataLen >> 32) & 0xFF);
        package[i++] = (char)((dataLen >> 24) & 0xFF);        // 到这里就够传4GB数据了
        package[i++] = (char)((dataLen >> 16) & 0xFF);
        package[i++] = (char)((dataLen >> 8) & 0xFF);
        package[i++] = (char)((dataLen >> 0) & 0xFF);
        len += 9;
    }
    //
    if(isMask)    // 数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下
    {
        webSocket_getRandomString(maskKey, sizeof(maskKey));    // 随机生成掩码
        package[i++] = maskKey[0];
        package[i++] = maskKey[1];
        package[i++] = maskKey[2];
        package[i++] = maskKey[3];
        len += 4;
        for(i = 0, count = 0; i < dataLen; i++)
        {
            temp1 = maskKey[count];
            temp2 = data[i];
            package[i++] = (char)(((~temp1)&temp2) | (temp1&(~temp2)));  // 异或运算后得到数据
            count += 1;
            if(count >= sizeof(maskKey))    // maskKey[4]循环使用
                count = 0;
        }
        len += i;
    }
    else    // 数据没使用掩码, 直接复制数据段
    {
        memcpy(&package[i], data, dataLen);
        len += dataLen;
    }
    package.resize(len);
    //
    return len;
}

/*******************************************************************************
 * 名称: webSocket_dePackage
 * 功能: websocket数据收发阶段的数据解包, 通常client发server的数据都要isMask(掩码)处理, 反之server到client却不用
 * 形参: *data：解包的数据
 *          dataLen : 长度
 *        *package : 解包后存储地址
 *        packageMaxLen : 存储地址可用长度
 *        *packageLen : 解包所得长度
 * 返回: 解包识别的数据类型 如 : txt数据, bin数据, ping, pong等
 * 说明: 无
 ******************************************************************************/
int WebSocketResponse::WebSocket_dePackage(const uint8_t* data, int dataLen, vector<uint8_t> &package, int &decodeDataLen)
{
    unsigned char maskKey[4] = {0};    // 掩码
    unsigned char temp1, temp2;
    char Mask = 0, type;
    int count, ret = WCT_ERR;
    unsigned int len = 0, dataStart = 2;
    if(dataLen < 2)
        return WCT_ERR;

    decodeDataLen = 0;

    type = data[0]&0x0F;

    if((data[0]&0x80) == 0x80)
    {
        if(type == 0x01)
            ret = WCT_TXTDATA;
        else if(type == 0x02)
            ret = WCT_BINDATA;
        else if(type == 0x08)
            ret = WCT_DISCONN;
        else if(type == 0x09)
            ret = WCT_PING;
        else if(type == 0x0A)
            ret = WCT_PONG;
        else
            return WCT_ERR;
    }
    else if(type == 0x00)
        ret = WCT_MINDATA;
    else
        return WCT_ERR;
    //
    if((data[1] & 0x80) == 0x80)
    {
        Mask = 1;
        count = 4;
    }
    else
    {
        Mask = 0;
        count = 0;
    }
    //
    len = data[1] & 0x7F;
    //
    if(len == 126)
    {
        if(dataLen < 4)
            return WCT_ERR;
        len = data[2];
        len = (len << 8) + data[3];
        if(dataLen < len + 4 + count)
            return WCT_ERR;
        if(Mask)
        {
            maskKey[0] = data[4];
            maskKey[1] = data[5];
            maskKey[2] = data[6];
            maskKey[3] = data[7];
            dataStart = 8;
        }
        else
            dataStart = 4;
    }
    else if(len == 127)
    {
        if(dataLen < 10)
            return WCT_ERR;
        if(data[2] != 0 || data[3] != 0 || data[4] != 0 || data[5] != 0)    // 使用8个字节存储长度时, 前4位必须为0, 装不下那么多数据...
            return WCT_ERR;
        len = data[6];
        len = (len << 8) + data[7];
        len = (len << 8) + data[8];
        len = (len << 8) + data[9];
        if(dataLen < len + 10 + count)
            return WCT_ERR;
        if(Mask)
        {
            maskKey[0] = data[10];
            maskKey[1] = data[11];
            maskKey[2] = data[12];
            maskKey[3] = data[13];
            dataStart = 14;
        }
        else
            dataStart = 10;
    }
    else
    {
        if(dataLen < len + 2 + count)
            return WCT_ERR;
        if(Mask)
        {
            maskKey[0] = data[2];
            maskKey[1] = data[3];
            maskKey[2] = data[4];
            maskKey[3] = data[5];
            dataStart = 6;
        }
        else
            dataStart = 2;
    }
    //
    if(dataLen < len + dataStart)
        return WCT_ERR;

    //
    package.resize(len);
    if(Mask)    // 解包数据使用掩码时, 使用异或解码, maskKey[4]依次和数据异或运算, 逻辑如下
    {
        //printf("depackage : len/%d\r\n", len);
        int n = 0;
        for(int i = 0, count = 0; i < len; i++)
        {
            temp1 = maskKey[count];
            temp2 = data[i + dataStart];
            package[n++] =  (char)(((~temp1)&temp2) | (temp1&(~temp2)));  // 异或运算后得到数据
            count += 1;
            if(count >= sizeof(maskKey))    // maskKey[4]循环使用
                count = 0;
        }
    }
    else    // 解包数据没使用掩码, 直接复制数据段
    {
        memcpy(&package[0], &data[dataStart], len);
    }

    decodeDataLen = dataStart + len;
    //
    return ret;
}


}
