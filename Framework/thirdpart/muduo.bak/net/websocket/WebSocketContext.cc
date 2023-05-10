// Copyright (c) 2019, Landy
// All rights reserved.

#include <arpa/inet.h>

#include "WebSocketContext.h"

#include <muduo/net/Buffer.h>
#include "sha1.h"

using namespace muduo;
using namespace muduo::net;

namespace  Landy
{

bool WebSocketContext::processRequestLine(const char* begin, const char* end)
{
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  if (space != end && request_.setMethod(start, space))
  {
    start = space+1;
    space = std::find(start, end, ' ');
    if (space != end)
    {
      const char* question = std::find(start, space, '?');
      if (question != space)
      {
        request_.setPath(start, question);
        request_.setQuery(question, space);
      }
      else
      {
        request_.setPath(start, space);
      }
      start = space+1;
      succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
      if (succeed)
      {
        if (*(end-1) == '1')
        {
          request_.setVersion(WebSocketRequest::kHttp11);
        }
        else if (*(end-1) == '0')
        {
          request_.setVersion(WebSocketRequest::kHttp10);
        }
        else
        {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// return false if any error
bool WebSocketContext::parseRequest(boost::shared_ptr<muduo::net::Buffer> buf, Timestamp receiveTime)
{
  bool ok = true;
  bool hasMore = true;
  while (hasMore)
  {
    if (state_ == kExpectRequestLine)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        ok = processRequestLine(buf->peek(), crlf);
        if (ok)
        {
          request_.setReceiveTime(receiveTime);
          buf->retrieveUntil(crlf + 2);
          state_ = kExpectHeaders;
        }
        else
        {
          hasMore = false;
        }
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectHeaders)
    {
      const char* crlf = buf->findCRLF();
      if (crlf)
      {
        const char* colon = std::find(buf->peek(), crlf, ':');
        if (colon != crlf)
        {
          request_.addHeader(buf->peek(), colon, crlf);
        }
        else
        {
          // empty line, end of header
          // FIXME:
          state_ = kGotAll;
          hasMore = false;
        }
        buf->retrieveUntil(crlf + 2);
      }
      else
      {
        hasMore = false;
      }
    }
    else if (state_ == kExpectBody)
    {
      // FIXME:
    }
  }
  return ok;
}

const char cb64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

string WebSocketContext::base64Encode(const void* data, size_t length)
{
    std::string output;
    const auto dataPtr = reinterpret_cast<const uint8_t*>(data);
    for (auto i = 0u; i < length; i += 3)
    {
        const auto bytesLeft = length - i;
        const auto b0 = dataPtr[i];
        const auto b1 = bytesLeft > 1 ? dataPtr[i + 1] : 0;
        const auto b2 = bytesLeft > 2 ? dataPtr[i + 2] : 0;
        output.push_back(cb64[b0 >> 2]);
        output.push_back(cb64[((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4)]);
        output.push_back((bytesLeft > 1 ? cb64[((b1 & 0x0f) << 2) | ((b2 & 0xc0) >> 6)] : '='));
        output.push_back((bytesLeft > 2 ? cb64[b2 & 0x3f] : '='));
    }
    return output;
}

namespace
{
    const char* magicString = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
}

string WebSocketContext::getAcceptKey(const std::string& challenge)
{
    auto fullString = challenge + magicString;
    SHA1 hasher;
    hasher.Input(fullString.c_str(), fullString.size());
    unsigned hash[5];
    hasher.Result(hash);
    for (unsigned int& i : hash) {
        i = htonl(i);
    }
    return base64Encode(hash, sizeof(hash));
}

string WebSocketContext::webtime(time_t time)
{
    struct tm timeValue;
    gmtime_r(&time, &timeValue);
    char buf[1024];
    // Wed, 20 Apr 2011 17:31:28 GMT
    strftime(buf, sizeof(buf) - 1, "%a, %d %b %Y %H:%M:%S %Z", &timeValue);
    return buf;
}

string WebSocketContext::now()
{
    return webtime(time(nullptr));
}

}
