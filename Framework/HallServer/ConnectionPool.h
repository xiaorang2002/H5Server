#pragma once

#include <iostream>

#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>
#include <jdbc/cppconn/exception.h>
#include <jdbc/cppconn/driver.h>
#include <jdbc/cppconn/connection.h>
#include <jdbc/cppconn/resultset.h>
#include <jdbc/cppconn/prepared_statement.h>
#include <jdbc/cppconn/statement.h>

#include <mutex>
#include <pthread.h>
#include <list>


#include "gameDefine.h"


namespace Landy
{

using namespace std;
using namespace sql;


class ConnectionPool
{
public:
    // Singleton
    static ConnectionPool* GetInstance();

    //init pool
    bool initPool(std::string url, std::string user, std::string password, string db, int maxSize);

    //get a conn from pool
    Connection* GetConnection(string dbName = DB_ACCOUNT);

    //put the conn back to pool
    void ReleaseConnection(Connection *conn);

    ~ConnectionPool();

private:
    ConnectionPool(){}

    //init DB pool
    bool InitConnection(string db, int initSize);

    // create a connection
    Connection* CreateConnection();

    //destory connection
    void DestoryConnection(Connection *conn);

    //destory db pool
    void DestoryConnPool();


private:
    string m_user;
    string m_password;
    string m_url;
    int m_maxSize;
    atomic_int m_curSize;

    Driver*             m_driver;     //sql driver (the sql will free it)
    list<Connection*>   m_connList;   //create conn list

    //thread lock mutex
    static mutex m_mutex;

};


}






