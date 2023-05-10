// Copyright (c) 2019, Landy
// All rights reserved.

#pragma once

#include <muduo/base/Mutex.h>  // MCHECK

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <memory>
#include <RedisClient/Redisclient.h>

using namespace std;

class ThreadLocalSingletonRedisClient : boost::noncopyable
{

public:
    static RedisClient& instance()
    {
        if(!t_value_)
        {
            t_value_ = new RedisClient;
            deleter_.set(t_value_);
        }
        return *t_value_;
    }
    //reset() 抛连接断开异常时调用
    static void reset() {
        deleter_.reset();
        t_value_ = 0;
    }
	
    static RedisClient* pointer()
    {
        return t_value_;
    }

    static void setUri(string mongoDBStr) { m_mongoDBConntionString = mongoDBStr; }

private:
    ThreadLocalSingletonRedisClient();
    ~ThreadLocalSingletonRedisClient();


    static void destructor(void* obj)
    {
        assert(obj == t_value_);
        typedef char T_must_be_complete_type[sizeof(RedisClient) == 0 ? -1 : 1];
        T_must_be_complete_type dummy; (void) dummy;
        delete t_value_;
        t_value_ = 0;
    }

    class Deleter
    {
    public:
        Deleter()
        {
            pthread_key_create(&pkey_, &ThreadLocalSingletonRedisClient::destructor);
        }

        ~Deleter()
        {
            pthread_key_delete(pkey_);
        }

        void reset() {
            pthread_key_delete(pkey_);
        }

        void set(RedisClient* newObj)
        {
            assert(pthread_getspecific(pkey_) == NULL);
            pthread_setspecific(pkey_, newObj);
        }

        pthread_key_t pkey_;
    };

    static __thread RedisClient * t_value_;
    static Deleter deleter_;

    static string  m_mongoDBConntionString;
   
};




