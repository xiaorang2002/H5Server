// Copyright (c) 2019, Landy
// All rights reserved.

#pragma once


#include <stdint.h>
#include <iostream>
#include <string>
#include <deque>
#include <map>

#include <boost/thread.hpp>


#define likely(x)                   __builtin_expect(!!(x), 1)
#define unlikely(x)                 __builtin_expect(!!(x), 0)


#define READ_LOCK(mutex)            boost::shared_lock<boost::shared_mutex> guard(mutex)
#define WRITE_LOCK(mutex)           boost::lock_guard<boost::shared_mutex> guard(mutex)

using namespace std;

// 定义宏 add by caiqing
#define CALLBACK_0(__selector__,__target__, ...) std::bind(&__selector__,__target__, ##__VA_ARGS__)
#define CALLBACK_1(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, ##__VA_ARGS__)
#define CALLBACK_2(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, ##__VA_ARGS__)
#define CALLBACK_3(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 ##__VA_ARGS__)
#define CALLBACK_4(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4 ##__VA_ARGS__)
#define CALLBACK_5(__selector__,__target__, ...) std::bind(&__selector__,__target__, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5 ##__VA_ARGS__)



#pragma pack(1)

enum pub_enctype
{
    PUBENC__JSON_NONE = 0x01,
    PUBENC__PROTOBUF_NONE = 0x02,
    PUBENC__JSON_BIT_MASK = 0x11,
    PUBENC__PROTOBUF_BIT_MASK = 0x12,
    PUBENC__JSON_RSA = 0x21,
    PUBENC__PROTOBUF_RSA = 0x22,
    PUBENC__JSON_AES = 0x31,
    PUBENC__PROTOBUF_AES = 0x32
};

//// pub_header must = prev_header.
//struct pub_header
//{
//    uint16_t packsize;    // total package size.
//    uint16_t enctype;     // encrypt type,0:none,1:rsa,2:aes
//    uint16_t realsize;    // real size of decrypted package.
//};

// size =  18.
struct Header
{
    uint16_t len;         // total package size.
    uint16_t crc;
    uint16_t ver;
    uint16_t sign;
    uint8_t  mainId;
    uint8_t  subId;
    uint8_t  encType;
    uint8_t  reserve;
    uint32_t reqId;
    uint16_t realSize;    // real size of decrypted package.
};

#define SESSION_SIZE  32
#define AESKEY_SIZE   16
// size = 2 + 2 + 4 + 4 + 32 + 16 = 60.
struct internal_prev_header
{
    uint16_t len;
    int16_t  bKicking;
    int32_t  nOK;
    int64_t  userId;
    uint32_t ip;
    char     session[SESSION_SIZE];
    char     aesKey[AESKEY_SIZE];
    uint16_t checksum;
};

#pragma pack()

