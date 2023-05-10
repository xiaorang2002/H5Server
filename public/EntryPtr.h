#pragma once


#include <stdint.h>
#include <iostream>
#include <string>
#include <deque>
#include <map>

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Callbacks.h>

#include "Globals.h"

struct Entry : public muduo::copyable
{
  explicit Entry(const muduo::net::TcpConnectionWeakPtr& weakConn)
    : m_weakConn(weakConn)
  {
        m_aesKey = "";
  }

  ~Entry()
  {
    muduo::net::TcpConnectionPtr conn = m_weakConn.lock();
    if (conn)
    {
      conn->shutdown();
      conn->forceClose();
    }
  }

  void setAESKey(string key)
  {
      m_aesKey = key;
  }

  string getAESKey()
  {
      return m_aesKey;
  }

  muduo::net::TcpConnectionWeakPtr m_weakConn;

private:
  string               m_aesKey;
};


typedef shared_ptr<Entry> EntryPtr;
typedef weak_ptr<Entry> WeakEntryPtr;




