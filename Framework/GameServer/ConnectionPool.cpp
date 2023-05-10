#include "ConnectionPool.h"

#include <iostream>
#include <stdexcept>
#include <exception>
#include <stdio.h>


namespace Landy
{


using namespace std;
using namespace sql;

mutex ConnectionPool::m_mutex;

//Singleton: get the single object
ConnectionPool* ConnectionPool::GetInstance()
{
    static ConnectionPool g_instance;
    return &g_instance;
}

bool ConnectionPool::initPool(std::string url, std::string user, string password, string db, int maxSize)
{
    m_user = user;
    m_password = password;
    m_url = url;
    m_maxSize = maxSize;
    m_curSize = 0;

    try
    {
        m_driver=sql::mysql::get_driver_instance();
    }
    catch(sql::SQLException& e)
    {
        perror("Get sql driver failed");
        return false;
    }
    catch(std::runtime_error& e)
    {
        perror("Run error");
        return false;
    }
    return InitConnection(db, maxSize/2);
}

//init conn pool
bool ConnectionPool::InitConnection(string db, int initSize)
{
    Connection* conn;
    for(int i =0;i <initSize; i++)
    {
        conn= CreateConnection();
        if(conn)
        {
            try
           {
                conn->setSchema(db);
                conn->setClientOption("characterSetResults", "utf8");
                conn->setAutoCommit(true);

            }
            catch (sql::SQLException &e)
            {
                std::cout << " (MySQL error code: " << e.getErrorCode();
            }


            lock_guard<mutex> lock(m_mutex);
            m_connList.push_back(conn);
            m_curSize++;
        }else
        {
            perror("create conn error");
            return false;
        }
    }
    return true;
}

Connection* ConnectionPool::CreateConnection()
{
    Connection* conn;
    try
    {
        conn = m_driver->connect(m_url, m_user, m_password);  //create a conn
        return conn;
    }
    catch(sql::SQLException& e)
    {
        perror("link error");
        return NULL;
    }
    catch(std::runtime_error& e)
    {
        perror("run error");
        return NULL;
    }
}

Connection* ConnectionPool::GetConnection(string dbName)
{
    Connection* conn = NULL;
    do
    {
        m_mutex.lock();
        //the pool have a conn.
        if(m_connList.size()>0)
        {
            conn = m_connList.front();
            m_connList.pop_front();

            m_mutex.unlock();

            //if the conn is closed, delete it and recreate it
            if(!conn->isValid())
            {
                delete conn;
                conn = CreateConnection();
            }

            if(conn)
            {
                conn->setSchema(dbName);
                conn->setAutoCommit(true);
            } else
            {
                // sub pool size.
                m_curSize--;
            }
        }
        else
        {
            m_mutex.unlock();
            // update the pool no conn.
            if(m_curSize < m_maxSize)
            {
                conn = this->CreateConnection();
                if(conn)
                {
                    m_curSize++;
                    conn->setSchema(dbName);
                    conn->setAutoCommit(true);
                }
            }
        }

//        // closed.
//        if (conn)
//        {
//            try
//            {
//                // check if the special connect is ok for query now.
//                shared_ptr<Statement> stmt(conn->createStatement());
//                shared_ptr<ResultSet> res(stmt->executeQuery("select 1"));
//            }catch (sql::SQLException& ex)
//            {
//                delete conn;
//                conn = CreateConnection();
//            }
//        }

    } while (0);
//Cleanup:
    return (conn);
}

//put conn back to pool
void ConnectionPool::ReleaseConnection(Connection *conn)
{
    if(conn)
    {
        lock_guard<mutex> lock(m_mutex);
        m_connList.push_back(conn);
    }
}

void ConnectionPool::DestoryConnPool()
{
    list<Connection*>::iterator iter;
    lock_guard<mutex> lock(m_mutex);
    for(iter = m_connList.begin(); iter!=m_connList.end(); ++iter)
    {
        DestoryConnection(*iter);
    }
    m_curSize=0;
    m_connList.clear();
}


void ConnectionPool::DestoryConnection(Connection* conn)
{
    if(conn)
    {
        try
        {
            conn->close();
        }
        catch(sql::SQLException&e)
        {
            perror(e.what());
        }
        catch(std::exception& e)
        {
            perror(e.what());
        }
        delete conn;
    }
}

ConnectionPool::~ConnectionPool()
{
    DestoryConnPool();
}



}
