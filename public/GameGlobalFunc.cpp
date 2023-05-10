#include "GameGlobalFunc.h"


// #include <boost/proto/detail/ignore_unused.hpp>
// #include <boost/algorithm/string.hpp>
// #include <boost/bind.hpp>
// #include <boost/filesystem.hpp>

// #include <muduo/base/Logging.h>
// #include <muduo/base/TimeZone.h>

// #include "RedisClient/RedisClient.h"

// #include <curl/curl.h>

// #include "GlobalFunc.h"


//using namespace Landy;

//================================LOGIC FUNCTION===========================
//bool GameGlobalFunc::RedisLoginInfoScoreBankScoreChange(int32_t userId, int64_t nAddscore, int64_t nAddBankScore, int64_t &newScore, int64_t &newBankScore)
//{
//    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);

//    string strScore="0";
//    string strBankScore="0";
//    double score, bankScore=0;
//    string strKeyName = REDIS_ACCOUNT_PREFIX + to_string(userId);

//    bool b1 = redisClient->hincrby_float(strKeyName, "score", nAddscore, &newScore);
//    bool b2 = redisClient->hincrby_float(strKeyName, "bankScore", nAddBankScore, &newBankScore);

//    return b1 && b2;
//}

//void GameGlobalFunc::RedisSetLoginInfoScore(int32_t userId, double score)
//{
//    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);

//    string strKeyName = REDIS_ACCOUNT_PREFIX + to_string(userId);
//    redisClient->hset(strKeyName, "score", to_string(score));
//}

// insert the special score changed record to the db_record database for log user changed.
//bool GameGlobalFunc::InsertScoreChangeRecord(dword userid, int64_t lUserScore, int64_t lBankScore,
//                                         int64_t lAddScore, int64_t lAddBankScore,
//                                         int64_t lTargetUserScore, int64_t lTargetBankScore,
//                                         int type, int configId)
//{
//    bool bSuccess = false;
//    do
//    {
//        char sql[1024]={0};
//        snprintf(sql,sizeof(sql),"INSERT INTO db_record.record_score_change(userid, source_score,"
//                                 " source_bankscore, change_score, change_bank, target_score, target_bankscore, type, config_id) VALUES(%d, %lf,"
//                                 " %lf, %lf, %lf, %lf, %lf, %d, %d)", userid, lUserScore, lBankScore,
//                                 lAddScore, lAddBankScore, lTargetUserScore, lTargetBankScore, type, configId);
//        // try to bind the special redis client item value data for later user content item value content.
//        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//        bSuccess = redisClient->PushSQL(sql);

//        LOG_DEBUG << ">>> InsertScoreChangeRecord push sql to redis, sql: " << sql;
//    }while (0);
////Cleanup:
//    return (bSuccess);
//}


//bool GameGlobalFunc::UserRechargeScore(shared_ptr<sql::Connection> &conn, int32_t userid, int64_t lAddGem, int64_t lAddScore, int32_t *vipLevel, int64_t *allCharge)
//{
//    int ret = 0;
//    int nVipLevel = 0;
//    double chargeAmount = 0.0;

//    conn->setSchema(DB_ACCOUNT);
////    auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
////    shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_ACCOUNT), fuc);
//    shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//    shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//    pstmt.reset(conn->prepareStatement("update user_extendinfo set gem=gem+?, score=score+?, charge_amount=charge_amount+?,"
//                                       "recharge_times=recharge_times+1 where userid=?"));
//    pstmt->setDouble(1, lAddGem);
//    pstmt->setDouble(2, lAddScore);
//    pstmt->setDouble(3, lAddScore);
//    pstmt->setInt(4, userid);
//    ret = pstmt->executeUpdate();

////    pstmt.reset(conn->prepareStatement("update user_extendinfo set charge_amount=charge_amount+? where userid=?"));
////    pstmt->setDouble(1, lAddScore);
////    pstmt->setInt(2, userid);
////    ret = pstmt->executeUpdate();

//    pstmt.reset(conn->prepareStatement("select charge_amount from user_extendinfo where userid=?"));
//    pstmt->setInt(1, userid);
//    res.reset(pstmt->executeQuery());
//    if(res->rowsCount() > 0 && res->next())
//    {
//        // get the special charge amount.
//        chargeAmount = res->getDouble(1);
//        if(allCharge)
//            *allCharge = chargeAmount;
//    }

//    pstmt.reset(conn->prepareStatement("select level from db_platform.vip_config where ?>=cash order by level desc limit 1"));
//    pstmt->setInt(1, chargeAmount);
//    res.reset(pstmt->executeQuery());
//    if(res->rowsCount() > 0 && res->next())
//    {
//        // get the special charge amount.
//        nVipLevel = res->getInt(1);
//        if(vipLevel)
//            *vipLevel = nVipLevel;
//    }

//    pstmt.reset(conn->prepareStatement("update user_vipinfo set vip=? where userid=? and vip!=?"));
//    pstmt->setInt(1, nVipLevel);
//    pstmt->setInt(2, userid);
//    pstmt->setInt(3, nVipLevel);
//    ret = pstmt->executeUpdate();

//    // update the vip level for later user item now update the vip level for later user item now update the.
//    // update the special vip info content for update the special vip info content for update the.

//    // try to bind the special redis client item value data for later user content item value content.
//    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//    redisClient->setUserLoginInfo(userid, "vip", std::to_string(nVipLevel));

//    return true;
//}


//bool GameGlobalFunc::UserExchangeFailScore(shared_ptr<sql::Connection> &conn, int32_t userid, int64_t lAddGem, int64_t lAddScore, int64_t *allExchange)
//{
//    int ret = 0;

//    conn->setSchema(DB_ACCOUNT);
////    auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
////    shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_ACCOUNT), fuc);
//    shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//    shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//    pstmt.reset(conn->prepareStatement("update user_extendinfo set gem=gem+?, score=score+?, exchange_amount=exchange_amount-?,"
//                                       "exchange_times=exchange_times-1 where userid=?"));
//    pstmt->setDouble(1, lAddGem);
//    pstmt->setDouble(2, lAddScore);
//    pstmt->setDouble(3, lAddScore);
//    pstmt->setInt(4, userid);
//    ret = pstmt->executeUpdate();
//    LOG_DEBUG << "UserExchangeFailScore update ret: " << ret;

//    if(allExchange)
//    {
//        pstmt.reset(conn->prepareStatement("select exchange_amount from user_extendinfo where userid=?"));
//        pstmt->setInt(1, userid);
//        res.reset(pstmt->executeQuery());
//        if(res->rowsCount() > 0 && res->next())
//        {
//            // get the special charge amount.
//            *allExchange = res->getDouble(1);
//        }
//    }


////    pstmt.reset(conn->prepareStatement("select level from db_platform.vip_config where ?>=cash order by level desc limit 1"));
////    pstmt->setInt(1, chargeAmount);
////    res.reset(pstmt->executeQuery());
////    if(res->rowsCount() > 0 && res->next())
////    {
////        // get the special charge amount.
////        nVipLevel = res->getInt(1);
////        if(vipLevel)
////            *vipLevel = nVipLevel;
////    }

////    pstmt.reset(conn->prepareStatement("update user_vipinfo set vip=? where userid=? and vip!=?"));
////    pstmt->setInt(1, nVipLevel);
////    pstmt->setInt(2, userid);
////    pstmt->setInt(3, nVipLevel);
////    ret = pstmt->executeUpdate();

////    // update the vip level for later user item now update the vip level for later user item now update the.
////    // update the special vip info content for update the special vip info content for update the.

////    // try to bind the special redis client item value data for later user content item value content.
////    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
////    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
////    redisClient->setUserLoginInfo(userid, "vip", std::to_string(nVipLevel));

//    return true;
//}

//bool GameGlobalFunc::PromoterGetMoney(shared_ptr<sql::Connection> &conn, int32_t userId, int64_t lAddScore)
//{
//    int ret = 0;

//    conn->setSchema(DB_ACCOUNT);
////    auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
////    shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_ACCOUNT), fuc);
//    shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//    shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//    pstmt.reset(conn->prepareStatement("UPDATE user_extendinfo SET score=score+? WHERE userid=?"));
//    pstmt->setDouble(1, lAddScore);
//    pstmt->setInt(2, userId);
//    ret = pstmt->executeUpdate();
//    LOG_DEBUG << "PromoterGetMoney update ret: " << ret;
//    return true;
//}


//===============================sms=============================
// send the special sms code to the special remote host item value.
//bool GameGlobalFunc::sendCodeSMS(string strPhoneValue, string strCode)
//{
//    auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//    shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_PLATFORM), fuc);
//    shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//    shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//    // try to prepare the special statement
////    pstmt.reset(conn->prepareStatement("SELECT id,name,address,userid,password,account,content FROM sms_config WHERE status=1 and (configid=0 or configid=?)"));
////    pstmt.reset(conn->prepareStatement("SELECT id,name,address,userid,password,account,content FROM sms_config WHERE status=1 and configid=0"));
//    pstmt.reset(conn->prepareStatement("SELECT id,name,address,userid,password,account,content,isJson FROM sms_config WHERE status=1"));
//    res.reset(pstmt->executeQuery());

//    int num = res->rowsCount();
//    if(num <= 0)
//        return false;

//    int index = GlobalFunc::RandomInt64(0, num - 1);
////    index = 0;
//    for(int i = 0; i <= index; ++i)
//    {
//        if(!res->next())
//            return false;
//    }

//    int id = res->getInt(1);
//    string name = res->getString(2);
//    string address = res->getString(3);
//    string userId = res->getString(4);
//    string password = res->getString(5);
//    string account= res->getString(6);
//    string content = res->getString(7);
//    bool isJson = res->getBoolean(8);

//    GlobalFunc::string_replace(address, "$USER_ID$", userId);
//    GlobalFunc::string_replace(address, "$ACCOUNT$", account);
//    GlobalFunc::string_replace(address, "$PASSWORD$", password);
//    GlobalFunc::string_replace(address, "$PHONE_NUM$", strPhoneValue);

//    GlobalFunc::string_replace(content, "$USER_ID$", userId);
//    GlobalFunc::string_replace(content, "$ACCOUNT$", account);
//    GlobalFunc::string_replace(content, "$PASSWORD$", password);
//    GlobalFunc::string_replace(content, "$PHONE_NUM$", strPhoneValue);


//    GlobalFunc::string_replace(content, "$SMSCODE$", strCode);


//    LOG_INFO << "ADDRESS: "<<address;
//    LOG_INFO << "CONTENT: " <<content;

//    // ============ create new veritycode ============
////     string strOutput = "action=send&userid=8321&account=lanshenglin&password=12345678&mobile="+strPhoneValue
////             +"&content=您的短信验证码是:【"+strCode+"】.请不要把验证码泄漏给其他人.如非本人操作,可不用理会!  回T退订【平和堂】&sendTime=&extno=";
////     return curlSendSMS(strOutput);

//    int ret = GlobalFunc::curlSendSMS(address, content, isJson);

//    string reason;
//    switch (ret)
//    {
//    case CURLE_OK:
//        reason = "OK";
//        break;
//    case CURLE_UNSUPPORTED_PROTOCOL:
//        reason = "不支持的协议，由URL的头部指定";
//        break;
//    case CURLE_COULDNT_CONNECT:
//        reason = "不能连接到remote 主机或者代理";
//        break;
//    case CURLE_HTTP_RETURNED_ERROR:
//        reason = "Http返回错误";
//        break;
//    default:
//        reason = "错误";
//        break;
//    }

//    conn->setSchema(DB_RECORD);
//    pstmt.reset(conn->prepareStatement("INSERT INTO record_sms_send(phone, code, reason, status, configid) VALUES(?,?,?,?,?)"));
//    pstmt->setString(1, strPhoneValue);
//    pstmt->setString(2, strCode);
//    pstmt->setString(3, reason);
//    pstmt->setInt(4, ret);
//    pstmt->setInt(5, id);
//    pstmt->executeUpdate();

//    return true;
//}
