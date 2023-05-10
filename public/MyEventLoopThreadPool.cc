
#include "MyEventLoopThreadPool.h"

#include <muduo/net/EventLoop.h>
#include <muduo/net/EventLoopThread.h>

#include <boost/bind.hpp>

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;



namespace Landy
{


MyEventLoopThreadPool::MyEventLoopThreadPool(const string& nameArg)
  : name_(nameArg),
    started_(false),
    numThreads_(0),
    next_(0)
{
}

MyEventLoopThreadPool::~MyEventLoopThreadPool()
{

}

void MyEventLoopThreadPool::start()
{
  assert(!started_);

  started_ = true;

  for (int i = 0; i < numThreads_; ++i)
  {
    char buf[name_.size() + 32];
    snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
    EventLoopThread* t = new EventLoopThread(EventLoopThread::ThreadInitCallback(), buf);
    threads_.push_back(t);
    loops_.push_back(t->startLoop());
  }
}

EventLoop* MyEventLoopThreadPool::getNextLoop()
{
  assert(started_);
  EventLoop* loop = NULL;

  if (!loops_.empty())
  {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size())
    {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop* MyEventLoopThreadPool::getLoopForHash(size_t hashCode)
{
  EventLoop* loop = NULL;

  if (!loops_.empty())
  {
    loop = loops_[hashCode % loops_.size()];
  }
  return loop;
}

}
