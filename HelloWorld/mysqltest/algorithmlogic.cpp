#include <ctime>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>
#include <boost/date_time.hpp>
#include <bsoncxx/validate.hpp>
#include <mongocxx/database.hpp>
#include "boost/date_time/parse_format_base.hpp"

#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/GlobalFunc.h"
#include "ConnectionPool.h"


#define MONGODBCLIENT ThreadLocalSingletonMongoDBClient::instance()

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

#include "algorithmlogic.h"
#include <time.h>//110 40 //120 50

using namespace Landy;
using namespace std ;
using namespace std::chrono;

AlgorithmLogic::AlgorithmLogic(int32_t count,int32_t timedistant):
    timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(), "ThreadInitCallback"))
{
    srand(time_t());
    timelooper->startLoop();

    numRount =count;
    timeDis=timedistant;



    InItAgorithm();
}
AlgorithmLogic::~AlgorithmLogic()
{

}

bool AlgorithmLogic::startDB(string host, uint16_t port, string dbname, string name, string password, int db_password_len, int maxSize)
{
    string url = "tcp://"+host+":"+std::to_string(port);
    string dbPassword = /*decryDBKey(password, db_password_len);*/password;
    bool bSucces = ConnectionPool::GetInstance()->initPool(url, name, dbPassword, dbname, maxSize);
    return bSucces;
}

//这个要求临时先写mysql 和 mongodb
void AlgorithmLogic::WtiteUserPlayRecordMysqlx(string mysqlStr)
{
    try
    {
        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



        conn->setSchema(DB_RECORD);
        pstmt.reset(conn->prepareStatement(mysqlStr));

        pstmt->executeUpdate();
        LOG_WARN << "write user recored count--------";
    }
    catch (sql::SQLException &e)
    {
        std::cout << " (MySQL error code: " << e.getErrorCode();
        LOG_ERROR<< " (MySQL error code: " << e.getErrorCode();
    }
}
bool AlgorithmLogic::InItAgorithm()
{
    timelooper->getLoop()->runEvery((double)timeDis,bind(&AlgorithmLogic::ThreadInitCallback,this));
    return true;
}
void AlgorithmLogic::ThreadInitCallback()
{

    string mysqlRecord="";
    for(int i=0;i<numRount;i++)
    {

        int32_t m_year = 0;
        char bufer[20000];
                     string tableName ="";
                     time_t tx = time(NULL); //获取目前秒时间
                     struct tm * local;
                     local = localtime(&tx);
                     string months ="";
                     if(local->tm_mon+1>=10 ) months = to_string(local->tm_mon+1);
                     else months = "0"+to_string(local->tm_mon+1);
                     tableName = "play_record_mt";
                     m_year = (local->tm_year+1900)*100+local->tm_mon+1;
        if(i==0)
        {
            mysqlRecord="INSERT INTO "+tableName+"(userid, agentid, "
                                                               "gameid,roomid,tableid,chairid,isBanker,"
                                                               "usercount,beforescore,rwinscore,"
                                                               "allbet,winscore,score,revenue,isandroid,"
                                                               "rank,month,cardvalue,gameinfoid,account,linecode,gamestarttime,gameendtime,cellscore,id)";
           string uuid=GlobalFunc::generateUUID();
           sprintf(bufer,"VALUES(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                   100,100,100,100,100,
                   1,0,100,100,100,
                   100,100,100,100,0,100,m_year,"ok",
                   "ok","ok","ok","ok","ok",
                   "ok",uuid.c_str());
           mysqlRecord+=bufer;
        }
        else
        {
            string uuid=GlobalFunc::generateUUID();
            sprintf(bufer,",(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                    100,100,100,100,100,
                    1,0,100,100,100,
                    100,100,100,100,0,100,m_year,"ok",
                    "ok","ok","ok","ok","ok",
                    "ok",uuid.c_str());
            mysqlRecord+=bufer;
        }
    }

    WtiteUserPlayRecordMysqlx(mysqlRecord);
}

