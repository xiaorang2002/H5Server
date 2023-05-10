// Copyright (c) 2019, Landy
// All rights reserved.

#ifndef MUDUO_WEBSOCKET_CONTEXT_H
#define MUDUO_WEBSOCKET_CONTEXT_H

#include <boost/shared_ptr.hpp>

#include <muduo/base/copyable.h>
#include <muduo/net/Buffer.h>

#include "WebSocketRequest.h"

namespace Landy
{

class muduo::net::Buffer;

class WebSocketContext : public muduo::copyable
{
 public:
  enum WebSocketRequestParseState
  {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  WebSocketContext()
    : state_(kExpectRequestLine)
  {
  }

  // default copy-ctor, dtor and assignment are fine

  // return false if any error
  bool parseRequest(boost::shared_ptr<muduo::net::Buffer> buf, Timestamp receiveTime);

  bool gotAll() const
  { return state_ == kGotAll; }

  void reset()
  {
    state_ = kExpectRequestLine;
    WebSocketRequest dummy;
    request_.swap(dummy);
  }

  const WebSocketRequest& request() const
  { return request_; }

  WebSocketRequest& request()
  { return request_; }

  static string base64Encode(const void* data, size_t length);
  static string getAcceptKey(const string& challenge);
  static string webtime(time_t time);
  static string now();

 private:
  bool processRequestLine(const char* begin, const char* end);

  WebSocketRequestParseState state_;
  WebSocketRequest request_;
};

}


#endif
