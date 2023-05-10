// Copyright (c) 2019, Landy
// All rights reserved.

#pragma once

#include <muduo/base/Mutex.h>  // MCHECK

#include <boost/noncopyable.hpp>
#include <pthread.h>
#include <memory>

#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/json.hpp>

#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/config/config.hpp>
#include <mongocxx/options/client.hpp>
#include <mongocxx/options/client_session.hpp>

using namespace std;

class ThreadLocalSingletonMongoDBClient : boost::noncopyable
{

public:
    static mongocxx::client& instance()
    {
        if(!t_value_)
        {
            t_value_ = new mongocxx::client(mongocxx::uri{m_mongoDBConntionString}, m_options);
            deleter_.set(t_value_);
        }
        return *t_value_;
    }
    //reset() 抛连接断开异常时调用
    static void reset() {
        deleter_.reset();
        t_value_ = 0;
    }
	
    static mongocxx::client* pointer()
    {
        return t_value_;
    }

    static void setUri(string mongoDBStr) { m_mongoDBConntionString = mongoDBStr; }
    static void setOption(mongocxx::options::client options)   { m_options = options; }

private:
    ThreadLocalSingletonMongoDBClient();
    ~ThreadLocalSingletonMongoDBClient();


    static void destructor(void* obj)
    {
        assert(obj == t_value_);
        typedef char T_must_be_complete_type[sizeof(mongocxx::client) == 0 ? -1 : 1];
        T_must_be_complete_type dummy; (void) dummy;
        delete t_value_;
        t_value_ = 0;
    }

    class Deleter
    {
    public:
        Deleter()
        {
            pthread_key_create(&pkey_, &ThreadLocalSingletonMongoDBClient::destructor);
        }

        ~Deleter()
        {
            pthread_key_delete(pkey_);
        }

        void reset() {
            pthread_key_delete(pkey_);
        }

        void set(mongocxx::client* newObj)
        {
            assert(pthread_getspecific(pkey_) == NULL);
            pthread_setspecific(pkey_, newObj);
        }

        pthread_key_t pkey_;
    };

    static __thread mongocxx::client * t_value_;
    static Deleter deleter_;

    static string  m_mongoDBConntionString;
    static mongocxx::options::client m_options;
};




