#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>


#include "algorithmlogic.h"
#include <stdio.h>
#include <unistd.h>  // usleep
#include "redblackalgorithm.h"








shared_ptr <EventLoopThread>     timelooper;
void threadFun()
{


}
//int main()
//{
////    AlgorithmLogic alt;

////    while(alt.InItAgorithm())
////    {
////        sleep(10000);
////    }

//}
#include <stdio.h>
#include <unistd.h>
#include "IAicsEngine.h"
#include "time.h"
using namespace std;
int RandomInt(int min,int max)
{
    if(min>max)
    {
        int res=min;
        min=max;
        max=res;
    }
    if(min==max)
    {
        return min;
    }
    return min+rand()%(max-min)+1;
}
int firetimes=0;
int randomdds=10;
int main(int argc,char* argv[])
{
    srand(time_t(NULL));

    AlgorithmLogic *redBlack=NULL;
    if(boost::filesystem::exists("./conf/mysqltest.conf"))
    {
        boost::property_tree::ptree pt;
        boost::property_tree::read_ini("./conf/mysqltest.conf", pt);
        string mysql_ip = pt.get<string>("mysql.IP", "192.168.2.29");
        uint16_t mysql_port = pt.get<int>("mysql.Port", 3306);
        string mysql_dbname = pt.get<string>("mysql.DBName","user_recored");
        string mysql_user = pt.get<string>("mysql.User","root");
        string mysql_password = pt.get<string>("mysql.Password","123456");
        int mysql_max_connection = pt.get<int>("mysql.Len", 0);
        int db_password_len = pt.get<int>("mysql.Len", 0);
        int32_t roundNum = pt.get<int>("mysql.roundNum", 10);
        int32_t timeNum = pt.get<int>("mysql.timeDis", 5);
        redBlack= new AlgorithmLogic(roundNum,timeNum);
        redBlack->startDB(mysql_ip, mysql_port, mysql_dbname, mysql_user, mysql_password, db_password_len, mysql_max_connection);
    }
    while(redBlack)
    {

    }
    printf("all done!\n");
    return 0;
}
