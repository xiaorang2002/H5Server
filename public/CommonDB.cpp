
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include "muduo/base/Logging.h"
#include "muduo/base/TimeZone.h"
//#include "RedisClient/RedisClient.h"


#include "GlobalFunc.h"
#include "GameGlobalFunc.h"
#include "ThreadLocalSingletonMongoDBClient.h"

#include "proto/Game.Common.pb.h"
#include "proto/HallServer.Message.pb.h"

#include "CommonDB.h"


using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;

using namespace bsoncxx::builder::basic;
using namespace bsoncxx::types;

using namespace std;

CommonDB::CommonDB(string str)
{
    status = 0;
    task_amount = 0;
    refresh_time = 0;
    m_isUpdate = true;
    task_ids.clear();
    agent_ids.clear();
    // m_tasksRecord.clear();
}
CommonDB::CommonDB()
{
    cout << __FUNCTION__ <<" ------- ";
}
CommonDB::~CommonDB()
{
}
