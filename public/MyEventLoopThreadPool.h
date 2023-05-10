// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef MY_NET_EVENTLOOPTHREADPOOL_H
#define MY_NET_EVENTLOOPTHREADPOOL_H

#include "muduo/base/Types.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/EventLoopThread.h"

#include <vector>
#include <boost/ptr_container/ptr_vector.hpp>

using namespace std;

using namespace muduo;
using namespace muduo::net;


namespace Landy
{


class MyEventLoopThreadPool : noncopyable
{
 public:

  MyEventLoopThreadPool(const string& nameArg);
  ~MyEventLoopThreadPool();
  void setThreadNum(int numThreads) { numThreads_ = numThreads; }
  void start();

  // valid after calling start()
  /// round-robin
  EventLoop* getNextLoop();

  /// with the same hash code, it will always return the same EventLoop
  EventLoop* getLoopForHash(size_t hashCode);

  bool started() const
  { return started_; }

  const string& name() const
  { return name_; }

 private:
    string name_;
    atomic_bool started_;
    atomic_int numThreads_;
    atomic_int next_;
    boost::ptr_vector<EventLoopThread> threads_;
    std::vector<EventLoop*> loops_;
};

}

#endif  // MY_NET_EVENTLOOPTHREADPOOL_H
