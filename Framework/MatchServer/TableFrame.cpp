#include <sys/time.h>
#include <stdarg.h>

#include <atomic>

#include <boost/bind.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>

#include <muduo/base/Logging.h>

#include "public/SubNetIP.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "public/Globals.h"
#include "public/gameDefine.h"
#include "public/GlobalFunc.h"
// #include "public/GameGlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
//#include "EntryPtr.h"

#include "TableFrame.h"

#include "AndroidUserManager.h"
#include "GameServer/ServerUserManager.h"
#include "GameTableManager.h"
#include "proto/GameServer.Message.pb.h"
#include "TraceMsg/FormatCmdId.h"
#include "ThreadLocalSingletonMongoDBClient.h"
#include "ThreadLocalSingletonRedisClient.h"
using namespace std;
using namespace Landy;
using namespace boost::gregorian;


using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::close_array;


extern int g_bisDebug;
extern int g_bisDisEncrypt;


atomic_llong  CTableFrame::m_CurStock(0);
double        CTableFrame::m_LowestStock(0);
double        CTableFrame::m_HighStock(0);


int           CTableFrame::m_SystemAllKillRatio(0);
int           CTableFrame::m_SystemReduceRatio(0);
int           CTableFrame::m_ChangeCardRatio(0);


CTableFrame::CTableFrame()
        : m_pTableFrameSink(NULL)
        , m_pGameInfo(NULL)
        , m_pGameRoomInfo(NULL)
        , m_pLogicThread(NULL)
        ,m_pMatchRoom(NULL)
//        , m_game_logic_thread(NULL)
        , m_cbGameStatus(GAME_STATUS_INIT)
{
    m_UserList.clear();

    memset(&m_TableState,0,sizeof(m_TableState));
}

CTableFrame::~CTableFrame()
{
    m_UserList.clear();
}

void CTableFrame::Init(shared_ptr<ITableFrameSink>& pSink, TableState& tableState,
                       tagGameInfo* pGameInfo, tagGameRoomInfo* pGameRoomInfo,
                       shared_ptr<EventLoopThread>& game_logic_thread, shared_ptr<EventLoopThread>& gameDBThread,ILogicThread* pLogicThread)
{
    m_pTableFrameSink = pSink;
    m_pGameInfo = pGameInfo;
    m_pGameRoomInfo = pGameRoomInfo;
    m_pLogicThread	= pLogicThread;

    m_game_logic_thread = game_logic_thread;
    m_game_mysql_thread = gameDBThread;
    m_TableState = tableState;
    m_UserList.resize(m_pGameRoomInfo->maxPlayerNum);

    for(uint32_t chairId = 0; chairId < m_pGameRoomInfo->maxPlayerNum; ++chairId)
    {
        shared_ptr<CServerUserItem> cServerUserItem;
//        cServerUserItem.reset();
        m_UserList[chairId] = cServerUserItem;
    }
}

string CTableFrame::GetNewRoundId()
{
    string strRoundId = to_string(m_pGameRoomInfo->roomId) + "-";

//    chrono::system_clock::time_point t = chrono::system_clock::now();
    int64_t seconds_since_epoch = chrono::duration_cast<chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    strRoundId += to_string(seconds_since_epoch) + "-";
    strRoundId += to_string(::getpid()) + "-";
    strRoundId += to_string(GetTableId())+"-";
    strRoundId += to_string(rand()%10);
    return strRoundId;
}

bool CTableFrame::sendData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                           vector<uint8_t> &data, Header *commandHeader, int enctype)
{
    bool bSuccess = false;
    do
    {
        if(!pIServerUserItem)
            break;
        if(pIServerUserItem && !pIServerUserItem->IsAndroidUser())
        {
            string aesKey = "";

            uint32_t realSize = data.size();

            int64_t userId = pIServerUserItem->GetUserId();
            boost::tuple<TcpConnectionWeakPtr, shared_ptr<internal_prev_header>> mytuple = m_pLogicThread->GetProxyConnectionWeakPtrFromUserId(userId);
            TcpConnectionWeakPtr weakConn = mytuple.get<0>();
            shared_ptr<internal_prev_header> internal_header_ptr = mytuple.get<1>();
            TcpConnectionPtr conn(weakConn.lock());
            if(likely(conn) && internal_header_ptr)
            {
                // try to check if the special interal header has breen ok and get the content now.
                if (!internal_header_ptr)
                {
                    LOG_ERROR << " >>> sendData get internal_header failed error!,userId:" << pIServerUserItem->GetUserId();
                    break;
                }
//                internal_prev_header *internal_header = new internal_prev_header();
//                memcpy(internal_header, internal_header_ptr.get(), sizeof(internal_prev_header));
//                shared_ptr<internal_prev_header> internal_header_ptr_free(internal_header);

//                // store the command id the the ip field.
//                int16_t cmdid = ((mainId << 8) | subId);
//                internal_header->ip = cmdid;

//                // try to get the special aes key content value for later user content.
//                string aesKey(internal_header->aesKey, sizeof(internal_header->aesKey));
                int ret = -1;
                vector<unsigned char> encryptedData;
//                if(enctype)
//                {
//                    // try to encrypt the speciald data and build the pub header content item now.
//                    ret = Landy::Crypto::aesEncrypt(aesKey, data, encryptedData, internal_header);
//                }else
                {
                    int packsize = sizeof(internal_prev_header) + sizeof(Header) + data.size();
                    encryptedData.resize(packsize);
                    memcpy(&encryptedData[sizeof(internal_prev_header)+sizeof(Header)], &data[0], data.size());

                    internal_prev_header* internal = (internal_prev_header*)&encryptedData[0];
                    memcpy(internal, internal_header_ptr.get(), sizeof(internal_prev_header));
                    internal->len = packsize;
                    GlobalFunc::SetCheckSum(internal);
//                    internal++;

                    Header* pubheader = (Header*)(&encryptedData[sizeof(internal_prev_header)]);
                    if(commandHeader)
                    {
                        memcpy(pubheader, commandHeader, sizeof(Header));
                    }else
                    {
                        pubheader->ver = PROTO_VER;
                        pubheader->sign = HEADER_SIGN;
                        pubheader->reserve = 0;
                        pubheader->reqId = 0;
                    }
                    pubheader->mainId = mainId;
                    pubheader->subId = subId;
                    pubheader->encType = PUBENC__PROTOBUF_NONE;
                    pubheader->realSize = realSize;
                    pubheader->len = realSize + sizeof(Header);
                    pubheader->realSize = realSize;
                    pubheader->crc = GlobalFunc::GetCheckSum(&encryptedData[sizeof(internal_prev_header)+4], pubheader->len - 4);
                    ret = packsize;
                }

                if (likely(ret > 0 && !encryptedData.empty()))
                {
					TRACE_COMMANDID(mainId, subId);
                    // send the special data content.
                    conn->send(&encryptedData[0], ret);
                    bSuccess = true;
                }
            }
            else
            {
                int64_t userId = pIServerUserItem->GetUserId();
                bool  isAndroid = pIServerUserItem->IsAndroidUser();
                //LOG_WARN <<"CTableFrame sendData TcpConnectionPtr closed. userId:" << userId << ", isAndroid:" << isAndroid << ",mainid:" << mainId << "subid:" << subId;
            }
        }
        else
        {
            // try to send the special user data item value content value content value data now value data now.
            bSuccess = pIServerUserItem->SendUserMessage(mainId, subId, (const uint8_t*)&data[0], data.size());
        }
    }while (0);
//Cleanup:
    return (bSuccess);
}

bool CTableFrame::SendTableData(uint32_t chairId, uint8_t subid, const uint8_t* data, int size, bool bRecord /* = true */)
{
    try
    {
        if((chairId >= m_pGameRoomInfo->maxPlayerNum) && (chairId != INVALID_CHAIR))
        {
            return false;
        }

        if(INVALID_CHAIR == chairId)
        {
            for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
            {
                shared_ptr<CServerUserItem> pIServerUserItem;
                {
//                    READ_LOCK(m_list_mutex);
                    pIServerUserItem = m_UserList[i];
                }

                if(pIServerUserItem)
                {
                    SendUserData(pIServerUserItem, subid, data, size);
                }
            }
        }else
        {
            shared_ptr<CServerUserItem> pIServerUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pIServerUserItem = m_UserList[chairId];
            }

            if(pIServerUserItem)
            {
                SendUserData(pIServerUserItem,subid, data, size);
            }
        }
    }catch(exception e)
    {
        LOG_ERROR<<e.what();
    }
    return (true);
}

bool CTableFrame::SendUserData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t subid, const uint8_t* data, int datasize, bool isRecord)
{
    int  mainId   = Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC;
    bool bSuccess = false;

    do
    {
        if((!pIServerUserItem) || (!data) || (!datasize))
            break;

//        shared_ptr<CServerUserItem> pIServerUserItem = dynamic_pointer_cast<CServerUserItem>(pIBaseUserItem);
        if(!pIServerUserItem->IsAndroidUser()) //real user
        {
            // send the special data to the remote host now.
            ::GameServer::MSG_CSC_Passageway pass;
            ::Game::Common::Header* header = pass.mutable_header();
            header->set_sign(PROTO_BUF_SIGN);

            pass.set_passdata(data, datasize);

            int enctype = PUBENC__PROTOBUF_AES; // no encrypt.
            if(g_bisDisEncrypt)
            {
                enctype = PUBENC__PROTOBUF_NONE;
            }

            int bytesize = pass.ByteSize();
            vector<unsigned char> vecData(bytesize);

            if(pass.SerializeToArray(&vecData[0], bytesize))
            {
                bSuccess = sendData(pIServerUserItem, mainId, subid, vecData, nullptr, enctype);
            }else
            {
                LOG_ERROR << "pass.SerializeToArray failed.";
            }
        }else
        {
            pIServerUserItem->SendUserMessage(mainId, subid,data,datasize);
        }

    }while(0);
Cleanup:
    return (bSuccess);
}

bool CTableFrame::SendGameMessage(uint32_t chairId, const std::string& szMessage, uint8_t msgType, int64_t score)
{
    bool bSuccess = false;
    uint8_t mainId = ::Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC;
    uint8_t subId  = ::GameServer::SUB_GF_SYSTEM_MESSAGE;
    uint32_t maxPlayerNum = m_pGameRoomInfo->maxPlayerNum;

    do
    {
        if((chairId != INVALID_CHAIR) && (chairId >= maxPlayerNum))
        {
            return false;
        }

        if( (msgType & SMT_GLOBAL) && (msgType & SMT_SCROLL) )
        {
            shared_ptr<CServerUserItem> pIServerUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pIServerUserItem = m_UserList[chairId];
            }

            if(pIServerUserItem)
            {
                Json::Value root;
                root["gameId"] = m_pGameRoomInfo->gameId;
                root["nickName"] = pIServerUserItem->GetNickName();
                root["roomKind"] = m_pGameRoomInfo->roomName;
                root["cardType"] = szMessage;
                root["score"] = score;
                Json::FastWriter writer;
                string msg = writer.write(root);

//                redisClient->publishUserKillBossMessage(msg);
                return true;
            }
        }


        ::GameServer::MSG_S2C_SystemMessage msgInfo;
        ::Game::Common::Header* header = msgInfo.mutable_header();
        header->set_sign(HEADER_SIGN);
//        header->set_crc(0);
//        header->set_mainid(mainid);
//        header->set_subid(subid);
        // set the message type value.
        msgInfo.set_msgtype(msgType);
        msgInfo.set_msgcontent(szMessage);

        int enctype = PUBENC__PROTOBUF_AES; // no encrypt.
        if(g_bisDisEncrypt)
        {
            enctype = PUBENC__PROTOBUF_NONE;
        }

        int bytesize = msgInfo.ByteSize();
        vector<unsigned char> vecData(bytesize);
        if(!msgInfo.SerializeToArray(&vecData[0], bytesize))
        {
            LOG_ERROR << "SendGameMessage SerilizeToArray failed";
            break;
        }

        if (INVALID_CHAIR == chairId)
        {
            for (int idx = 0; idx < maxPlayerNum; ++idx)
            {
                shared_ptr<CServerUserItem> pIServerUserItem;
                {
//                    READ_LOCK(m_list_mutex);
                    pIServerUserItem = m_UserList[chairId];
                }

                if(pIServerUserItem)
                {
                    bSuccess = sendData(pIServerUserItem, mainId, subId, vecData, nullptr, enctype);
                }
            }
        }else
        {
            shared_ptr<CServerUserItem> pIServerUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pIServerUserItem = m_UserList[chairId];
            }

            if(pIServerUserItem)
            {
                bSuccess = sendData(pIServerUserItem, mainId, subId, vecData, nullptr, enctype);
            }
        }
    }while (0);
Cleanup:
    return (bSuccess);
}

void CTableFrame::GetTableInfo(TableState& tableState)
{
    tableState = m_TableState;
}

uint32_t CTableFrame::GetTableId()
{
    return m_TableState.nTableID;
}

shared_ptr<EventLoopThread> CTableFrame::GetLoopThread()
{
    return m_game_logic_thread;
}

bool CTableFrame::DismissGame()
{
//    m_pTableFrameSink->OnEventGameConclude(INVALID_CHAIR, GER_DISMISS);
    return true;
}

bool CTableFrame::ConcludeGame(uint8_t gameStatus)
{
    m_cbGameStatus = gameStatus;

    // modify by James end.
    if (m_pTableFrameSink)
    {
        m_pTableFrameSink->RepositionSink();
    }
    if(m_pMatchRoom)
    {
        m_pMatchRoom->TableFinish(this->GetTableId());
    }
    return !m_pLogicThread->IsServerStoped();
}

int64_t CTableFrame::CalculateRevenue(int64_t score)
{
    return score * m_pGameInfo->revenueRatio / 100;
}

shared_ptr<CServerUserItem> CTableFrame::GetTableUserItem(uint32_t chairId)
{
    shared_ptr<CServerUserItem> pIServerUserItem;
//    pIServerUserItem.reset();
    if(chairId < m_pGameRoomInfo->maxPlayerNum)
    {
//        READ_LOCK(m_list_mutex);
        pIServerUserItem = m_UserList[chairId];
    }
    return pIServerUserItem;
}

shared_ptr<CServerUserItem> CTableFrame::GetUserItemByUserId(int64_t userId)
{
    shared_ptr<CServerUserItem> pIServerUserItem;
//    pIServerUserItem.reset();

//    READ_LOCK(m_list_mutex);
    for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
    {
        // check the special user item data value now.
        shared_ptr<CServerUserItem> pITempUserItem = m_UserList[i];
        if(pITempUserItem && pITempUserItem->GetUserId() == userId)
        {
            return pITempUserItem;
        }
    }
    // LOG_DEBUG << "GetUserItemByUserId userid:" << userid << ", pIServerUserItem:" << pIServerUserItem;
    return pIServerUserItem;
}

bool CTableFrame::IsExistUser(uint32_t chairId)
{
    bool bExist = false;
    do
    {
        if ((chairId < 0) || (chairId >= m_pGameRoomInfo->maxPlayerNum))
            break;

        shared_ptr<CServerUserItem> pIServerUserItem;
        {
//            READ_LOCK(m_list_mutex);
            pIServerUserItem = m_UserList[chairId];
        }

        if ((pIServerUserItem) && (pIServerUserItem->GetUserId() > 0))
        {
            bExist = true;
        }

    }while(0);
    return bExist;
}

bool CTableFrame::CloseUserConnect(uint32_t chairId)
{
    //LOG_ERROR << "CTableFrame::CloseUserConnect NO IMPL yet!";
}

void CTableFrame::SetGameStatus(uint8_t cbStatus)
{
    //LOG_DEBUG << "SetGameStatus, cbStatus:" << (int)cbStatus;
   m_cbGameStatus = cbStatus;
}

// get the special game status now.
uint8_t CTableFrame::GetGameStatus()
{
    return m_cbGameStatus;
}

bool CTableFrame::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    if(m_cbGameStatus==GAME_STATUS_START || GetPlayerCount()>=m_pGameRoomInfo->maxPlayerNum)
    {
        return false;
    }
    return true;

    //return m_pTableFrameSink->CanJoinTable(pUser);
}

bool CTableFrame::CanLeftTable(int64_t userId)
{
    return m_pTableFrameSink->CanLeftTable(userId);
}

void CTableFrame::SetUserTrustee(uint32_t chairId, bool bTrustee)
{
    if(chairId < m_pGameRoomInfo->maxPlayerNum)
    {
        shared_ptr<CServerUserItem> pIServerUserItem;
        {
//            READ_LOCK(m_list_mutex);
            pIServerUserItem = m_UserList[chairId];
        }
        if (pIServerUserItem)
        {
            pIServerUserItem->SetTrustship(bTrustee);
        }
    }
    return;
}

bool CTableFrame::GetUserTrustee(uint32_t chairId)
{
    bool bTrustee = false;

    if(chairId < m_pGameRoomInfo->maxPlayerNum)
    {
        shared_ptr<CServerUserItem> pIServerUserItem;
        {
//            READ_LOCK(m_list_mutex);
            pIServerUserItem = m_UserList[chairId];
        }
        if(pIServerUserItem)
        {
            bTrustee = pIServerUserItem->GetTrustship();
        }
    }
    return bTrustee;
}

void CTableFrame::SetUserReady(uint32_t chairId)
{
    return;
}

bool CTableFrame::OnUserLeft(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendToSelf, bool bForceLeave)
{
    bool ret = false;
    if(pIServerUserItem)
    {
        //LOG_DEBUG << __FILE__ << __FUNCTION__ << "CTableFrame::OnUserLeft"<<pIServerUserItem->GetUserId();
//        if(m_pGameInfo->gameType == GameType_BaiRen)
        {
            pIServerUserItem->SetUserStatus(sOffline);
            pIServerUserItem->SetTrustship(true);
//            BroadcastUserStatus(pIServerUserItem, bSendToSelf);
        }
        ret = m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserId(), false);
    }
    return ret;
}

bool CTableFrame::OnUserOffline(shared_ptr<CServerUserItem>& pIServerUserItem, bool bLeaveGs)
{
    bool ret = false;
    if(pIServerUserItem)
    {
        //LOG_DEBUG << __FILE__ << __FUNCTION__ << "CTableFrame::OnUserOffline"<<pIServerUserItem->GetUserId();
        {
            pIServerUserItem->SetUserStatus(sOffline);
            pIServerUserItem->SetTrustship(true);
//            BroadcastUserStatus(pIServerUserItem, false);
        }
        //掉线暂时不做任何处理
        //ret = m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserId(), false);
    }
    return ret;
}

uint32_t CTableFrame::GetPlayerCount(bool bIncludeAndroid)
{
    uint32_t count = 0;

//    READ_LOCK(m_list_mutex);
    for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_UserList[i];

        if(pIServerUserItem)
        {
            if (pIServerUserItem->IsAndroidUser())
            {
                if(bIncludeAndroid)
                {
                    ++count;
                }
            }else
            {
                ++count;
            }
        }
    }
    return (count);
}

uint32_t CTableFrame::GetAndroidPlayerCount()
{
    uint32_t count = 0;

//    READ_LOCK(m_list_mutex);
    for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_UserList[i];

        // check user item now.
        if(pIServerUserItem  && pIServerUserItem->IsAndroidUser())
        {
            ++count;
        }
    }
    return (count);
}

uint32_t CTableFrame::GetMaxPlayerCount()
{
    return m_pGameRoomInfo->maxPlayerNum;
}

void CTableFrame::GetPlayerCount(uint32_t &nRealPlayerCount, uint32_t &nAndroidPlayerCount)
{
    nRealPlayerCount = 0;
    nAndroidPlayerCount = 0;

//    READ_LOCK(m_list_mutex);
    for (int i =0 ; i < m_pGameRoomInfo->maxPlayerNum; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_UserList[i];

        // check if the validate.
        if(pIServerUserItem)
        {
            // try to check if include android now.
            if(pIServerUserItem->IsAndroidUser())
                ++nAndroidPlayerCount;
            else
                ++nRealPlayerCount;
        }
    }
    return;
}

tagGameRoomInfo* CTableFrame::GetGameRoomInfo()
{
    return m_pGameRoomInfo;
}

//int64_t CTableFrame::GetUserEnterMinScore()
//{
//    return (m_pRoomKindInfo->nEnterMinScore);
//}

//int64_t CTableFrame::GetUserEnterMaxScore()
//{
//    return (m_pRoomKindInfo->nEnterMaxScore);
//}

//SCORE CTableFrame::GetGameCellScore()
//{
////    LOG_ERROR<<__FILE__ << " " <<__FUNCTION__ << " " << __LINE__ << " CELLSCORE="<<m_pRoomKindInfo->nCellScore;
//    return m_pRoomKindInfo->nCellScore;
//}

void CTableFrame::ClearTableUser(uint32_t chairId, bool bSendState, bool bSendToSelf, uint8_t cbSendErrorCode)
{
    //LOG_DEBUG << ">>> ClearTableUser tableid:" << m_TableState.nTableID << ", chairid:" << chairId;

    int kickType = KICK_GS;
    if((INVALID_CARD == chairId) && (ERROR_ENTERROOM_SERVERSTOP == cbSendErrorCode))
    {
        kickType |= (KICK_HALL);
    }

    if(chairId != INVALID_CHAIR && chairId < m_pGameRoomInfo->maxPlayerNum)
    {
        shared_ptr<CServerUserItem> pUserItem;
        {
//            READ_LOCK(m_list_mutex);
            pUserItem = m_UserList[chairId];
        }

        // check if user item exist.
        if(!pUserItem)
        {
            LOG_WARN << " >>> ClearTableUser Get UserItem is NULL, chairId:" << chairId;
            return;
        }

        int64_t userId  = pUserItem->GetUserId();
        uint32_t chairId = pUserItem->GetChairId();

        //LOG_DEBUG << __FILE__ << __FUNCTION__ << "ClearTableUser pUserItem->userId:"<<userId<<" table("<<m_TableState.nTableID<<","<<chairId<<")";

        OnUserStandup(pUserItem, bSendState);

        return;
    }
}

bool CTableFrame::IsAndroidUser(uint32_t chairId)
{
    bool bIsAndroid = false;
    if (chairId < m_pGameRoomInfo->maxPlayerNum)
    {
        shared_ptr<IServerUserItem> pIServerUserItem;
        {
//            READ_LOCK(m_list_mutex);
            pIServerUserItem = m_UserList[chairId];
        }

        if (pIServerUserItem)
        {
            bIsAndroid = pIServerUserItem->IsAndroidUser();
        }
    }
    return bIsAndroid;
}

bool CTableFrame::OnGameEvent(uint32_t chairId, uint8_t subId, const uint8_t *data, int size)
{
    bool bSuccess = m_pTableFrameSink->OnGameMessage(chairId, subId, data, size);
    if (!bSuccess)
    {
        LOG_WARN << "pTableFrameSink process message failed!, subid:" << subId;
    }
    return bSuccess;
}

void CTableFrame::onStartGame()
{
    //LOG_ERROR << " >>> CTableFrame::StartGame().";
}

bool CTableFrame::OnUserEnterAction(shared_ptr<CServerUserItem> &pIServerUserItem, Header *commandHeader, bool bDistribute)
{
    bool bRet = false;
    if(pIServerUserItem)
    {
        SendUserSitdownFinish(pIServerUserItem, commandHeader, bDistribute);


        //LOG_DEBUG << __FILE__ << __FUNCTION__ << "m_pTableFrameSink->OnUserEnter pIServerUserItem->GetUserID()"<<pIServerUserItem->GetUserId();
        bRet = m_pTableFrameSink->OnUserEnter(pIServerUserItem->GetUserId(), pIServerUserItem->isLookon());

    }else
    {
        LOG_WARN << "void CTableFrame::OnUserEnterAction(IServerUserItem *pIBaseUserItem, bool bDistribute, bool bWrite) pIServerUserItem=NULL";
    }
    return bRet;
}

bool CTableFrame::OnUserStandup(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendState, bool bSendToSelf)
{
    //LOG_ERROR<<__FILE__ << " " <<__FUNCTION__ << " " << __LINE__;
    bool bSuccess = false;
    do
    {
        if(!pIServerUserItem || pIServerUserItem->GetChairId() == INVALID_CHAIR || pIServerUserItem->GetUserId() == 0)
        {
            LOG_ERROR << "OnUserStandup find user failed!, userid:" << pIServerUserItem->GetUserId();
            break;
        }
        if(pIServerUserItem->isPlaying())
        {
            LOG_ERROR << "OnUserStandup pIServerUserItem->isPlaying(), userId:" << pIServerUserItem->GetUserId();
            break;
        }

        int64_t userId = pIServerUserItem->GetUserId();
        uint32_t chairId = pIServerUserItem->GetChairId();
        //LOG_INFO << "leaved userId:" << userId << " at tableid(" << m_TableState.nTableID << "," << chairId << ").";

        pIServerUserItem->SetTableId(INVALID_TABLE);
        pIServerUserItem->SetChairId(INVALID_CHAIR);
        if (chairId < m_UserList.size())
        {
            shared_ptr<CServerUserItem> cServerUserItem;
            m_UserList[chairId] = cServerUserItem;
        }
        m_pMatchRoom->UserOutTable(userId);
        m_pLogicThread->SavePlayGameTime(userId);
        bSuccess = true;

    }while(0);
    return bSuccess;
}

void CTableFrame::SendUserSitdownFinish(shared_ptr<CServerUserItem>& pIServerUserItem, Header *commandHeader, bool bDistribute)
{
//    if (!bDistribute)
    {
        int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
        int subId  = GameServer::SUB_S2C_ENTER_ROOM_RES;
        GameServer::MSG_S2C_UserEnterMessageResponse response;
        Game::Common::Header* header = response.mutable_header();
        header->set_sign(HEADER_SIGN);
//        header->set_crc(0);
//        header->set_mainid(mainid);
//        header->set_subid(subid);

//        enterSucc.set_gameid(m_pRoomKindInfo->gameId);
//        enterSucc.set_kindid(m_pRoomKindInfo->roomId);
//        enterSucc.set_roomkindid(m_pRoomKindInfo->roomId);
//        enterSucc.set_cellscore(m_pRoomKindInfo->ceilScore);

        string content = response.SerializeAsString();
        SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), commandHeader);
    }
}

void CTableFrame::BroadcastUserInfoToOther(shared_ptr<CServerUserItem> & pIServerUserItem)
{
    int datasize = 0;
    string strLoginfo;
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId  = GameServer::SUB_S2C_USER_ENTER_NOTIFY;
    do
    {
        // check my user item.
        if (!pIServerUserItem)
        {
            LOG_ERROR << "BroadcastUserInfo pIserverUserItem is NULL.";
            break;
        }

        GSUserBaseInfo& userInfo = pIServerUserItem->GetUserBaseInfo();
        GameServer::MSG_S2C_UserBaseInfo msgUserInfo;
        Game::Common::Header* header = msgUserInfo.mutable_header();
        header->set_sign(HEADER_SIGN);
//        header->set_crc(0);
//        header->set_mainid(mainid);
//        header->set_subid(subid);

        int64_t myUserId = pIServerUserItem->GetUserId();
        strLoginfo = "BroadcastUserInfo(SUB_S2C_USER_ENTER) score:" + to_string(userInfo.userScore) + ", myuid:" +
                to_string(myUserId) + ", other userlist:";

        msgUserInfo.set_userid(userInfo.userId);
        msgUserInfo.set_account(userInfo.account);
        msgUserInfo.set_nickname(userInfo.nickName);
        msgUserInfo.set_headindex(userInfo.headId);
        msgUserInfo.set_score(userInfo.userScore);
        msgUserInfo.set_location(userInfo.location);
        msgUserInfo.set_tableid(pIServerUserItem->GetTableId());
        msgUserInfo.set_chairid(pIServerUserItem->GetChairId());
        msgUserInfo.set_userstatus(pIServerUserItem->GetUserStatus());

        datasize = msgUserInfo.ByteSize();
        string content = msgUserInfo.SerializeAsString();

        for(int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
        {
            shared_ptr<CServerUserItem> pTargetUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pTargetUserItem = m_UserList[i];
            }

            if(pTargetUserItem)
            {
                int64_t targetUId = pTargetUserItem->GetUserId();
                SendPackedData(pTargetUserItem, mainId, subId, (uint8_t*)content.data(),content.size(), nullptr);
                strLoginfo += to_string(targetUId) + ",";
            }
        }
    }while(0);
    //LOG_WARN << strLoginfo << " ,size:" << datasize;
    return;
}

void CTableFrame::SendAllOtherUserInfoToUser(shared_ptr<CServerUserItem>& pIServerUserItem)
{
    string strLogInfo = "";
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    int subId = GameServer::SUB_S2C_USER_ENTER_NOTIFY;
    do
    {
        if(!pIServerUserItem)
            break;

        int mUId = pIServerUserItem->GetUserId();
        strLogInfo = "SendOtherUserInfoToUser mUId:" + to_string(mUId) + ", others uids:";
        for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
        {
            shared_ptr<CServerUserItem> pOtherUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pOtherUserItem = m_UserList[i];
            }

            //  loop to send all other user data to me.
            if ((pOtherUserItem != pIServerUserItem) &&
                (pOtherUserItem))

            {
                GSUserBaseInfo& userInfo = pOtherUserItem->GetUserBaseInfo();
                GameServer::MSG_S2C_UserBaseInfo msgUserInfo;
                Game::Common::Header* header = msgUserInfo.mutable_header();
                header->set_sign(HEADER_SIGN);
//                header->set_crc(0);
//                header->set_mainid(mainid);
//                header->set_subid(subid);

                // try to update hte special user info.
                msgUserInfo.set_userid(userInfo.userId);
                msgUserInfo.set_nickname(userInfo.nickName);
                msgUserInfo.set_location(userInfo.location);
                msgUserInfo.set_headindex(userInfo.headId);
                msgUserInfo.set_tableid(pOtherUserItem->GetTableId());
                msgUserInfo.set_chairid(pOtherUserItem->GetChairId());
                msgUserInfo.set_score(userInfo.userScore);
                msgUserInfo.set_userstatus(pOtherUserItem->GetUserStatus());

                string content = msgUserInfo.SerializeAsString();
                SendPackedData(pIServerUserItem, mainId , subId, (uint8_t*)content.data(), content.size(), nullptr);
            }
        }
//        dumpUserList();
    }while(0);
//    LOG_DEBUG << strLoginfo;
    return;
}

void CTableFrame::SendOtherUserInfoToUser(shared_ptr<CServerUserItem>& pServerUserItem, tagUserInfo &userInfo)
{
    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    int subId = GameServer::SUB_S2C_USER_ENTER_NOTIFY;

    if(pServerUserItem)
    {
        GameServer::MSG_S2C_UserBaseInfo msgUserInfo;
        Game::Common::Header* header = msgUserInfo.mutable_header();
        header->set_sign(HEADER_SIGN);

        msgUserInfo.set_userid(userInfo.userId);
        msgUserInfo.set_nickname(userInfo.nickName);
        msgUserInfo.set_location(userInfo.location);
        msgUserInfo.set_headindex(userInfo.headId);
        msgUserInfo.set_tableid(userInfo.tableId);
        msgUserInfo.set_chairid(userInfo.chairId);
        msgUserInfo.set_score(userInfo.score);
        msgUserInfo.set_userstatus(userInfo.status);

        string content = msgUserInfo.SerializeAsString();
        SendPackedData(pServerUserItem, mainId , subId, (uint8_t*)content.data(), content.size(), nullptr);
    }
    return;
}

void CTableFrame::BroadcastUserScore(shared_ptr<CServerUserItem>& pIServerUserItem)
{
//    char buf[1024]={0};
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId  = GameServer::SUB_S2C_USER_SCORE_NOTIFY;
    do
    {
        if (!pIServerUserItem)
            break;

        // try to get all the user score information content.
        GameServer::MSG_S2C_UserScoreInfo scoreinfo;
        Game::Common::Header* header = scoreinfo.mutable_header();
        header->set_sign(HEADER_SIGN);
//        header->set_crc(0);
//        header->set_mainid(mainid);
//        header->set_subid(subid);
        // try to update all user id and table id content.
        scoreinfo.set_userid(pIServerUserItem->GetUserId());
        scoreinfo.set_tableid(pIServerUserItem->GetTableId());
        scoreinfo.set_chairid(pIServerUserItem->GetChairId());
        scoreinfo.set_userscore(pIServerUserItem->GetUserScore());

        string content = scoreinfo.SerializeAsString();

        for (uint32_t i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
        {
            shared_ptr<CServerUserItem> pTargetUserItem;
            {
//                READ_LOCK(m_list_mutex);
                pTargetUserItem = m_UserList[i];
            }

            if (pTargetUserItem)
            {
                SendPackedData(pTargetUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), nullptr);
            }
        }

    }while(0);
    return;
}

void CTableFrame::BroadcastUserStatus(shared_ptr<CServerUserItem> & pIUserItem, bool bSendTySelf)
{
    //LOG_DEBUG << "BroadcastUserStatus chairid:" << pIUserItem->GetChairId() << ", status:" << pIUserItem->GetUserStatus() << ", userid:" << pIUserItem->GetUserId();
    uint8_t mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
    uint8_t subId = GameServer::SUB_S2C_USER_STATUS_NOTIFY;
    do
    {
        if (!pIUserItem)
        {
            LOG_INFO<<" pIUserItem == NULL";
            break;
        }

        GameServer::MSG_S2C_GameUserStatus UserStatus;
        Game::Common::Header* header = UserStatus.mutable_header();
        header->set_sign(HEADER_SIGN);
//        header->set_crc(0);
//        header->set_mainid(mainid);
//        header->set_subid(subid);

        UserStatus.set_tableid(pIUserItem->GetTableId());
        UserStatus.set_chairid(pIUserItem->GetChairId());
        UserStatus.set_userid(pIUserItem->GetUserId());
        UserStatus.set_usstatus(pIUserItem->GetUserStatus());
        {
            string content = UserStatus.SerializeAsString();
            for(uint32_t chairid = 0; chairid < m_pGameRoomInfo->maxPlayerNum; ++chairid)
            {
                shared_ptr<CServerUserItem> pTargetUserItem;
                {
//                    READ_LOCK(m_list_mutex);
                    pTargetUserItem = m_UserList[chairid];
                }

                // check the user item value now.
                if (!pTargetUserItem)
                {
//                    LOG_INFO<<"++++++++++++++++++++++++pTargetUserItem==NULL,chairid:" << chairid;
                    continue;
                }

                if(!bSendTySelf && pTargetUserItem == pIUserItem)
                {
//                    LOG_INFO<<"++++++++++++++++++++++++bSendTySelf==FALSE OR pTargetUserItem == pIUserItem";
                    continue;
                }

                SendPackedData(pTargetUserItem, mainId, subId, (uint8_t*)content.data(),content.size(), nullptr);
            }
        }
    }while(0);
    return;
}

bool CTableFrame::SaveKillFishBossRecord(GSKillFishBossInfo& bossinfo)
{
    return false ;
}
//设置个人库存控制等级
void CTableFrame::setkcBaseTimes(int kcBaseTimes[],double timeout,double reduction)
{

}
bool CTableFrame::WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, string &strRound)
{
    if(pScoreInfo && nCount > 0)
    {
        uint32_t chairId = 0;
        try
        {
            for(uint32_t i = 0; i < nCount; ++i)
            {
                tagScoreInfo *scoreInfo = (pScoreInfo + i);
                chairId = scoreInfo->chairId;

                shared_ptr<CServerUserItem> pIServerUserItem;
                {
    //                READ_LOCK(m_list_mutex);
                    pIServerUserItem = m_UserList[chairId];
                }

                if(pIServerUserItem)
                {
                    GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();
                    int64_t sourceScore = userBaseInfo.userScore;
                    int64_t targetScore = sourceScore + scoreInfo->addScore;
                    if(!pIServerUserItem->IsAndroidUser())
                    {
                        AddUserGameInfoToDB(userBaseInfo,scoreInfo, strRound, nCount, false);
                        AddUserGameLogToDB(userBaseInfo, scoreInfo, strRound);
                        //LOG_DEBUG << "AddUserScore userId:" << userBaseInfo.userId << ", Source Score:" << sourceScore << ", AddScore:" << scoreInfo->addScore << ", lNewScore:" << targetScore;
                    }
                    pIServerUserItem->SetUserScore(targetScore);
                }else
                {
                    LOG_WARN << " >>> WriteUserScore pIServerUserItem is NULL! <<<";
                    continue;
                }
            }
//            dbSession.commit_transaction();
        }catch(exception &e)
        {
//            dbSession.abort_transaction();
            LOG_ERROR << "MongoDB WriteData Error:"<<e.what();
        }
    }
    return true;
}

bool CTableFrame::WriteSpecialUserScore(tagSpecialScoreInfo* pSpecialScoreInfo, uint32_t nCount, string &strRound)
{
    if(pSpecialScoreInfo && nCount > 0)
    {
        int64_t userId = 0;
        uint32_t chairId = 0;
        mongocxx::client_session dbSession = MONGODBCLIENT.start_session();
        dbSession.start_transaction();
        try
        {
            for(uint32_t i = 0; i < nCount; ++i)
            {
                // try to get the special user score info.
                tagSpecialScoreInfo *scoreInfo = (pSpecialScoreInfo + i);

                userId = scoreInfo->userId;
                chairId = scoreInfo->chairId;
                if(scoreInfo->bWriteScore)
                {
                    shared_ptr<CServerUserItem> pIServerUserItem;
                    {
                        pIServerUserItem = m_UserList[chairId];
                    }
                    if(pIServerUserItem)
                    {
                        GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();
                        int64_t sourceScore = userBaseInfo.userScore;
                        int64_t targetScore = sourceScore + scoreInfo->addScore;
                        if(!pIServerUserItem->IsAndroidUser())
                        {
                            scoreInfo->lineCode=userBaseInfo.lineCode;
                            AddUserGameLogToDB(scoreInfo, strRound);

                        }else//is android
                        {
                            if(m_pGameInfo->gameType == GameType_Confrontation)
                            {
                            }
                        }
                        pIServerUserItem->SetUserScore(targetScore);
                    }
                }

                if(scoreInfo->bWriteRecord)
                {
                    AddUserGameInfoToDB(scoreInfo, strRound, nCount, false);
                }
            }
            dbSession.commit_transaction();
        }catch(exception &e)
        {
            dbSession.abort_transaction();
            LOG_ERROR << "MongoDB WriteData Error:"<<e.what();
        }
    }
    return true;
}

bool CTableFrame::UpdateUserScoreToDB(int64_t userId, tagScoreInfo* pScoreInfo)
{
    bool bSuccess = false;
//    try
//    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                        <<"gamerevenue" << pScoreInfo->revenue
                        <<"winorlosescore" << pScoreInfo->addScore
                        <<"score" << pScoreInfo->addScore << close_document
                        << finalize);
        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}

bool CTableFrame::UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo)
{
    bool bSuccess = false;
//    try
//    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                        <<"gamerevenue" << pScoreInfo->revenue
                        <<"winorlosescore" << pScoreInfo->addScore
                        <<"score" << pScoreInfo->addScore << close_document
                        << finalize);
        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}

bool CTableFrame::AddUserGameInfoToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
{
    bool bSuccess = false;
//    try
//    {
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                << "matchinfoid" << m_pMatchRoom->getMatchRoundId()
                << "gameinfoid" << strRoundId
                << "userid" << userBaseInfo.userId
                << "account" << userBaseInfo.account
                << "agentid" << (int32_t)userBaseInfo.agentId
                << "linecode" << userBaseInfo.lineCode
                << "gameid" << (int32_t)m_pGameInfo->gameId
                << "roomid" << (int32_t)m_pGameRoomInfo->roomId
                << "tableid" << (int32_t)m_TableState.nTableID
                << "chairid" << (int32_t)scoreInfo->chairId
                << "isBanker" << (int32_t)scoreInfo->isBanker
                << "cardvalue" << scoreInfo->cardValue
                << "usercount" << userCount
                << "beforescore" << userBaseInfo.userScore
                << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
                << "cellscore" << open_array;
        for(int64_t &betscore : scoreInfo->cellScore)
            insert_value = insert_value << betscore;
        auto after_array = insert_value << close_array;
        after_array
                << "allbet" << scoreInfo->betScore
                << "winscore" << scoreInfo->addScore
                << "score" << userBaseInfo.userScore + scoreInfo->addScore
                << "revenue" << scoreInfo->revenue
                << "isandroid" << bAndroidUser
                << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
                << "gameendtime" << bsoncxx::types::b_date(chrono::system_clock::now());

        auto doc = after_array << bsoncxx::builder::stream::finalize;

//        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(doc);

#if 1
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_play_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
        if(result)
        {
        }
#else
        m_pLogicThread->RunWriteDataToMongoDB("gamemain", "play_record", INSERT_ONE, doc.view(), bsoncxx::document::view());
#endif

        bSuccess = true;
//        try
//        {
//            auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//            conn->setSchema(DB_RECORD);
//            pstmt.reset(conn->prepareStatement("INSERT INTO match_play_record(id, matchid, "
//                                               "gameid,userid,account,agentid,linecode,"
//                                               "gameid,roomid,tableid,"
//                                               "chairid,isBanker,cardvalue,usercount,beforescore,"
//                                               "cellscore,allbet,winscore,score,revenue,isandroid,gamestarttime,gameendtime) "
//                                               "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
//            pstmt->setString(1, m_pMatchRoom->getMatchRoundId());
//            pstmt->setString(2, strRoundId);
//            pstmt->setInt64(3, userBaseInfo.userId);
//            pstmt->setString(4,userBaseInfo.account);
//            pstmt->setInt(5, userBaseInfo.agentId);
//            pstmt->setString(6, userBaseInfo.lineCode);
//            pstmt->setInt(7, m_pGameInfo->gameId);
//            pstmt->setInt(8, m_pGameRoomInfo->roomId);

//            pstmt->setInt(9, m_TableState.nTableID);
//            pstmt->setInt(10, scoreInfo->chairId);
//            pstmt->setInt(11, scoreInfo->isBanker);
//            pstmt->setString(12, scoreInfo->cardValue);
//            pstmt->setInt(13, userCount);
//            pstmt->setInt64(14,userBaseInfo.userScore);
//            pstmt->setInt64(15, scoreInfo->rWinScore);
//            string cellscore;
//            for(int i =0;(int)scoreInfo->cellScore.size();i++)
//            {
//                if(i<scoreInfo->cellScore.size()-1)
//                {
//                    cellscore=to_string(scoreInfo->cellScore[i])+"|";
//                }
//                else
//                {
//                    cellscore=to_string(scoreInfo->cellScore[i]);
//                }
//            }

//            pstmt->setString(16, cellscore);
//            pstmt->setInt64(17, scoreInfo->betScore);
//            pstmt->setInt64(18, scoreInfo->addScore);
//            pstmt->setInt64(19,userBaseInfo.userScore + scoreInfo->addScore);
//            pstmt->setInt64(20, scoreInfo->revenue);
//            pstmt->setInt(21, bAndroidUser);
//            pstmt->setDateTime(22,GlobalFunc::InitialConversion(std::chrono::system_clock::to_time_t(scoreInfo->startTime)));
//            pstmt->setDateTime(23, GlobalFunc::InitialConversion(time(NULL)));
//            pstmt->executeUpdate();

//        }
//        catch (sql::SQLException &e)
//        {
//            std::cout << " (MySQL error code: " << e.getErrorCode();
//        }
    return bSuccess;
}

bool CTableFrame::AddUserGameInfoToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
{
    bool bSuccess = false;
//    try
//    {
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                << "matchinfoid" << m_pMatchRoom->getMatchRoundId()
                << "gameinfoid" << strRoundId
                << "userid" << scoreInfo->userId
                << "account" << scoreInfo->account
                << "agentid" << (int32_t)scoreInfo->agentId
                << "linecode" << scoreInfo->lineCode
                << "gameid" << (int32_t)m_pGameInfo->gameId
                << "roomid" << (int32_t)m_pGameRoomInfo->roomId
                << "tableid" << (int32_t)m_TableState.nTableID
                << "chairid" << (int32_t)scoreInfo->chairId
                << "isBanker" << (int32_t)scoreInfo->isBanker
                << "cardvalue" << scoreInfo->cardValue
                << "usercount" << userCount
                << "beforescore" << scoreInfo->beforeScore
                << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
                << "cellscore" << open_array;
        for(int64_t &betscore : scoreInfo->cellScore)
            insert_value = insert_value << betscore;
        auto after_array = insert_value << close_array;
        after_array
                << "allbet" << scoreInfo->betScore
                << "winscore" << scoreInfo->addScore
                << "score" << scoreInfo->beforeScore + scoreInfo->addScore
                << "revenue" << scoreInfo->revenue
                << "isandroid" << bAndroidUser
                << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
                << "gameendtime" << bsoncxx::types::b_date(chrono::system_clock::now());

        auto doc = after_array << bsoncxx::builder::stream::finalize;

//        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(doc);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_play_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
        if(result)
        {
        }

        bSuccess = true;
//        try
//        {
//            auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//            conn->setSchema(DB_RECORD);
//            pstmt.reset(conn->prepareStatement("INSERT INTO match_play_record(id, matchid, "
//                                               "gameid,userid,account,agentid,linecode,"
//                                               "gameid,roomid,tableid,"
//                                               "chairid,isBanker,cardvalue,usercount,beforescore,"
//                                               "cellscore,allbet,winscore,score,revenue,isandroid,gamestarttime,gameendtime) "
//                                               "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
//            pstmt->setString(1, m_pMatchRoom->getMatchRoundId());
//            pstmt->setString(2, strRoundId);
//            pstmt->setInt64(3, scoreInfo->userId);
//            pstmt->setString(4,scoreInfo->account);
//            pstmt->setInt(5, scoreInfo->agentId);
//            pstmt->setString(6, scoreInfo->lineCode);
//            pstmt->setInt(7, m_pGameInfo->gameId);
//            pstmt->setInt(8, m_pGameRoomInfo->roomId);

//            pstmt->setInt(9, m_TableState.nTableID);
//            pstmt->setInt(10, scoreInfo->chairId);
//            pstmt->setInt(11, scoreInfo->isBanker);
//            pstmt->setString(12, scoreInfo->cardValue);
//            pstmt->setInt(13, userCount);
//            pstmt->setInt64(14,scoreInfo->beforeScore);
//            pstmt->setInt64(15, scoreInfo->rWinScore);
//            string cellscore;
//            for(int i =0;(int)scoreInfo->cellScore.size();i++)
//            {
//                if(i<scoreInfo->cellScore.size()-1)
//                {
//                    cellscore=to_string(scoreInfo->cellScore[i])+"|";
//                }
//                else
//                {
//                    cellscore=to_string(scoreInfo->cellScore[i]);
//                }
//            }

//            pstmt->setString(16, cellscore);
//            pstmt->setInt64(17, scoreInfo->betScore);
//            pstmt->setInt64(18, scoreInfo->addScore);
//            pstmt->setInt64(19,scoreInfo->beforeScore + scoreInfo->addScore);
//            pstmt->setInt64(20, scoreInfo->revenue);
//            pstmt->setInt(21, bAndroidUser);
//            pstmt->setDateTime(22,GlobalFunc::InitialConversion(std::chrono::system_clock::to_time_t(scoreInfo->startTime)));
//            pstmt->setDateTime(23, GlobalFunc::InitialConversion(time(NULL)));
//            pstmt->executeUpdate();

//        }
//        catch (sql::SQLException &e)
//        {
//            std::cout << " (MySQL error code: " << e.getErrorCode();
//        }
    return bSuccess;
}

bool CTableFrame::AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore)
{
    bool bSuccess = false;

        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << userBaseInfo.userId
                << "account" << userBaseInfo.account
                << "agentid" << (int32_t)userBaseInfo.agentId
                << "linecode" << userBaseInfo.lineCode
                << "changetype" << (int32_t)m_pGameRoomInfo->roomId
                << "changemoney" << addScore
                << "beforechange" << sourceScore
                << "afterchange" << targetScore
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

#if 1
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_match_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if(result)
        {
        }
#else
        m_pLogicThread->RunWriteDataToMongoDB("gamemain", "user_score_record", INSERT_ONE, insert_value.view(), bsoncxx::document::view());
#endif
//        try
//        {
//            auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//            conn->setSchema(DB_RECORD);
//            pstmt.reset(conn->prepareStatement("INSERT INTO match_play_record(id, userid, "
//                                               "account,agentid,linecode,changetype,changemoney,"
//                                               "beforechange,afterchange,logdate)"
//                                               "VALUES(?,?,?,?,?,?,?,?,?)"));
//            pstmt->setInt64(1, userBaseInfo.userId);
//            pstmt->setString(2,userBaseInfo.account);
//            pstmt->setInt(3, userBaseInfo.agentId);
//            pstmt->setString(4, userBaseInfo.lineCode);
//            pstmt->setInt(5, m_pGameRoomInfo->roomId);
//            pstmt->setInt64(6, addScore);

//            pstmt->setInt64(7, sourceScore);
//            pstmt->setInt64(8, targetScore);
//            pstmt->setDateTime(9, GlobalFunc::InitialConversion(time(NULL)));
//            pstmt->executeUpdate();

//        }
//        catch (sql::SQLException &e)
//        {
//            std::cout << " (MySQL error code: " << e.getErrorCode();
//        }
        bSuccess = true;

    return bSuccess;
}

bool CTableFrame::AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo)
{
    bool bSuccess = false;

        auto insert_value = bsoncxx::builder::stream::document{}
                << "userid" << scoreInfo->userId
                << "account" << scoreInfo->account
                << "agentid" << (int32_t)scoreInfo->agentId
                << "linecode" << scoreInfo->lineCode
                << "changetype" << (int32_t)m_pGameRoomInfo->roomId
                << "changemoney" << scoreInfo->addScore
                << "beforechange" << scoreInfo->beforeScore
                << "afterchange" << scoreInfo->beforeScore + scoreInfo->addScore
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if(result)
        {
        }

        bSuccess = true;
//        try
//        {
//            auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//            conn->setSchema(DB_RECORD);
//            pstmt.reset(conn->prepareStatement("INSERT INTO match_play_record(id, userid, "
//                                               "account,agentid,linecode,changetype,changemoney,"
//                                               "beforechange,afterchange,logdate)"
//                                               "VALUES(?,?,?,?,?,?,?,?,?)"));
//            pstmt->setInt64(1, scoreInfo->userId);
//            pstmt->setString(2,scoreInfo->account);
//            pstmt->setInt(3, (int32_t)scoreInfo->agentId);
//            pstmt->setString(4, scoreInfo->lineCode);
//            pstmt->setInt(5, (int32_t)m_pGameRoomInfo->roomId);
//            pstmt->setInt64(6, scoreInfo->addScore);

//            pstmt->setInt64(7,  scoreInfo->beforeScore);
//            pstmt->setInt64(8, scoreInfo->beforeScore + scoreInfo->addScore);
//            pstmt->setDateTime(9, GlobalFunc::InitialConversion(time(NULL)));
//            pstmt->executeUpdate();
//        }
//        catch (sql::SQLException &e)
//        {
//            std::cout << " (MySQL error code: " << e.getErrorCode();
//        }
    return bSuccess;
}

bool CTableFrame::AddUserGameLogToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId)
{
    bool bSuccess = false;
//    try
//    {
        auto insert_value = bsoncxx::builder::stream::document{}
                << "gamelogid" << strRoundId
                << "userid" << userBaseInfo.userId
                << "account" << userBaseInfo.account
                << "agentid" << (int32_t)userBaseInfo.agentId
                << "linecode" << userBaseInfo.lineCode
                << "gameid" << (int32_t)m_pGameInfo->gameId
                << "roomid" << (int32_t)m_pGameRoomInfo->roomId
                << "winscore" << scoreInfo->addScore
                << "revenue" << scoreInfo->revenue
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

#if 1
        // mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_game_log"];
        // bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        // if(result)
        // {
        // }
#else
        m_pLogicThread->RunWriteDataToMongoDB(string("gamemain"), string("game_log"), INSERT_ONE, insert_value.view(), bsoncxx::document::view());
#endif

        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}

bool CTableFrame::AddUserGameLogToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId)
{
    bool bSuccess = false;
//    try
//    {
        auto insert_value = bsoncxx::builder::stream::document{}
                << "gamelogid" << strRoundId
                << "userid" << scoreInfo->userId
                << "account" << scoreInfo->account
                << "agentid" << (int32_t)scoreInfo->agentId
                << "linecode" << scoreInfo->lineCode
                << "gameid" << (int32_t)m_pGameInfo->gameId
                << "roomid" << (int32_t)m_pGameRoomInfo->roomId
                << "winscore" << scoreInfo->addScore
                << "revenue" << scoreInfo->revenue
                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

        // mongocxx::collection coll = MONGODBCLIENT["gamemain"]["match_game_log"];
        // bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        // if(result)
        // {
        // }

        bSuccess = true;
//    }catch(exception &e)
//    {
//        LOG_ERROR << "exception: " << e.what();
//        throw;
//    }
    return bSuccess;
}


//===========================STORAGE SCORE===========================
int CTableFrame::UpdateStorageScore(int64_t changeStockScore)
{
    //比赛场不需要库存
    return 0;
}

bool CTableFrame::GetStorageScore(tagStorageInfo &storageInfo)
{
    storageInfo.lLowlimit = m_LowestStock;
    storageInfo.lUplimit = m_HighStock;

    storageInfo.lEndStorage = m_CurStock;
    int8_t storageStatus = 0;
    if(m_pMatchRoom)
        storageStatus=m_pMatchRoom->GetStorageStatus();

    if(storageStatus==1)
    {
        storageInfo.lEndStorage = m_HighStock + (m_HighStock-m_LowestStock)/2;
    }else if(storageStatus==-1)
    {
        storageInfo.lEndStorage = m_LowestStock - (m_HighStock-m_LowestStock)/2;
    }else
    {
        storageInfo.lEndStorage = (m_HighStock-m_LowestStock)/2 +m_LowestStock;
    }

    storageInfo.lSysAllKillRatio = m_SystemAllKillRatio;
    storageInfo.lReduceUnit = m_SystemReduceRatio;
    storageInfo.lSysChangeCardRatio = m_ChangeCardRatio;

    return true;
}

bool CTableFrame::ReadStorageScore()
{
    m_CurStock = m_pGameRoomInfo->totalStock;
    m_LowestStock = m_pGameRoomInfo->totalStockLowerLimit;
    m_HighStock = m_pGameRoomInfo->totalStockHighLimit;

    m_SystemAllKillRatio = m_pGameRoomInfo->systemKillAllRatio;
    m_SystemReduceRatio = m_pGameRoomInfo->systemReduceRatio;
    m_ChangeCardRatio = m_pGameRoomInfo->changeCardRatio;
    return true;
}

bool CTableFrame::WriteGameChangeStorage(int64_t changeStockScore)
{
    return true;
}

bool CTableFrame::SaveReplay(tagGameReplay replay) {
    return replay.saveAsStream ? 
        SaveReplayDetailBlob(replay) :
        SaveReplayDetailJson(replay);
}

bool CTableFrame::SaveReplayDetailBlob(tagGameReplay& replay)
{
    if(replay.players.size() == 0 || !replay.players[0].flag)
    {
        return  false;
    }
	bsoncxx::builder::stream::document builder{};
	auto insert_value = builder
        //<< "gameid" << replay.gameid
		<< "gameinfoid" << replay.gameinfoid
		<< "timestamp" << bsoncxx::types::b_date{ chrono::system_clock::now() }
		<< "roomname" << replay.roomname
		<< "cellscore" << replay.cellscore
        << "detail" << bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, uint32_t(replay.detailsData.size()), reinterpret_cast<uint8_t const*>(replay.detailsData.data()) }
        << "players" << open_array;
		
    for(tagReplayPlayer &player : replay.players)
    {
        if(player.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "userid" << player.userid
                                        << "account" << player.accout
                                        << "score" << player.score
                                        << "chairid" << player.chairid
                                        << bsoncxx::builder::stream::close_document;
        }
    }
    insert_value << close_array << "steps" << open_array;
    for(tagReplayStep &step : replay.steps)
    {
        if(step.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "bet" << step.bet
                                        << "time" << step.time
                                        << "ty" << step.ty
                                        << "round" << step.round
                                        << "chairid" << step.chairId
                                        << "pos" << step.pos
                                        << bsoncxx::builder::stream::close_document;
        }
    }

    insert_value << close_array << "results" << open_array;

    for(tagReplayResult &result : replay.results)
    {
        if(result.flag)
        {
            insert_value = insert_value << bsoncxx::builder::stream::open_document
                                        << "chairid" << result.chairId
                                        << "bet" << result.bet
                                        << "pos" << result.pos
                                        << "win" << result.win
                                        << "cardtype" << result.cardtype
                                        << "isbanker" << result.isbanker
                                        << bsoncxx::builder::stream::close_document;
        }
    }
    auto after_array = insert_value << close_array;

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    mongocxx::collection coll = MONGODBCLIENT["gamelog"]["match_game_replay"];
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());


//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });


//        string playersStr,stepsStr,resultsStr;

//        for(tagReplayPlayer &player : replay.players)
//        {
//            playersStr+=to_string(player.userid)+","+to_string(player.accout)+","+to_string(player.score)
//                    +","+to_string(player.chairid)+"|";
//        }
//        for(tagReplayStep &step : replay.steps)
//        {
//            stepsStr+=to_string(step.bet)+","+o_string(step.time)+","+o_string(step.ty)
//                    +","+o_string(step.round)+","+o_string(step.chairId)+","+o_string(step.pos)+"|";
//        }
//        for(tagReplayResult &result : replay.results)
//        {
//            resultsStr+=to_string(result.chairId)+","+to_string(result.bet)+","+to_string(result.pos)
//                    +","+to_string(result.win)+","+to_string(result.cardtype)+","+to_string(result.isbanker)+"|";
//        }
//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO match_game_replay(gameinfoid,"
//                                           "timestamp,roomname,cellscore,players,steps,"
//                                           "results)"
//                                           "VALUES(?,?,?,?,?,?,?)"));
//        pstmt->setString(1, replay.gameinfoid);
//        pstmt->setDateTime(2,GlobalFunc::InitialConversion(time(NULL)));
//        pstmt->setString(3, replay.roomname);
//        pstmt->setInt(4, replay.cellscore);
//        pstmt->setString(5, playersStr);
//        pstmt->setString(6, stepsStr);
//        pstmt->setString(7,  resultsStr);
//        pstmt->executeUpdate();
//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }

    return true;
}

bool CTableFrame::SaveReplayDetailJson(tagGameReplay& replay)
{
	if (replay.players.size() == 0 || !replay.players[0].flag)
	{
		return  false;
	}
	bsoncxx::builder::stream::document builder{};
	auto insert_value = builder
        //<< "gameid" << replay.gameid
		<< "gameinfoid" << replay.gameinfoid
		<< "timestamp" << bsoncxx::types::b_date{ chrono::system_clock::now() }
		<< "roomname" << replay.roomname
		<< "cellscore" << replay.cellscore
        << "detail" << std::move(replay.detailsData)
	    << "players" << open_array;

	for (tagReplayPlayer& player : replay.players)
	{
		if (player.flag)
		{
			insert_value = insert_value << bsoncxx::builder::stream::open_document
				<< "userid" << player.userid
				<< "account" << player.accout
				<< "score" << player.score
				<< "chairid" << player.chairid
				<< bsoncxx::builder::stream::close_document;
		}
	}
	insert_value << close_array << "steps" << open_array;
	for (tagReplayStep& step : replay.steps)
	{
		if (step.flag)
		{
			insert_value = insert_value << bsoncxx::builder::stream::open_document
				<< "bet" << step.bet
				<< "time" << step.time
				<< "ty" << step.ty
				<< "round" << step.round
				<< "chairid" << step.chairId
				<< "pos" << step.pos
				<< bsoncxx::builder::stream::close_document;
		}
	}

	insert_value << close_array << "results" << open_array;

	for (tagReplayResult& result : replay.results)
	{
		if (result.flag)
		{
			insert_value = insert_value << bsoncxx::builder::stream::open_document
				<< "chairid" << result.chairId
				<< "bet" << result.bet
				<< "pos" << result.pos
				<< "win" << result.win
				<< "cardtype" << result.cardtype
				<< "isbanker" << result.isbanker
				<< bsoncxx::builder::stream::close_document;
		}
	}
	auto after_array = insert_value << close_array;

	auto doc = after_array << bsoncxx::builder::stream::finalize;

	mongocxx::collection coll = MONGODBCLIENT["gamelog"]["match_game_replay"];
	bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
	return true;
}

void CTableFrame::DeleteUserToProxy(shared_ptr<CServerUserItem>& pIServerUserItem, int32_t nKickType)
{
    do
    {
        if (!pIServerUserItem)
            break;

        int64_t userId = pIServerUserItem->GetUserId();
        //LOG_DEBUG << ">>> DeleteUserToProxy userid:" << userId;

        if (m_pLogicThread)
        {
            m_pLogicThread->KickUserIdToProxyOffLine(userId,nKickType);
        }

    } while (0);
//Cleanup:
    return;
}

bool CTableFrame::DelUserOnlineInfo(int64_t userId, bool bonlyExpired)
{
	LOG_DEBUG << "--- *** " << "userid = " << userId;

    bool b1 = false, b2 = false;
    do
    {
        string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
        if (bonlyExpired)
        {
//            b2 = redisClient->resetExpiredUserIdGameServerInfo(userid);
//            b1 = redisClient->resetExpired(strKeyName);

            b1 = REDISCLIENT.ResetExpiredUserOnlineInfo(userId); 
            //          << "online key:" << strKeyName << ", userid:" << userId;
        }
        else
        {
//            b2 = redisClient->delUserIdGameServerInfo(userId);
//            b1 = redisClient->del(strKeyName);
            if(REDISCLIENT.ExistsUserLoginInfo(userId)){
                //LOG_ERROR<<"not instance";
            }

            b1 = REDISCLIENT.DelUserOnlineInfo(userId);
        }

    }while (0);
//Cleanup:
    return (b1);
}

bool CTableFrame::SetUserOnlineInfo(int64_t userId)
{
    bool bSuccess = false;
    uint32_t ntable = INVALID_TABLE;
    REDISCLIENT.SetUserOnlineInfo(userId, m_pGameRoomInfo->gameId, m_pGameRoomInfo->roomId,ntable);
    LOG_WARN << "---user["<< userId <<"] playing gameId[" << m_pGameRoomInfo->gameId <<"],roomId["<< m_pGameRoomInfo->roomId<<"]";
    return (bSuccess);
}

bool CTableFrame::RoomSitChair(shared_ptr<CServerUserItem>& pIServerUserItem, Header *commandHeader, bool bDistribute)
{
//    muduo::MutexLockGuard lockSit(m_lockSitMutex);
//    muduo::RecursiveLockGuard lock(m_RecursiveMutextLockEnterLeave);
    //LOG_DEBUG << __FILE__ << __FUNCTION__ <<  "RoomSitChair userId:" << pIServerUserItem->GetUserId();

    do
    {
        if(!pIServerUserItem || pIServerUserItem->GetUserId() <= 0)
        {
            break;
        }

        uint32_t chairId = 0;
        uint32_t maxPlayerNum = m_pGameRoomInfo->maxPlayerNum;
        uint32_t startIndex = 0;
        if(m_pGameInfo->gameType == GameType_Confrontation)
            startIndex = GlobalFunc::RandomInt64(0, maxPlayerNum-1);
        for(uint32_t i = 0; i < maxPlayerNum; ++i)
        {
            {
                chairId = (startIndex + i) % maxPlayerNum;
//                WRITE_LOCK(m_list_mutex);
                if(m_UserList[chairId])
                    continue;
                // set the user sit status item content.
                pIServerUserItem->SetTableId(m_TableState.nTableID);
                pIServerUserItem->SetChairId(chairId);
                m_UserList[chairId] = pIServerUserItem;
            }

            //LOG_DEBUG << "roomsit chairId:" << chairId << ", userId:" << pIServerUserItem->GetUserId();
            pIServerUserItem->setClientReady(true);

//            // dump list.
//            dumpUserList();
//            // udpate player status to playing.
//            if(m_pRoomKindInfo->bisDynamicJoin)
//            {
//                if(m_pRoomKindInfo->bisEnterIsReady)
//                {
//                    pIServerUserItem->SetUserStatus(sReady);
//                }else
//                {
//                    pIServerUserItem->SetUserStatus(sPlaying);
//                }
//            }else if(m_pRoomKindInfo->bisAutoReady)
//            {
//                pIServerUserItem->SetUserStatus(sReady);
//            }else
//            {
                pIServerUserItem->SetUserStatus(sSit);
                //LOG_ERROR << "Sit1";
//            }

            OnUserEnterAction(pIServerUserItem, commandHeader, bDistribute);

            if(!pIServerUserItem->IsAndroidUser())
            {
                SetUserOnlineInfo(pIServerUserItem->GetUserId());
            }

            int64_t userId = pIServerUserItem->GetUserId();
            int64_t userScore = pIServerUserItem->GetUserScore();
            //LOG_WARN << ">>> sit down userId: " << userId
            //         << " at table id(" << m_TableState.nTableID << "," << chairId
            //         << "), score:" << userScore << ", pTable:" << this
            //         << ", pIServerUserItem:" << pIServerUserItem.get();
            return true;
        }
    }while(0);

    return false;
}



bool CTableFrame::IsLetAndroidOffline(shared_ptr<CServerUserItem>& pIServerUserItem)
{
    bool bSuccess = false;
//    do
//    {
//        if (!pIServerUserItem->IsAndroidUser()) break;

//        dword wLimitCount = 0;
//        // if (m_pRoomKindInfo->wGameID == GAME_BR)
//        if (m_pRoomKindInfo->isDynamicJoin)
//        {
//            wLimitCount = 20 + rand() % 50;
//        }
//        else
//        {
//            wLimitCount = 8 + rand() % 15;
//        }

//        if (pIServerUserItem->GetPlayCount() >= wLimitCount)
//        {
//            OnUserStandup(pIServerUserItem, true);
////            AndroidUserManager::Singleton().DeleteAndroidUserItem(pIServerUserItem);
//            m_dwAndroidLeaveTime = (dword)time(NULL);
//            return true;
//        }
//    } while (0);
    return (bSuccess);
}

void CTableFrame::LeftExcessAndroid()
{
//    READ_LOCK(m_list_mutex);
    for (int i = 0; i < m_pGameRoomInfo->maxPlayerNum; ++i)
    {
        shared_ptr<CServerUserItem> pIServerUserItem = m_UserList[i];

        // check user item now.
        if(pIServerUserItem  && pIServerUserItem->IsAndroidUser()&&pIServerUserItem->GetUserStatus()!=sOffline)
        {
            OnUserLeft(pIServerUserItem);
            break;
        }
    }
}

bool CTableFrame::SendUnpackData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                                const uint8_t* data, int size)
{
    bool bSuccess = false;

    // LOG_INFO << "SendUnpackData data mainid:" << mainid << ", subid:" << subid << " data size:" << size;
    if(pIServerUserItem)
        bSuccess = pIServerUserItem->SendUserMessage(mainId, subId, data, size);
    return (bSuccess);
}

bool CTableFrame::SendPackedData(shared_ptr<CServerUserItem>& pIServerUserItem, uint8_t mainId, uint8_t subId,
                                 const uint8_t* data, int size, Header *commandHeader)
{
    bool bSuccess = false;

    do
    {
        vector<unsigned char> vecdata(size);
        memmove(&vecdata[0],data,size);

        int enctype = PUBENC__PROTOBUF_AES; // no encrypt.
        if(g_bisDisEncrypt)
        {
            enctype = PUBENC__PROTOBUF_NONE;
        }

        bSuccess = sendData(pIServerUserItem, mainId, subId, vecdata, commandHeader, enctype);
    }   while (0);
//Cleanup:
    return (bSuccess);
}

bool CTableFrame::DBRecordUserInout(shared_ptr<CServerUserItem>& pIserverUserItem, bool bisOut, int nReason)
{
//    LOG_ERROR << " >>> DBRecordUserInout NO IMPL yet!";
//    return true;

    bool bSuccess = false;
    try
    {


    }catch(exception& e)
    {
        LOG_ERROR << "SQLException: " << e.what();
    }

//Cleanup:
    return (bSuccess);
}

int CTableFrame::QueryInterface(const char* guid, void** ptr, int ver)
{
    int status = -1;
    if(ptr)
        *ptr= 0;

    string strGuid = guid;
    if(boost::iequals(strGuid, string(guid_ISaveReplay)))
    {
        if(ptr)
            *ptr = static_cast<ISaveReplayRecord*>(this);
        status = 0;
    }
//Cleanup:
    return (status);
}

// try to filter the special game name value.
std::string filterName(std::string room_name)
{
    const char* lstKey[] = {"抢庄牛牛", "斗地主", "炸金花"};
    for (int i = 0; i < CountArray(lstKey); i++)
    {
        std::string key = lstKey[i];
        int pos = room_name.find(key);
        if (pos != room_name.npos)
        {
            room_name.replace(pos, key.length(), "");
        }
    }
//Cleanup:
    return (room_name);
}

// try to save the special replay record content item value.
bool CTableFrame::SaveReplayRecord(tagGameRecPlayback& rec)
{

    return true;
}

// dump the user list item value.
void CTableFrame::dumpUserList()
{
//    READ_LOCK(m_mutex);
    //LOG_DEBUG << "dump user list:";
    int size = m_UserList.size();
    for (int i = 0; i < size; ++i)
    {
        void* ptr = 0;
        int64_t userId = 0;
        shared_ptr<CServerUserItem> pIServerUserItem = m_UserList[i];
        if (pIServerUserItem != NULL)
        {
            userId = pIServerUserItem->GetUserId();
            ptr = (void*)pIServerUserItem.get();
        }

        //LOG_DEBUG << "   tableid:[" << m_TableState.nTableID << "],  chairid:[" << i
        //          << "], userid:" << userId << ", pIServerUserItem:" << ptr;
    }
}





