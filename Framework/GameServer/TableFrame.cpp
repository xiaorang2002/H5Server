#include <sys/time.h>
#include <stdarg.h>


#include <stdio.h>
#include <sstream>
#include <time.h>
#include <sys/timeb.h>
#include <fstream>
#include <ios>


#include <boost/proto/detail/ignore_unused.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/tuple/tuple.hpp>

#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <bsoncxx/oid.hpp>
#include <bsoncxx/stdx/optional.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/stdx.hpp>
#include <mongocxx/uri.hpp>



#include <atomic>

#include <boost/bind.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <muduo/base/Logging.h>

#include "public/SubNetIP.h"

#include "json/json.h"
#include "crypto/crypto.h"

#include "public/Globals.h"
#include "public/gameDefine.h"

// #include "public/GameGlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
//#include "EntryPtr.h"

#include "TableFrame.h"

#include "AndroidUserManager.h"
#include "ServerUserManager.h"
#include "GameTableManager.h"

#include "public/ThreadLocalSingletonMongoDBClient.h"
#include "public/ThreadLocalSingletonRedisClient.h"
#include "proto/GameServer.Message.pb.h"
#include "TraceMsg/FormatCmdId.h"
#include "public/TaskService.h"
#include "public/GlobalFunc.h"
#include <functional>
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

int64_t       CTableFrame::m_TotalJackPot[5] = {0};

int           CTableFrame::m_SystemAllKillRatio(0);
int           CTableFrame::m_SystemReduceRatio(0);
int           CTableFrame::m_ChangeCardRatio(0);

vector<string>  CTableFrame::ListDBMysql;

CTableFrame::CTableFrame()
        : m_pTableFrameSink(NULL)
        , m_pGameInfo(NULL)
        , m_pGameRoomInfo(NULL)
        , m_pLogicThread(NULL)
//        , m_game_logic_thread(NULL)
        , m_cbGameStatus(GAME_STATUS_INIT)
{
    m_UserList.clear();
    ListDBMysql.clear();
    memset(&m_TableState,0,sizeof(m_TableState));
    memset(&m_TotalJackPot,0,sizeof(m_TotalJackPot));
	memset(m_kcBaseTimes,0,sizeof(m_kcBaseTimes));
    m_kcTimeoutHour = 1.0;
    m_kRatiocReduction = 10.0;
}

CTableFrame::~CTableFrame()
{
    m_UserList.clear();
}

void CTableFrame::Init(shared_ptr<ITableFrameSink>& pSink, TableState& tableState,
                       tagGameInfo* pGameInfo, tagGameRoomInfo* pGameRoomInfo,
                       shared_ptr<EventLoopThread>& game_logic_thread, shared_ptr<EventLoopThread>& game_dbmysql_thread, ILogicThread* pLogicThread)
{
    m_pTableFrameSink = pSink;
    m_pGameInfo = pGameInfo;
    m_pGameRoomInfo = pGameRoomInfo;
	m_pLogicThread	= pLogicThread;

    m_game_logic_thread = game_logic_thread;
    m_game_dbsql_thread = game_dbmysql_thread;
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
    int64_t seconds_since_epoch = chrono::duration_cast<chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
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

            if (!internal_header_ptr)
            {
                LOG_ERROR << " >>>1 sendData get internal_header failed error!,userId:" << pIServerUserItem->GetUserId();
            }

            if(likely(conn) && internal_header_ptr)
            {
                // try to check if the special interal header has breen ok and get the content now.
                if (!internal_header_ptr)
                {
                    LOG_ERROR << " >>>2 sendData get internal_header failed error!,userId:" << pIServerUserItem->GetUserId();
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
//					TRACE_COMMANDID(mainId, subId);
                    LOG_INFO << "mainid:" << mainId << ",subid:" << subId;

                    // send the special data content.
                    conn->send(&encryptedData[0], ret);
                    bSuccess = true;
                }
            }
            else
            {
                int64_t userId = pIServerUserItem->GetUserId();
                bool  isAndroid = pIServerUserItem->IsAndroidUser();
                LOG_WARN <<"CTableFrame sendData TcpConnectionPtr closed. userId:" << userId << ", isAndroid:" << isAndroid << ",mainid:" << mainId << "subid:" << subId;
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
        }
        else
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
        }
        else
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
                GSUserBaseInfo userBaseItem = pIServerUserItem->GetUserBaseInfo(); 

                Json::Value root;
                root["gameId"] = m_pGameRoomInfo->gameId;
                root["nickName"] = pIServerUserItem->GetNickName();
                root["userId"] = pIServerUserItem->GetUserId();
                root["android"] = pIServerUserItem->IsAndroidUser()? 1 : 0;
                root["agentId"] = userBaseItem.agentId;
                root["roomName"] = m_pGameRoomInfo->roomName;
                root["roomId"] = m_pGameRoomInfo->roomId;
                root["msg"]=szMessage;
                root["msgId"] = rand()%7;//szMessage;
                root["score"] = (uint32_t)score;
                Json::FastWriter writer;
                string msg = writer.write(root);

                // LOG_INFO << " Send Game Message ";
                // 
				REDISCLIENT.pushPublishMsg(eRedisPublicMsg::bc_marquee,msg);
				// REDISCLIENT.publishUserKillBossMessage(msg);
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
    if( m_pGameInfo->matchforbids[MTH_PLAYER_CNT] )
    {
        bool onlyOneGuy = GetPlayerCount(false) == 1;
        uint32_t maxPlayerNum = m_pGameRoomInfo->maxPlayerNum;
        for(uint32_t i = 0; i < maxPlayerNum -1; ++i)
        {
            if(m_UserList[i] && !m_UserList[i]->IsAndroidUser())
            {
                if(onlyOneGuy)
                {
                    REDISCLIENT.AddToMatchedUser(m_UserList[i]->GetUserId(), 0);
                    break;
                }
                for(uint32_t j = i + 1; j < maxPlayerNum; ++j)
                {
                    if(m_UserList[j] && !m_UserList[j]->IsAndroidUser())
                    {
                        REDISCLIENT.AddToMatchedUser(m_UserList[i]->GetUserId(), m_UserList[j]->GetUserId());
                    }
                }
            }
        }
    }
    /*
    // add by James 180907-clear table user on conclude game.
    if (m_pRoomKindInfo->bisQipai)
    {
        word wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
        for (int i = 0; i < wMaxPlayer; ++i)
        {
            CServerUserItem* pIServerUserItem = m_UserList[i];
            if (pIServerUserItem)
            {
                if (pIServerUserItem->GetUserStatus() == sOffline)
                {
                    ClearTableUser(pIServerUserItem->GetChairID());
                }
                else
                {
                    // check if auto ready content now.
                    if (m_pRoomKindInfo->bisAutoReady)
                    {
                        // reset the user state to sit down now.
                        pIServerUserItem->SetUserStatus(sReady);
                    }   else
                    {
                        // reset the user state to sit state.
                        pIServerUserItem->SetUserStatus(sSit);
                        LOG_ERROR << "Sit2";

                    }
                }
            }
        }
    }
    */

    // modify by James end.
    if (m_pTableFrameSink)
    {
        m_pTableFrameSink->RepositionSink();
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
	//LOG_DEBUG << "--- *** " << (int)cbStatus;
   m_cbGameStatus = cbStatus;
}

// get the special game status now.
uint8_t CTableFrame::GetGameStatus()
{
    return m_cbGameStatus;
}

bool CTableFrame::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    if (m_pGameRoomInfo->serverStatus==SERVER_STOPPED||m_pGameRoomInfo->serverStatus==SERVER_REPAIRING)
    {
        return false;
    }
    //LOG_ERROR << "userid"<<pUser->GetUserId()<<"当前进入玩家linecode值=="<<pUser->GetUserBaseInfo().lineCode.c_str();
    if(pUser->GetUserId() != -1)
    {
        uint32_t maxPlayerNum = m_pGameRoomInfo->maxPlayerNum;
        for(uint32_t i = 0; i < maxPlayerNum; ++i)
        {
            if(m_UserList[i] && !m_UserList[i]->IsAndroidUser())
            {
                if( m_UserList[i]->GetUserId() != pUser->GetUserId())
                {
                    //LOG_ERROR << "在座子内的玩家userid"<<pUser->GetUserId()<<"在座子内的玩家linecode值=="<<m_UserList[i]->GetUserBaseInfo().lineCode.c_str();
                    //for(int i=0;i<MTH_MAX;i++)
                    //{
                    //   LOG_ERROR <<"状态值" <<m_pGameInfo->matchforbids[i];
                    //}
                    if( m_pGameInfo->matchforbids[MTH_SINGLE_PLAYER] )
                    {
                        LOG_ERROR << "禁止多人匹配";
                        return false;
                    }
                    if(m_pGameInfo->gameType!=GameType_BaiRen&&m_pGameInfo->matchforbids[MTH_BLACKLIST] && (m_UserList[i]->GetBlacklistInfo().status == 1 || pUser->GetBlacklistInfo().status))
                    {
                        LOG_ERROR << "有黑名单人员";
                        return false;
                    }
                    if( m_pGameInfo->matchforbids[MTH_QUANRANTE_AREA] && (pUser->inQuarantine || m_UserList[i]->inQuarantine))
                    {
                        LOG_ERROR << "玩家正在隔离区";
                        return false;
                    }
                    if(pUser->GetUserBaseInfo().agentId != 0 && m_pGameInfo->matchforbids[MTH_FORB_DIFF_AGENT] &&   pUser->GetUserBaseInfo().lineCode.compare(m_UserList[i]->GetUserBaseInfo().lineCode.c_str()))
                    {
                        LOG_ERROR << "禁止不同线路之间玩家匹配";
                        return false;
                    }else if(pUser->GetUserBaseInfo().agentId != 0 && m_pGameInfo->matchforbids[MTH_FORB_SAME_AGENT] &&  pUser->GetUserBaseInfo().lineCode.compare(m_UserList[i]->GetUserBaseInfo().lineCode.c_str())==0)
                    {
                        LOG_ERROR << "禁止相同线路之间玩家匹配";
                        return false;
                    }else if(pUser->GetUserBaseInfo().ip != 0 && m_pGameInfo->matchforbids[MTH_FORB_SAME_IP] && CheckSubnetInetIp(pUser->GetUserBaseInfo().ip, m_UserList[i]->GetIp()))
                    {
                        LOG_ERROR << "禁止相同IP玩家匹配";
                        return false;
                    }else if(pUser->GetUserBaseInfo().ip != 0 && m_pGameInfo->matchforbids[MTH_FORB_DIFF_IP] && !CheckSubnetInetIp(pUser->GetUserBaseInfo().ip, m_UserList[i]->GetIp()))
                    {
                        LOG_ERROR << "禁止不同IP玩家匹配";
                        return false;
                    }else if( m_pGameInfo->matchforbids[MTH_PLAYER_CNT])
                    {
                        vector<string> list;
                        bool result = REDISCLIENT.GetMatchBlockList(pUser->GetUserId(),list);
                        string key;
                        if( result )
                        {
                            key = to_string(m_UserList[i]->GetUserId());
                            std::vector<string>::iterator it = std::find(list.begin(), list.end(),key);
                            if(it != list.end()){
                                LOG_ERROR << "有玩家在当前匹配玩家的匹配屏蔽列表中";
                                return false;
                            }
                        }
                        key = to_string(pUser->GetUserId());
                        std::vector<string>::iterator it1 = std::find(m_UserList[i]->m_sMthBlockList.begin(),m_UserList[i]->m_sMthBlockList.end(),key);
                        if(it1 != m_UserList[i]->m_sMthBlockList.end())
                        {
                            LOG_ERROR << "当前玩家在座玩家的匹配屏蔽列表中";
                            return false;
                        }
                    }
                }else
                {
                    break;
                }
            }
        }
    }


    return m_pTableFrameSink->CanJoinTable(pUser);
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
//    do
//    {
//        shared_ptr<CServerUserItem> pIServerUserItem;
//        {
//            READ_LOCK(m_list_mutex);
//            pIServerUserItem = m_UserList[chairId];
//        }

//        if(!pIServerUserItem)
//        {
//            break;
//        }

//        if(pIServerUserItem->GetUserScore() > m_pRoomKindInfo->enterMinScore)
//        {
//            // try to set the special user status now.
//            pIServerUserItem->SetUserStatus(sReady);
//            // check if the specfial game can been start item now.
//            if ((m_pRoomKindInfo->bisQipai) && (CheckGameStart()))
//            {
//                if (GetGameStatus() == GAME_STATUS_FREE)
//                {
//                    GameRoundStartDeploy();
//                }

//                StartGame();
//            }
//            else
//            {
////                shared_ptr<IServerUserItem> pIItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);
//                BroadcastUserStatus(pIServerUserItem, true);
//            }
//        }else
//        {
////            OnUserLeft(pIServerUserItem, true, true);
//            ClearTableUser(wChairID, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
//        }
//    }while(0);

    return;
}

bool CTableFrame::OnUserLeft(shared_ptr<CServerUserItem>& pIServerUserItem, bool bSendToSelf, bool bForceLeave)
{
    bool ret = false;
    if(pIServerUserItem)
    {
        LOG_DEBUG << __FILE__ << __FUNCTION__ << "CTableFrame::OnUserLeft"<<pIServerUserItem->GetUserId();
//        if(m_pGameInfo->gameType == GameType_BaiRen)
        {
            // 离线
            pIServerUserItem->SetUserStatus(sOffline);
            // 托管
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
        LOG_DEBUG << __FILE__ << __FUNCTION__ << "CTableFrame::OnUserOffline"<<pIServerUserItem->GetUserId();
        {
            pIServerUserItem->SetUserStatus(sOffline);
            pIServerUserItem->SetTrustship(true);
//            BroadcastUserStatus(pIServerUserItem, false);
        }
        ret = m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserId(), false);
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
    LOG_DEBUG << " >>> ClearTableUser tableid:" << m_TableState.nTableID << ", chairid:" << chairId;

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

//        // check if the special user id =0 or wChairID = INVALID_CHAIR.
//        if ((userId  == 0) || (chairId == INVALID_CHAIR))
//        {
//            LOG_WARN << " >>> ClearTableUser userid or chairid is invalidate, userId:" << userId << ", chairId:" << chairId;
//        }

        if(cbSendErrorCode != 0)
        {
//            int mainId = Game::Common::MAIN_MESSAGE_PROXY_TO_GAME_SERVER;
//            int subId  = GameServer::SUB_S2C_ENTER_ROOM_RES;
//            ::GameServer::MSG_SC_UserEnterMessageResponse response;
//            Game::Common::Header* header = response.mutable_header();
//            header->set_sign(HEADER_SIGN);
//            response.set_retcode(cbSendErrorCode);
//            response.set_errormsg("");

//            commandHeader->mainId = mainId;
//            commandHeader->subId = subId;

//            string content = errCode.SerializeAsString();
//            SendPackedData(pUserItem, mainId, subId, content.data(), content.size());
        }

        OnUserStandup(pUserItem, bSendState);

//        if(GetPlayerCount() == 0)
//        {
//            // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//            if(m_pGameInfo->gameType == GameType_Confrontation)
//            {
//                shared_ptr<ITableFrame> pThis = shared_from_this();
//                CGameTableManager::get_mutable_instance().FreeNormalTable(pThis);
//            }
//        }
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
    LOG_ERROR << " >>> CTableFrame::StartGame().";
//    if(m_pGameInfo->matchforbids[MTH_PLAYER_CNT])
//    {
//        //TODO
//        int userSize = m_UserList.size();
//        for(int i = 0; i < userSize; ++i)
//        {
//            REDISCLIENT
//        }
//    }
//    uint32_t maxPlayerNum = m_pGameRoomInfo->maxPlayerNum;
//    for(int i = 0; i < m_UserList.size(); ++i)
//    {
//        shared_ptr<CServerUserItem> pIServerUserItem;
//        {
//            READ_LOCK(m_list_mutex);
//            pIServerUserItem = m_UserList[i];
//        }

//        if(pIServerUserItem)
//        {
//            if( pIServerUserItem->isSit() ||
//                    pIServerUserItem->isReady() ||
//                    pIServerUserItem->isPlaying() )
//            {
//                pIServerUserItem->setClientReady(true); // add by James 181019 - auto set clientReady to true.
//                pIServerUserItem->SetUserStatus(sPlaying);

////                shared_ptr<IServerUserItem> pIItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);
//                BroadcastUserStatus(pIServerUserItem);
//            }
//        }
//    }

////    m_bGameStarted = true;
////    m_dwStartTime  = (dword)time(NULL);
////    m_startDateTime = second_clock::local_time();
////    m_pTableFrameSink->OnGameStart();
////Cleanup:
//    LOG_DEBUG << " >>> CTableFrame::StartGame called, tableid:" << m_TableState.nTableID;
}

//bool CTableFrame::CheckGameStart()
//{
//    dword wMaxPlayer = GetMaxPlayerCount();
//    dword wReadyPlayerCount = 0;
//    dword wUserCount = 0;

//    // loop to sum all special player.
//    for(dword i=0; i<wMaxPlayer; i++)
//    {
//        shared_ptr<CServerUserItem> pIServerUserItem = GetTableUserItem(i);
//        if(!pIServerUserItem)
//            continue;

//        ++wUserCount;
//        if(pIServerUserItem->isReady() == false)
//        {
//            return false;
//        }

//        ++wReadyPlayerCount;
//    }

//    // check current game type now.
//    if(m_pRoomKindInfo->bisQipai)
//    {
//        if(wReadyPlayerCount >= m_pRoomKindInfo->nStartMinPlayer)
//        {
//            return true;
//        }
//    }else if(wMaxPlayer == wReadyPlayerCount)
//    {
//        return true;
//    }else if(m_pRoomKindInfo->bisDynamicJoin)
//    {
//        return true;
//    }
//Cleanup:
//    return false;
//}

bool CTableFrame::OnUserEnterAction(shared_ptr<CServerUserItem> &pIServerUserItem, Header *commandHeader, bool bDistribute)
{
    bool bRet = false;
    if(pIServerUserItem)
    {
        SendUserSitdownFinish(pIServerUserItem, commandHeader, bDistribute);

//        BroadcastUserInfoToOther(pIServerUserItem);
//        SendAllOtherUserInfoToUser(pIServerUserItem);
//        BroadcastUserStatus(pIServerUserItem, true);

       // LOG_DEBUG << __FILE__ << __FUNCTION__ << "m_pTableFrameSink->OnUserEnter pIServerUserItem->GetUserID()"<<pIServerUserItem->GetUserId();
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
        
        // 玩家正在游戏中则不能清除
        if(pIServerUserItem->isPlaying())
        {
            LOG_ERROR << "OnUserStandup pIServerUserItem->isPlaying(), userId:" << pIServerUserItem->GetUserId();
            break;
        }
            int64_t userId = pIServerUserItem->GetUserId();
            uint32_t chairId = pIServerUserItem->GetChairId();
            LOG_INFO << "leaved userId:" << userId << " at tableid(" << m_TableState.nTableID << "," << chairId << ").";

            if(!pIServerUserItem->IsAndroidUser())
            {
                //百人场才需要发通知
                if(m_pGameRoomInfo->serverStatus !=SERVER_RUNNING&&m_pGameInfo->gameType!=1)
                {
                    int mainId = Game::Common::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER;
                    int subId  = GameServer::SUB_S2C_USER_LEFT_RES;

                    ::GameServer::MSG_C2S_UserLeftMessageResponse response;
                    ::Game::Common::Header *resp_header = response.mutable_header();
                    resp_header->set_sign(HEADER_SIGN);

                    response.set_gameid(m_pGameInfo->gameId);
                    response.set_roomid(m_pGameRoomInfo->roomId);
                    response.set_type(0);

                    response.set_retcode(3);//ERROR_ENTERROOM_SERVERSTOP
                    response.set_errormsg("游戏维护请进入其他房间");
                    string content = response.SerializeAsString();
                    SendPackedData(pIServerUserItem, mainId, subId, (uint8_t*)content.data(), content.size(), nullptr);

                    LOG_INFO << "百人场才需要发通知:" << userId << " mainId subId(" << mainId << "," << subId << ")";
                }

                m_pLogicThread->clearUserIdProxyConnectionInfo(userId);
                DelUserOnlineInfo(pIServerUserItem->GetUserId());
                CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);
            }
            else
            {
                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(userId);
            }

            pIServerUserItem->SetTableId(INVALID_TABLE);
            pIServerUserItem->SetChairId(INVALID_CHAIR);
//            pIServerUserItem->ResetUserItem();
            if (chairId < m_UserList.size())
            {
                shared_ptr<CServerUserItem> cServerUserItem;
//                WRITE_LOCK(m_list_mutex);
                m_UserList[chairId] = cServerUserItem;
            }

//            dumpUserList();
            m_pLogicThread->SavePlayGameTime(userId);
            bSuccess = true;
//        }
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

//                LOG_INFO<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";

//                LOG_INFO<<"++++++++++++++++++++++++++++++++pIUserItem GetTableID:"<<pIUserItem->GetTableID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pIUserItem GetChairID:"<<pIUserItem->GetChairID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pIUserItem GetUserID:"<<pIUserItem->GetUserID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pIUserItem GetUserStatus:"<<pIUserItem->GetUserStatus();

//                LOG_INFO<<"--------------------------------------------------------------------------------------------------------------------------------";

//                LOG_INFO<<"++++++++++++++++++++++++++++++++pTargetUserItem GetTableID:"<<pTargetUserItem->GetTableID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pTargetUserItem GetChairID:"<<pTargetUserItem->GetChairID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pTargetUserItem GetUserID:"<<pTargetUserItem->GetUserID();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++pTargetUserItem GetUserStatus:"<<pTargetUserItem->GetUserStatus();
//                LOG_INFO<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++";

                SendPackedData(pTargetUserItem, mainId, subId, (uint8_t*)content.data(),content.size(), nullptr);
            }
            //LOG_INFO<<"++++++++++++++++++++++++++++++++maxPlayerNum :"<<m_pGameRoomInfo->maxPlayerNum;
        }
    }while(0);
    return;
}

//获取水果机免费游戏剩余次数
bool CTableFrame::GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId)
{
    //LOG_INFO<<"+++++++++GetSgjFreeGameRecord+++++++: "<<userId;
    //操作数据
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["sgj_freegame_record"];
    auto query_value = document{} << "userId" << (int64_t)userId << finalize;
    LOG_DEBUG << "Query:"<<bsoncxx::to_json(query_value);
    bsoncxx::stdx::optional<bsoncxx::document::value> maybe_result = coll.find_one(query_value.view());
    if(maybe_result)
    {
        LOG_DEBUG << "QueryResult:"<<bsoncxx::to_json(maybe_result->view());

        bsoncxx::document::view view = maybe_result->view();
        pSgjFreeGameRecord->userId      = view["userId"].get_int64().value;
        pSgjFreeGameRecord->betInfo     = view["betInfo"].get_int64().value;
        pSgjFreeGameRecord->allMarry    = view["allMarry"].get_int32().value;
        pSgjFreeGameRecord->freeLeft    = view["freeLeft"].get_int32().value;
        pSgjFreeGameRecord->marryLeft   = view["marryLeft"].get_int32().value;
        LOG_INFO<<"+++++++++ find_one +++++++:true "<<userId;
        return true;
    }
    
   // LOG_INFO<<"+++++++++ not find one +++++++:false "<<userId;
    return false;
}
//记录水果机免费游戏剩余次数
bool CTableFrame::WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord)
{
    // LOG_INFO"+++++++++ WriteSgjFreeGameRecord +++++++:"<<pSgjFreeGameRecord->userId
    // <<pSgjFreeGameRecord->allMarry<< " " <<pSgjFreeGameRecord->betInfo<< " " 
    // <<pSgjFreeGameRecord->freeLeft<< " " <<pSgjFreeGameRecord->marryLeft;

    //操作数据
    mongocxx::collection coll = MONGODBCLIENT["gamemain"]["sgj_freegame_record"];

    int leftCount = pSgjFreeGameRecord->freeLeft + pSgjFreeGameRecord->marryLeft;
    //没有时清楚记录
    if(leftCount <= 0){
        coll.delete_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize);
    }
    else
    {
        auto find_result = coll.find_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize);
        if(find_result) {
            coll.update_one(document{} << "userId" << pSgjFreeGameRecord->userId << finalize,
                    document{} << "$set" << open_document
                    //<<"userId" << pSgjFreeGameRecord->userId
                    <<"betInfo" << pSgjFreeGameRecord->betInfo
                    <<"allMarry" << pSgjFreeGameRecord->allMarry
                    <<"freeLeft" << pSgjFreeGameRecord->freeLeft
                    <<"marryLeft" << pSgjFreeGameRecord->marryLeft << close_document
                    << finalize);
             LOG_INFO<<"+++++++++ update_one +++++++:";
        }
        else
        {
            auto insert_value = bsoncxx::builder::stream::document{}
                << "userId" << pSgjFreeGameRecord->userId
                << "betInfo" << pSgjFreeGameRecord->betInfo
                << "allMarry" << pSgjFreeGameRecord->allMarry
                << "freeLeft" << pSgjFreeGameRecord->freeLeft
                << "marryLeft" <<  pSgjFreeGameRecord->marryLeft
                << bsoncxx::builder::stream::finalize;

            // LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);
            bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
            if(result){
                // LOG_INFO<<"+++++++++ insert_one +++++++:"<<pSgjFreeGameRecord->userId;
            }     
        }
        
    }

    return true;
}

/*
写玩家分
@param pScoreInfo 玩家分数结构体数组
@param nCount  写玩家个数
@param strRound 
*/
bool CTableFrame::SetBlackeListInfo(string insert,map<string,int16_t> &usermap)
{
    usermap.clear();
    vector<string> vec;
    vec.clear();
    if(insert=="")
    {
        //LOG_ERROR<<"balcklist has no key";
        return false;
    }
    string covers=insert;
    covers.erase(std::remove(covers.begin(), covers.end(), '"'), covers.end());
    boost::algorithm::split( vec,covers, boost::is_any_of( "|" ));
    if(vec.size()==0)
    {
        LOG_ERROR<<"balcklist vec.size()==0 has no key";
    }
    for(int i=0;i<(int)vec.size();i++)
    {
        vector<string> vec1;
        vec1.clear();
        boost::algorithm::split( vec1,vec[i] , boost::is_any_of( "," ));
        if(vec1.size()==2)
        {
            usermap.insert(pair<string,int16_t>(vec1[0],stoi(vec1[1])));
        }
    }
    return true;
}
void CTableFrame::ConversionFormat(GSUserWriteMysqlInfo &userMysqlInfo, tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
{
    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
    int32_t rankId = 0;
    if (m_pGameInfo->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
        //解析-97
        if (scoreInfo->cardValue.length() >= 5){
            string str1= scoreInfo->cardValue.substr(0,5);
            string str2= scoreInfo->cardValue.substr(5);
            if(str1.compare("---97") == 0){
                rankId = -97;
                // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
                scoreInfo->cardValue = str2;
                // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
                // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
            }
        }
    }
    else if (m_pGameInfo->gameId == (uint32_t)eGameKindId::bbqznn && scoreInfo->cardValue.compare("") != 0) //百变牛的喜钱
    {
        int pos = scoreInfo->cardValue.find(",");
        if(pos >= 0){
            rankId = -96;
        }
    }
    userMysqlInfo.account = scoreInfo->account;
    userMysqlInfo.agentId = scoreInfo->agentId;
    userMysqlInfo.allBet = scoreInfo->betScore;
    userMysqlInfo.beforeScore = scoreInfo->beforeScore;
    userMysqlInfo.cardValue = scoreInfo->cardValue;
    string strcellscore="";
    for(int i=0;i<scoreInfo->cellScore.size();i++)
    {
        if(i==scoreInfo->cellScore.size()-1)
        {
            strcellscore+=to_string(scoreInfo->cellScore[i]);
        }
        else
        {
            strcellscore+=to_string(scoreInfo->cellScore[i])+"|";
        }
        userMysqlInfo.cellScoreIntType.push_back(scoreInfo->cellScore[i]);
    }
    userMysqlInfo.startTime = scoreInfo->startTime;
    userMysqlInfo.cellScore = strcellscore;
    userMysqlInfo.effectiveScore = scoreInfo->addScore;
    userMysqlInfo.gameEndTime = InitialConversion(time(NULL));
    userMysqlInfo.gameEvent = "";
    userMysqlInfo.gameId = m_pGameInfo->gameId;
    userMysqlInfo.gameStartTime = InitialConversion(std::chrono::system_clock::to_time_t(scoreInfo->startTime));
    userMysqlInfo.isAndroid = bAndroidUser;
    userMysqlInfo.isBanker = scoreInfo->isBanker;
    userMysqlInfo.lineCode = scoreInfo->lineCode;
    userMysqlInfo.rankId = rankId;
    userMysqlInfo.revenue = scoreInfo->revenue;
    userMysqlInfo.roomId = m_pGameRoomInfo->roomId;
    userMysqlInfo.score = scoreInfo->beforeScore + scoreInfo->addScore;
    userMysqlInfo.strRoundId = strRoundId;
    userMysqlInfo.tableId = m_TableState.nTableID;
    userMysqlInfo.userCount = userCount;
    userMysqlInfo.userId = scoreInfo->userId;
    userMysqlInfo.winScore = scoreInfo->addScore;
    userMysqlInfo.changetype = m_pGameRoomInfo->roomId;
    userMysqlInfo.changemoney = scoreInfo->addScore;
    userMysqlInfo.fishRake = scoreInfo->fishRake;
    userMysqlInfo.rWinScore = scoreInfo->rWinScore;
    userMysqlInfo.chairId = scoreInfo->chairId;
    userMysqlInfo.currency = m_pGameRoomInfo->currency;
    userMysqlInfo.endTime=chrono::system_clock::now();
}
void CTableFrame::ConversionFormat(GSUserWriteMysqlInfo &userMysqlInfo,GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
{
    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
    int32_t rankId = 0;
    if (m_pGameInfo->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
        //解析-97
        if (scoreInfo->cardValue.length() >= 5){
            string str1= scoreInfo->cardValue.substr(0,5);
            string str2= scoreInfo->cardValue.substr(5);
            if(str1.compare("---97") == 0){
                rankId = -97;
                // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
                scoreInfo->cardValue = str2;
                // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
                // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
            }
        }
    }
    else if (m_pGameInfo->gameId == (uint32_t)eGameKindId::bbqznn && scoreInfo->cardValue.compare("") != 0) //百变牛的喜钱
    {
        int pos = scoreInfo->cardValue.find(",");
        if(pos >= 0){
            rankId = -96;
        }
    }
    userMysqlInfo.account = userBaseInfo.account;
    userMysqlInfo.agentId = userBaseInfo.agentId;
    userMysqlInfo.allBet = scoreInfo->betScore;
    userMysqlInfo.beforeScore = userBaseInfo.userScore;
    userMysqlInfo.cardValue = scoreInfo->cardValue;
    string strcellscore="";
    for(int i=0;i<scoreInfo->cellScore.size();i++)
    {
        if(i==scoreInfo->cellScore.size()-1)
        {
            strcellscore+=to_string(scoreInfo->cellScore[i]);
        }
        else
        {
            strcellscore+=to_string(scoreInfo->cellScore[i])+"|";
        }
        userMysqlInfo.cellScoreIntType.push_back(scoreInfo->cellScore[i]);
    }
    userMysqlInfo.startTime = scoreInfo->startTime;
    userMysqlInfo.cellScore = strcellscore;
    userMysqlInfo.effectiveScore = scoreInfo->addScore;
    userMysqlInfo.gameEndTime = InitialConversion(time(NULL));
    userMysqlInfo.gameEvent = "";
    userMysqlInfo.gameId = m_pGameInfo->gameId;
    userMysqlInfo.gameStartTime = InitialConversion(std::chrono::system_clock::to_time_t(scoreInfo->startTime));
    userMysqlInfo.isAndroid = bAndroidUser;
    userMysqlInfo.isBanker = scoreInfo->isBanker;
    userMysqlInfo.lineCode = userBaseInfo.lineCode;
    userMysqlInfo.rankId = rankId;
    userMysqlInfo.revenue = scoreInfo->revenue;
    userMysqlInfo.roomId = m_pGameRoomInfo->roomId;
    userMysqlInfo.score = userBaseInfo.userScore + scoreInfo->addScore;
    userMysqlInfo.strRoundId = strRoundId;
    userMysqlInfo.tableId = m_TableState.nTableID;
    userMysqlInfo.userCount = userCount;
    userMysqlInfo.userId = userBaseInfo.userId;
    userMysqlInfo.winScore = scoreInfo->addScore;
    userMysqlInfo.changetype = m_pGameRoomInfo->roomId;
    userMysqlInfo.changemoney = scoreInfo->addScore;
    userMysqlInfo.fishRake = scoreInfo->fishRake;
    userMysqlInfo.rWinScore = scoreInfo->rWinScore;
    userMysqlInfo.chairId = scoreInfo->chairId;
    userMysqlInfo.endTime=chrono::system_clock::now();
    userMysqlInfo.currency = m_pGameRoomInfo->currency;
}

void CTableFrame::WriteDBThread(GSUserWriteMysqlInfo MysqlInfo)
{
    //写注单
    //WtiteUserPlayRecordMysql(MysqlInfo);
    //写游戏记录明细
    //WtiteUserGameLogMysql(MysqlInfo);
    //写账变记录表
    //WtiteUserScoreTrangeMysql(MysqlInfo);
}



//这个要求临时先写mysql 和 mongodb
void CTableFrame::WtiteUserPlayRecordMysqlx(string mysqlStr)
{
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement(mysqlStr));

//        pstmt->executeUpdate();
//        LOG_WARN << "write user recored count--------";
////        pstmt->close();
////        res->close();
////        conn->close();
//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
}
//逐条写入的函数暂时不要了，换成一次插入多条数据的方法
void CTableFrame::WtiteUserPlayRecordMysql(GSUserWriteMysqlInfo MysqlInfo)
{
    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });



//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO play_record(userid, agentid, "
//                                           "gameid,roomid,tableid,chairid,isBanker,"
//                                           "usercount,beforescore,rwinscore,"
//                                           "allbet,winscore,score,revenue,isandroid,"
//                                           "rank,gameevent,gameinfoid,account,linecode,gamestarttime,gameendtime,cellscore,id) "
//                                           "VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"));
//        pstmt->setInt64(1, MysqlInfo.userId);
//        pstmt->setInt(2, MysqlInfo.agentId);
//        pstmt->setInt(3, MysqlInfo.gameId);
//        pstmt->setInt(4, MysqlInfo.roomId);
//        pstmt->setInt(5, MysqlInfo.tableId);
//        pstmt->setInt(6, MysqlInfo.chairId);
//        pstmt->setInt(7, MysqlInfo.isBanker);
//        pstmt->setInt(8, MysqlInfo.userCount);

//        pstmt->setInt64(9, MysqlInfo.beforeScore);
//        pstmt->setInt64(10, MysqlInfo.rWinScore);
//        pstmt->setInt64(11, MysqlInfo.allBet);
//        pstmt->setInt64(12, MysqlInfo.winScore);
//        pstmt->setInt64(13, MysqlInfo.score);
//        pstmt->setInt64(14, MysqlInfo.revenue);
//        pstmt->setInt(15, MysqlInfo.isAndroid);
//        pstmt->setInt(16, MysqlInfo.rankId);
//        pstmt->setString(17, MysqlInfo.gameEvent);
//        pstmt->setString(18, MysqlInfo.strRoundId);
//        pstmt->setString(19, MysqlInfo.account);
//        pstmt->setString(20, MysqlInfo.lineCode);

//        pstmt->setDateTime(21, MysqlInfo.gameStartTime);
//        pstmt->setDateTime(22, MysqlInfo.gameEndTime);
//        pstmt->setString(23, MysqlInfo.cellScore);
//        pstmt->setString(24, GlobalFunc::generateUUID());

//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
}
//逐条写入的函数暂时不要了，换成一次插入多条数据的方法
//void CTableFrame::WtiteUserGameLogMysql(GSUserWriteMysqlInfo MysqlInfo)
//{
//    //把些mysql语句全部放到容器中，进行缓存写入，逐条执行,而且是在l另外的线程中
//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO game_log(gamelogid, userid, "
//                                           "account,agentid,linecode,gameid,roomid,"
//                                           "winscore,revenue,logdate)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?,?)"));
//        pstmt->setString(1, MysqlInfo.strRoundId);
//        pstmt->setInt64(2, MysqlInfo.userId);
//        pstmt->setString(3, MysqlInfo.account);
//        pstmt->setInt(4, MysqlInfo.agentId);
//        pstmt->setString(5, MysqlInfo.lineCode);
//        pstmt->setInt(6, MysqlInfo.gameId);
//        pstmt->setInt(7, MysqlInfo.roomId);
//        pstmt->setInt64(8, MysqlInfo.winScore);
//        pstmt->setInt64(9, MysqlInfo.revenue);
//        pstmt->setDateTime(10, MysqlInfo.gameEndTime);
//        pstmt->executeUpdate();
//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
//}
//前期要求同时写mysql 和 mongodb
void CTableFrame::WtiteUserScoreTrangeMysqlx(string mysqlStr)
{
//    try
//    {

//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });
//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement(mysqlStr));
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
}
//逐条写的方法不要了，一次性写入多个玩家的提高效率
//void CTableFrame::WtiteUserScoreTrangeMysql(GSUserWriteMysqlInfo MysqlInfo)
//{

//    try
//    {

//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });
//        conn->setSchema(DB_RECORD);
//        pstmt.reset(conn->prepareStatement("INSERT INTO user_score_record(userid, account, "
//                                           "agentid,linecode,changetype,changemoney,beforechange,"
//                                           "afterchange,logdate)"
//                                           "VALUES(?,?,?,?,?,?,?,?,?)"));

//        pstmt->setInt64(1, MysqlInfo.userId);
//        pstmt->setString(2, MysqlInfo.account);
//        pstmt->setInt(3, MysqlInfo.agentId);
//        pstmt->setString(4, MysqlInfo.lineCode);
//        pstmt->setInt(5, MysqlInfo.changetype);
//        pstmt->setInt64(6, MysqlInfo.changemoney);
//        pstmt->setInt64(7, MysqlInfo.beforeScore);
//        pstmt->setInt64(8, MysqlInfo.score);
//        pstmt->setDateTime(9, MysqlInfo.gameEndTime);
//        pstmt->executeUpdate();

//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
//}
void CTableFrame::WtiteUserPersonalRedis(GSUserWriteMysqlInfo MysqlInfo)
{
    bool res=false;
    if(MysqlInfo.gameId== (uint32_t)eGameKindId::jcfish
     ||MysqlInfo.gameId== (uint32_t)eGameKindId::lkpy
     ||MysqlInfo.gameId== (uint32_t)eGameKindId::rycs
     ||MysqlInfo.gameId== (uint32_t)eGameKindId::dntg)
    {

        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(MysqlInfo.userId)+to_string(MysqlInfo.roomId);
        int64_t rake=0;
        //按照洗码量提成
        rake=MysqlInfo.winScore+MysqlInfo.fishRake;
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,rake ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,MysqlInfo.allBet ,&result);
    }
    else
    {
        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(MysqlInfo.userId)+to_string(MysqlInfo.roomId);
        //LOG_ERROR<<"roomId"<<m_pGameRoomInfo->roomId<<"    userId"<<userBaseInfo.userId;

        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,MysqlInfo.winScore+MysqlInfo.revenue ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,MysqlInfo.allBet ,&result);
    }
}
void CTableFrame::WtiteUserBasicInfoRedis(GSUserWriteMysqlInfo MysqlInfo)
{
    int64_t userId = MysqlInfo.userId;
    string keyUserInfo = REDIS_USER_BASIC_INFO + to_string(userId);
    time_t tx = time(NULL); //获取目前秒时间
    struct tm * local;
    local = localtime(&tx);
    struct tm t;
    t.tm_year = local->tm_year ;
    t.tm_mon = local->tm_mon;
    t.tm_mday = local->tm_mday;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    time_t shijianlong= mktime(&t);
    int64_t beginStart=(int64_t)shijianlong;

    if(REDISCLIENT.ExistsUserBasicInfo(userId))
    {
        string date="";
        string nowdate=to_string(beginStart);
        REDISCLIENT.GetUserBasicInfo(userId,REDIS_USER_DATE ,date);

        string fields[2] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET};
        redis::RedisValue values;
        int ret = REDISCLIENT.hmget(keyUserInfo, fields, 2, values);
        int64_t redisdate = values[REDIS_USER_DATE].asInt64();
        int64_t todaybet =  values[REDIS_USER_TODAY_BET].asInt64();
        //LOG_ERROR<<"redisdate"<<redisdate<<" beginStart "<<beginStart;
        if(redisdate==beginStart)
        {
             int64_t  current = todaybet+MysqlInfo.allBet;
             redis::RedisValue redisValue;
             redisValue[REDIS_USER_TODAY_BET] = to_string(current);
             REDISCLIENT.hmset(keyUserInfo, redisValue);

        }
        else
        {
            redis::RedisValue redisValue;
            redisValue[REDIS_USER_DATE] = to_string(beginStart);
            redisValue[REDIS_USER_TODAY_BET] = to_string(0);
            redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
            redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
            redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
            redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

            redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
            redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
            REDISCLIENT.hmset(keyUserInfo, redisValue);
        }
    }
    else
    {

        redis::RedisValue redisValue;
        redisValue[REDIS_USER_DATE] = to_string(beginStart);
        redisValue[REDIS_USER_TODAY_BET] = to_string(0);
        redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
        redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
        redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
        redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

        redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
        redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
        string keyUserInfo =REDIS_USER_BASIC_INFO+to_string(userId);
        REDISCLIENT.hmset(keyUserInfo, redisValue);

    }
}
bool CTableFrame::WriteUserScore(tagScoreInfo* pScoreInfo, uint32_t nCount, string &strRound)
{
    if (pScoreInfo && nCount > 0)
    {

        //本次请求开始时间戳(微秒)
        muduo::Timestamp timestart = muduo::Timestamp::now();



        int64_t currenttime = time(NULL);
        int32_t userCount = 0;
        double WritePersonalRedistimes=0.0;
        double UpdateUserScoreToDBtimes=0.0;
        double AddUserGameInfoToDBtimes=0.0;
        double AddScoreChangeRecordToDBtimes=0.0;
        double AddUserGameLogToDBtimes=0.0;
        double BlackListControltimes=0.0;
        double PersonalProfittimes=0.0;
        double UpdateUserRedisBasicInfotimes=0.0;
        double SetUserOnlineInfoitmes=0.0;
        double YBWriteDBThreadtimes=0.0;
        double YBupdateTaskSystemtimes=0.0;


        bool isFirst =false;
        bool isWrite = false;
        try
        {


            string mysqlRecord="";
            string mysqlTrange="";
            for (uint32_t i = 0; i < nCount; ++i)
            {
                tagScoreInfo *scoreInfo = (pScoreInfo + i);
                uint32_t chairId = scoreInfo->chairId;

                shared_ptr<CServerUserItem> pIServerUserItem;
                {
                    LOG_WARN << " chairId" <<chairId;
                    pIServerUserItem = m_UserList[chairId];
                }

                // is NULL
                if (!pIServerUserItem)
                {
                    LOG_WARN << " >>> WriteUserScore pIServerUserItem is NULL! <<<";
                    continue;
                }


                // get target Score
                GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();
                int64_t sourceScore = userBaseInfo.userScore;
                int64_t targetScore = sourceScore + scoreInfo->addScore;

                //is android,update android user score
                if (pIServerUserItem->IsAndroidUser()&&m_pGameInfo->gameId!=(int)eGameKindId::sgj)
                {
                    pIServerUserItem->SetUserScore(targetScore);
                    continue;
                }
                GSUserWriteMysqlInfo UserMysqlInfo;


                if(pIServerUserItem->IsAndroidUser())
                {
                    ConversionFormat(UserMysqlInfo,userBaseInfo, scoreInfo, strRound, nCount, true);

                }
                else
                {
                    ConversionFormat(UserMysqlInfo,userBaseInfo, scoreInfo, strRound, nCount, false);
                }

                muduo::Timestamp timex1 = muduo::Timestamp::now();
                //更新玩家分
                WtiteUserUserscoreRedisMysql(UserMysqlInfo);
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserUserscoreRedisMysql, this,UserMysqlInfo));
                  //更新redis捕鱼个人库存所需数据

                WtiteUserPersonalRedis(UserMysqlInfo);
                WriteRecordMongodb(UserMysqlInfo);
                WriteChangeToMongodb(UserMysqlInfo);
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserPersonalRedis, this,UserMysqlInfo));


                //每日注单汇总
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::InsertUserWeekBetting, this,UserMysqlInfo));

                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WriteRecordMongodb, this,UserMysqlInfo));
    //                //写账变记录
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WriteChangeToMongodb, this,UserMysqlInfo));




                pIServerUserItem->SetUserScore(targetScore);


                timex1 = muduo::Timestamp::now();
                // 黑名单控制
                BlackListControl(pIServerUserItem,userBaseInfo,scoreInfo);
                BlackListControltimes+=muduo::timeDifference(muduo::Timestamp::now(), timex1);


                timex1 = muduo::Timestamp::now();
                //读取个人库存控制
                PersonalProfit(pIServerUserItem);
                PersonalProfittimes+=muduo::timeDifference(muduo::Timestamp::now(), timex1);

                timex1 = muduo::Timestamp::now();
                //写任务系统
                TaskService::get_mutable_instance().updateTaskSystem(userBaseInfo.userId, userBaseInfo.agentId, scoreInfo->addScore,
                                                                     scoreInfo->rWinScore, m_pGameRoomInfo->gameId, m_pGameRoomInfo->roomId,false);
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&TaskService::updateTaskSystem, &TaskService::get_mutable_instance(),userBaseInfo.userId, userBaseInfo.agentId, scoreInfo->addScore,
                //                                                                                                 scoreInfo->rWinScore, m_pGameRoomInfo->gameId, m_pGameRoomInfo->roomId,false));
                YBupdateTaskSystemtimes+=muduo::timeDifference(muduo::Timestamp::now(), timex1);

                WtiteUserBasicInfoRedis(UserMysqlInfo);
                //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserBasicInfoRedis, this,UserMysqlInfo));
                if(pIServerUserItem->GetUserStatus()!=sOffline)
                {
                    SetUserOnlineInfo(userBaseInfo.userId,pIServerUserItem->GetTableId());
                }
                userCount++;

                int32_t m_year = 0;
                char bufer[20000];
                                string tableName ="";

                                time_t tx = time(NULL); //获取目前秒时间
                                struct tm * local;
                                local = localtime(&tx);
                                string months ="";
                                string days = "";
                                if(local->tm_mon+1>=10 ) months = to_string(local->tm_mon+1);
                                else months = "0"+to_string(local->tm_mon+1);
                                tableName = "play_record";
                                m_year = (local->tm_year+1900)*100+local->tm_mon+1;
                   if(!isFirst)
                   {
                       mysqlRecord="INSERT INTO "+tableName+"(userid, agentid, "
                                                                          "gameid,roomid,tableid,chairid,isBanker,"
                                                                          "usercount,beforescore,rwinscore,"
                                                                          "allbet,winscore,score,revenue,isandroid,"
                                                                          "rank,month,cardvalue,gameinfoid,account,linecode,gamestarttime,gameendtime,cellscore,id)";
                      string uuid=GlobalFunc::generateUUID();
                      sprintf(bufer,"VALUES(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                              UserMysqlInfo.userId,UserMysqlInfo.agentId,UserMysqlInfo.gameId,UserMysqlInfo.roomId,UserMysqlInfo.tableId,
                              UserMysqlInfo.chairId,UserMysqlInfo.isBanker,UserMysqlInfo.userCount,UserMysqlInfo.beforeScore,UserMysqlInfo.rWinScore,
                              UserMysqlInfo.allBet,UserMysqlInfo.winScore,UserMysqlInfo.score,UserMysqlInfo.revenue,UserMysqlInfo.isAndroid,UserMysqlInfo.rankId,m_year,UserMysqlInfo.cardValue.c_str(),
                              UserMysqlInfo.strRoundId.c_str(),UserMysqlInfo.account.c_str(),UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.gameStartTime.c_str(),UserMysqlInfo.gameEndTime.c_str(),
                              UserMysqlInfo.cellScore.c_str(),uuid.c_str());
                      mysqlRecord+=bufer;
                      isFirst = true;


                   }
                   else
                   {
                       string uuid=GlobalFunc::generateUUID();
                       sprintf(bufer,",(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                               UserMysqlInfo.userId,UserMysqlInfo.agentId,UserMysqlInfo.gameId,UserMysqlInfo.roomId,UserMysqlInfo.tableId,
                               UserMysqlInfo.chairId,UserMysqlInfo.isBanker,UserMysqlInfo.userCount,UserMysqlInfo.beforeScore,UserMysqlInfo.rWinScore,
                               UserMysqlInfo.allBet,UserMysqlInfo.winScore,UserMysqlInfo.score,UserMysqlInfo.revenue,UserMysqlInfo.isAndroid,UserMysqlInfo.rankId,m_year,UserMysqlInfo.cardValue.c_str(),
                               UserMysqlInfo.strRoundId.c_str(),UserMysqlInfo.account.c_str(),UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.gameStartTime.c_str(),UserMysqlInfo.gameEndTime.c_str(),
                               UserMysqlInfo.cellScore.c_str(),uuid.c_str());
                       mysqlRecord+=bufer;
                   }
                   isWrite =true;


//                char scoreTrageStr[2000];
//                if(i==0)
//                {
//                    mysqlTrange="INSERT INTO user_score_record(userid, account, "
//                                "agentid,linecode,changetype,changemoney,beforechange,"
//                                "afterchange,logdate)";
//                   sprintf(bufer,"VALUES(%d,'%s',%d,'%s',%d,%d,%d,%d,'%s')",
//                           UserMysqlInfo.userId,UserMysqlInfo.account.c_str(),UserMysqlInfo.agentId,UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.changetype,
//                           UserMysqlInfo.changemoney,UserMysqlInfo.beforeScore,UserMysqlInfo.score,UserMysqlInfo.gameEndTime.c_str());
//                   mysqlTrange+=scoreTrageStr;



//                }
//                else
//                {
//                    string uuid=GlobalFunc::generateUUID();
//                    sprintf(bufer,",(%d,'%s',%d,'%s',%d,%d,%d,%d,'%s')",
//                            UserMysqlInfo.userId,UserMysqlInfo.account.c_str(),UserMysqlInfo.agentId,UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.changetype,
//                            UserMysqlInfo.changemoney,UserMysqlInfo.beforeScore,UserMysqlInfo.score,UserMysqlInfo.gameEndTime.c_str());
//                    mysqlTrange+=scoreTrageStr;
//                }


            }//end for
            //if(isWrite)
             //   m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserPlayRecordMysqlx, this,mysqlRecord));
            //m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserScoreTrangeMysqlx, this,mysqlTrange));




            // 打印日志
            int64_t distance =  time(NULL)-currenttime;
            LOG_ERROR << "---------------------     "<<distance<<"     ---------------------     ";
            LOG_ERROR << ">>> 耗时[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) << "]秒 " << __FUNCTION__ << "     本次下注人数是:==="<<userCount;
            LOG_ERROR <<" WritePersonalRedistimes======  "<<WritePersonalRedistimes;
            LOG_ERROR <<" UpdateUserScoreToDBtimes======  "<<UpdateUserScoreToDBtimes;
            LOG_ERROR <<" AddUserGameInfoToDBtimes======  "<<AddUserGameInfoToDBtimes;
            LOG_ERROR <<" AddScoreChangeRecordToDBtimes======  "<<AddScoreChangeRecordToDBtimes;
            LOG_ERROR <<" AddUserGameLogToDBtimes======  "<<AddUserGameLogToDBtimes;
            LOG_ERROR <<" BlackListControltimes======  "<<BlackListControltimes;
            LOG_ERROR <<" PersonalProfittimes======  "<<PersonalProfittimes;
            LOG_ERROR <<" UpdateUserRedisBasicInfotimes======  "<<UpdateUserRedisBasicInfotimes;
            LOG_ERROR <<" SetUserOnlineInfoitmes======  "<<SetUserOnlineInfoitmes;
            LOG_ERROR <<" YBWriteDBThreadtimes======  "<<YBWriteDBThreadtimes;
            LOG_ERROR <<" YBupdateTaskSystemtimes======  "<<YBupdateTaskSystemtimes;
        }
        catch (exception &e)
        {
            LOG_ERROR << "MongoDB WriteData Error:" << e.what();
            return false;
        }


    }
    return true;
}

// 黑名单控制
void CTableFrame::BlackListControl(shared_ptr<CServerUserItem> & pIServerUserItem ,GSUserBaseInfo & userBaseInfo , tagScoreInfo *scoreInfo)
{
    if (m_pGameInfo->matchforbids[MTH_BLACKLIST])
    {
        long current;
        string blacklistkey = REDIS_BLACKLIST + to_string(pIServerUserItem->GetUserId());
        if (pIServerUserItem->GetBlacklistInfo().status == 1)
        {
            auto it = pIServerUserItem->GetBlacklistInfo().listRoom.find(to_string(m_pGameInfo->gameId));
            if (it != pIServerUserItem->GetBlacklistInfo().listRoom.end())
            {
                if (it->second > 0)
                {
                    bool ret = REDISCLIENT.hincrby(blacklistkey, REDIS_FIELD_CURRENT, -scoreInfo->addScore, &current);
                    if (ret)
                    {
                        string fields[2] = {REDIS_FIELD_TOTAL, REDIS_FIELD_STATUS};
                        redis::RedisValue values;
                        ret = REDISCLIENT.hmget(blacklistkey, fields, 2, values);
                        if (ret && !values.empty())
                        {
                            pIServerUserItem->GetBlacklistInfo().current = current;
                            if (values[REDIS_FIELD_STATUS].asInt() == 1)
                            {
                                if (values[REDIS_FIELD_TOTAL].asInt() <= current)
                                {
                                    redis::RedisValue redisValue;
                                    redisValue[REDIS_FIELD_STATUS] = (int32_t)0;
                                    REDISCLIENT.hmset(blacklistkey, redisValue);
                                    pIServerUserItem->GetBlacklistInfo().status = 0;
                                    WriteBlacklistLog(pIServerUserItem, 0);
                                    LOG_ERROR << "balcklist WriteBlacklistLog  0";
                                }
                                else
                                {
                                    WriteBlacklistLog(pIServerUserItem, 1);
                                    LOG_ERROR << "balcklist WriteBlacklistLog  1";
                                }
                            }
                            else
                            {
                                pIServerUserItem->GetBlacklistInfo().status = 0;
                                LOG_ERROR << "No WriteBlacklistLog  1";
                            }
                        }
                    }
                }
            }
        }
        else
        {
            redis::RedisValue status;
            string fields[1] = {REDIS_FIELD_STATUS};
            if (REDISCLIENT.hmget(REDIS_BLACKLIST + to_string(userBaseInfo.userId), fields, 1, status))
             if(!status[REDIS_FIELD_STATUS].asInt())
             {
                 return;
             }
        }
        redis::RedisValue status;
        string fields[5] = {REDIS_FIELD_STATUS, REDIS_FIELD_RATE, REDIS_FIELD_TOTAL, REDIS_FIELD_CONTROL, REDIS_FIELD_CURRENT};
        if (REDISCLIENT.hmget(REDIS_BLACKLIST + to_string(userBaseInfo.userId), fields, 5, status))
        {
            // LOG_INFO << "update blacklist userid : " << userBaseInfo.userId;
            SetBlackeListInfo(status[REDIS_FIELD_CONTROL].asString(), pIServerUserItem->GetBlacklistInfo().listRoom);
            auto it = pIServerUserItem->GetBlacklistInfo().listRoom.find(to_string(m_pGameInfo->gameId));
            if (it != pIServerUserItem->GetBlacklistInfo().listRoom.end())
            {
                pIServerUserItem->GetBlacklistInfo().weight = it->second; //
            }
            else
            {
                pIServerUserItem->GetBlacklistInfo().weight = 0;
                LOG_INFO << "hava no  blacklist room [" << userBaseInfo.userId << "]";
            }
            pIServerUserItem->GetBlacklistInfo().status = status[REDIS_FIELD_STATUS].asInt(); 
            pIServerUserItem->GetBlacklistInfo().total = status[REDIS_FIELD_TOTAL].asLong();
            pIServerUserItem->GetBlacklistInfo().current = status[REDIS_FIELD_CURRENT].asLong();
        }
        else
        {
            LOG_ERROR << "update redis faile,userId[" << userBaseInfo.userId << "]";
            pIServerUserItem->GetBlacklistInfo().status = 0;
        } 
    }
}
void CTableFrame::InsertUserWeekBetting(GSUserWriteMysqlInfo MysqlInfo)
{
    //获取当前时间戳
    time_t tx = time(NULL);
    struct tm * local;
    local = localtime(&tx);
    //是星期一就执行下面的
    int32_t today =local->tm_wday;

    //后去今天零点的时间戳
    tm tpNew ;
    tpNew.tm_year = local->tm_year;
    tpNew.tm_mon = local->tm_mon;
    tpNew.tm_mday = local->tm_mday;
    tpNew.tm_hour = 0;
    tpNew.tm_min = 0;
    tpNew.tm_sec = 0;
    time_t weekzero= mktime(&tpNew); //
    //今天是周几，然后用今天零点时间减去对应周几，就是这周周一开始的时间戳

    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_daybetting"];
        auto query_value = document{} << "userid" << MysqlInfo.userId << "inserttime" << weekzero << finalize;
        auto seq_updateValue = document{} << "$set" << open_document << "userid" << (int64_t)MysqlInfo.userId << "agentid" << (int32_t)MysqlInfo.agentId
                                          << "inserttime" << weekzero << "daybetting" << (int64_t)MysqlInfo.allBet << "dayprofit" << (int64_t)MysqlInfo.rWinScore
                                         <<close_document << finalize;
        //update options
        mongocxx::options::update options = mongocxx::options::update{};
        coll.update_one(query_value.view(), seq_updateValue.view(), options.upsert(true));

    }
    catch (const std::exception &e)
    {
        LOG_ERROR <<" MongoDB exception: " << e.what();
    }
}
//void CTableFrame::InsertUserWeekBetting(int64_t betting,int64_t userid,int32_t agentid,int64_t profit)
//{
//    //获取当前时间戳
//    time_t tx = time(NULL);
//    struct tm * local;
//    local = localtime(&tx);
//    //是星期一就执行下面的
//    int32_t today =local->tm_wday;

//    //后去今天零点的时间戳
//    tm tpNew ;
//    tpNew.tm_year = local->tm_year;
//    tpNew.tm_mon = local->tm_mon;
//    tpNew.tm_mday = local->tm_mday;
//    tpNew.tm_hour = 0;
//    tpNew.tm_min = 0;
//    tpNew.tm_sec = 0;
//    time_t weekzero= mktime(&tpNew); //
//    //今天是周几，然后用今天零点时间减去对应周几，就是这周周一开始的时间戳

//    try
//    {
//        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_daybetting"];
//        auto query_value = document{} << "userid" << userid << "inserttime" << weekzero << finalize;
//        auto seq_updateValue = document{} << "$set" << open_document << "userid" << (int64_t)userid << "agentid" << (int32_t)agentid
//                                          << "inserttime" << weekzero << "daybetting" << (int64_t)betting << "dayprofit" << (int64_t)profit
//                                         <<close_document << finalize;
//        //update options
//        mongocxx::options::update options = mongocxx::options::update{};
//        coll.update_one(query_value.view(), seq_updateValue.view(), options.upsert(true));

//    }
//    catch (const std::exception &e)
//    {
//        LOG_ERROR <<" MongoDB exception: " << e.what();
//    }
//}
//个人库存控制
void CTableFrame::PersonalProfit( shared_ptr<CServerUserItem> & pIServerUserItem )
{
    // 获取引用
    tagPersonalProfit &  _personalProfit = pIServerUserItem->GetPersonalPrott();

    if (!m_pGameInfo->matchforbids[MTH_PERSONALSTOCK])
    {
        _personalProfit.clear();
        return;
    }

    //string totalbet="0", totalprofit="0", agenttendayPro="0";
    /*res = REDISCLIENT.hget(key, REDIS_PERSONALBET, totalbet);
    res = REDISCLIENT.hget(key, REDIS_PERSONALPRO, totalprofit);*/

    string key = REDIS_SCORE_PREFIX + to_string(pIServerUserItem->GetUserId()) + to_string(m_pGameRoomInfo->roomId);

	int64_t totalbet = 0, totalprofit = 0, agenttendayPro = 0;

	string fields[2] = { REDIS_PERSONALBET, REDIS_PERSONALPRO };
	redis::RedisValue values;
	bool ret = REDISCLIENT.hmget(key, fields, 2, values);

	if (ret && !values.empty())
	{
		totalbet = values[REDIS_PERSONALBET].asInt64();
		totalprofit = values[REDIS_PERSONALPRO].asInt64();

		LOG_INFO << ">>> " << __FUNCTION__ << " UserId[" << pIServerUserItem->GetUserId() << "],totalbet[" << totalbet << "],totalprofit[" << totalprofit << "]";

        _personalProfit.playerAllProfit = totalprofit;      //get from redis
        _personalProfit.agentTendayProfit = agenttendayPro; //get from redis
        if (_personalProfit.agentTendayProfit < 1000 * COIN_RATE)//00) //设计要这样子,小于1000 就按1000算
        {
            _personalProfit.agentTendayProfit = 1000 * COIN_RATE;//  00;
        }
        _personalProfit.playerALlbet = totalbet; //get from redis
        _personalProfit.agentRatio = m_pGameRoomInfo->agentRatio;
        _personalProfit.playerBaseValue = m_pGameRoomInfo->personalInventory;
        _personalProfit.playerBetLoseRatio = m_pGameRoomInfo->betLowerLimit;
        _personalProfit.playerBetWinRatio = m_pGameRoomInfo->betHighLimit;
        _personalProfit.playerInterferenceRatio = m_pGameRoomInfo->personalratio;
        _personalProfit.isOpenPersonalAction = 0;

        _personalProfit.playerHighLimit = _personalProfit.playerALlbet * _personalProfit.playerBetWinRatio + _personalProfit.playerBaseValue + _personalProfit.agentTendayProfit * _personalProfit.agentRatio;
        _personalProfit.playerLowerLimit = _personalProfit.playerALlbet * _personalProfit.playerBetLoseRatio - _personalProfit.playerBaseValue - _personalProfit.agentTendayProfit * _personalProfit.agentRatio;

        LOG_ERROR << "playerALlbet: " << totalbet << "agentRatio: " << m_pGameRoomInfo->agentRatio
                  << "playerBaseValue :" << m_pGameRoomInfo->personalInventory << "playerBetLoseRatio"
                  << m_pGameRoomInfo->betLowerLimit << "playerBetWinRatio:" << m_pGameRoomInfo->betHighLimit
                  << "playerInterferenceRatio:" << m_pGameRoomInfo->personalratio;

        if (_personalProfit.playerAllProfit > _personalProfit.playerHighLimit)
        {
            int64_t fanweizhi = _personalProfit.playerHighLimit - _personalProfit.playerLowerLimit;
            if (fanweizhi == 0)
                fanweizhi = 1;
            if ((double)(_personalProfit.playerAllProfit - _personalProfit.playerHighLimit) / (double)fanweizhi > 1)
            {
            }
            else
            {
                _personalProfit.playerInterferenceRatio = _personalProfit.playerInterferenceRatio * ((double)(_personalProfit.playerAllProfit - _personalProfit.playerHighLimit) / (double)fanweizhi);
            }
            _personalProfit.isOpenPersonalAction = 1; //玩家高于上限,杀分
        }
        if (_personalProfit.playerAllProfit < _personalProfit.playerLowerLimit)
        {
            int64_t fanweizhi = _personalProfit.playerHighLimit - _personalProfit.playerLowerLimit;
            if (fanweizhi == 0)
                fanweizhi = 1;
            if ((double)(_personalProfit.playerLowerLimit - _personalProfit.playerAllProfit) / (double)fanweizhi > 1)
            {
            }
            else
            {
                _personalProfit.playerInterferenceRatio = _personalProfit.playerInterferenceRatio * ((double)(_personalProfit.playerLowerLimit - _personalProfit.playerAllProfit) / (double)fanweizhi);
            }
            _personalProfit.isOpenPersonalAction = -1; //玩家低于下限了,放分
        }
        LOG_ERROR << "_personalProfit.playerInterferenceRatio" << _personalProfit.playerInterferenceRatio;
    }
    else
    {
        //读不到就不起作用
        _personalProfit.clear();
    }
}

bool CTableFrame::WriteBlacklistLog(shared_ptr<CServerUserItem>& pIServerUserItem, int status)
{
    try
    { 
        tagBlacklistInfo info = pIServerUserItem->GetBlacklistInfo();
        auto insert_value = bsoncxx::builder::stream::document{}
                            << "userid" << pIServerUserItem->GetUserId()
                            << "account" << pIServerUserItem->GetAccount()
                            << "optime" << bsoncxx::types::b_date(chrono::system_clock::now())
                            << "opaccount" << "GameServer"
                            << "score" << (int32_t)pIServerUserItem->GetUserScore()
                            << "rate" << info.weight
                            << "amount" << info.total
                            << "current" << info.current
                            << "status" << status
                            << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(insert_value);

        mongocxx::collection coll = MONGODBCLIENT["dblog"]["blacklistlog"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if (!result)
        {
            return false;
        }

        coll = MONGODBCLIENT["gamemain"]["game_blacklist"];
        coll.update_one(document{} << "userid" << pIServerUserItem->GetUserId() << finalize,
                        document{} << "$set" << open_document
                                   << "status" << status
                                   << "current" << info.current << close_document
                                   << finalize);
    }
    catch (exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what(); 
        return false;
    }

    return true;
}

bool CTableFrame::WriteSpecialUserScore(tagSpecialScoreInfo* pSpecialScoreInfo, uint32_t nCount, string &strRound)
{
    if (pSpecialScoreInfo && nCount > 0)
    {
         //本次请求开始时间戳(微秒)
        muduo::Timestamp timestart = muduo::Timestamp::now();

        int64_t userId = 0;
        uint32_t chairId = 0;
        mongocxx::client_session dbSession = MONGODBCLIENT.start_session();
        dbSession.start_transaction();
        try
        {
            string mysqlRecord="";
            string mysqlTrange="";
            bool isFirst = false;
            for (uint32_t i = 0; i < nCount; ++i)
            {
                // try to get the special user score info.
                tagSpecialScoreInfo *scoreInfo = (pSpecialScoreInfo + i);

                userId = scoreInfo->userId;
                chairId = scoreInfo->chairId;
                GSUserWriteMysqlInfo UserMysqlInfo;



                if (scoreInfo->bWriteScore)
                {
                    shared_ptr<CServerUserItem> pIServerUserItem;
                    {
                        pIServerUserItem = m_UserList[chairId];
                    }
                    if (pIServerUserItem)
                    {
                        GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();
                        int64_t sourceScore = userBaseInfo.userScore;
                        int64_t targetScore = sourceScore + scoreInfo->addScore;

                        pIServerUserItem->SetUserScore(targetScore);

                        if (!pIServerUserItem->IsAndroidUser())
                        {

                            WritePersonalRedis(userBaseInfo, scoreInfo);
                            UpdateUserScoreToDB(userBaseInfo.userId, scoreInfo);
                            //周业绩写库
                            m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::InsertUserWeekBetting, this,UserMysqlInfo));
                            //InsertUserWeekBetting(scoreInfo->rWinScore,scoreInfo->userId,scoreInfo->agentId,scoreInfo->addScore);
                            //个人库存控制
                            PersonalProfit(pIServerUserItem);
                            if (m_pGameInfo->matchforbids[MTH_BLACKLIST] && pIServerUserItem->GetBlacklistInfo().status == 1)
                            {
                                long current;
                                string blacklistkey = REDIS_BLACKLIST + to_string(pIServerUserItem->GetUserId());
                                bool ret = REDISCLIENT.hincrby(blacklistkey, REDIS_FIELD_CURRENT, -scoreInfo->addScore, &current);
                                if (ret)
                                {
                                    string fields[2] = {REDIS_FIELD_TOTAL, REDIS_FIELD_STATUS};
                                    redis::RedisValue values;
                                    ret = REDISCLIENT.hmget(blacklistkey, fields, 2, values);
                                    if (ret && !values.empty())
                                    {
                                        pIServerUserItem->GetBlacklistInfo().current = current;
                                        if (values[REDIS_FIELD_STATUS].asInt() == 1)
                                        {
                                            if (values[REDIS_FIELD_TOTAL].asInt() <= current)
                                            {
                                                redis::RedisValue redisValue;
                                                redisValue[REDIS_FIELD_STATUS] = (int32_t)0;
                                                REDISCLIENT.hmset(blacklistkey, redisValue);
                                                pIServerUserItem->GetBlacklistInfo().status = 0;
                                                WriteBlacklistLog(pIServerUserItem, 0);
                                            }
                                            else
                                            {
                                                WriteBlacklistLog(pIServerUserItem, 1);
                                            }
                                        }
                                        else
                                        {
                                            pIServerUserItem->GetBlacklistInfo().status = 0;
                                        }
                                    }
                                }
                            }

                            // 更新任务系统
                            TaskService::get_mutable_instance().updateTaskSystem(userBaseInfo.userId, userBaseInfo.agentId, scoreInfo->addScore,
                                                                                 scoreInfo->rWinScore, m_pGameRoomInfo->gameId, m_pGameRoomInfo->roomId);

                            //刷新炸金花个人库存控制数值
                            RefreshKcControlParameter(pIServerUserItem,scoreInfo->addScore+scoreInfo->revenue);
                            UpdateUserRedisBasicInfo(pIServerUserItem->GetUserId(), scoreInfo);
                            if(pIServerUserItem->GetUserStatus()!=sOffline)
                            {
                                SetUserOnlineInfo(userBaseInfo.userId,pIServerUserItem->GetTableId());
                            }

                        }

                    }
                }

                if (scoreInfo->bWriteRecord)
                {
                    ConversionFormat(UserMysqlInfo, scoreInfo, strRound, nCount, false);
                    shared_ptr<CServerUserItem> pIServerUserItem;
                    {
                        pIServerUserItem = m_UserList[chairId];
                    }
                    if (pIServerUserItem)
                    {
                        GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();
                        UserMysqlInfo.lineCode = userBaseInfo.lineCode;
                    }

                    m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WriteRecordMongodb, this,UserMysqlInfo));
                    //写账变记录
                    m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WriteChangeToMongodb, this,UserMysqlInfo));


                }

                char bufer[2000];
                                string tableName ="";

                                time_t tx = time(NULL); //获取目前秒时间
                                struct tm * local;
                                local = localtime(&tx);
                                string months ="";
                                string days = "";
                                if(local->tm_mon+1>=10 ) months = to_string(local->tm_mon+1);
                                else months = "0"+to_string(local->tm_mon+1);
                                tableName = "play_record";
                               int32_t m_year = (local->tm_year+1900)*100+local->tm_mon+1;

                if(!isFirst)
                {
                    mysqlRecord="INSERT INTO  "+tableName+"(userid, agentid, "
                                                                       "gameid,roomid,tableid,chairid,isBanker,"
                                                                       "usercount,beforescore,rwinscore,"
                                                                       "allbet,winscore,score,revenue,isandroid,"
                                                                       "rank,month,cardvalue,gameinfoid,account,linecode,gamestarttime,gameendtime,cellscore,id)";
                   string uuid=GlobalFunc::generateUUID();
                   sprintf(bufer,"VALUES(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                           UserMysqlInfo.userId,UserMysqlInfo.agentId,UserMysqlInfo.gameId,UserMysqlInfo.roomId,UserMysqlInfo.tableId,
                           UserMysqlInfo.chairId,UserMysqlInfo.isBanker,UserMysqlInfo.userCount,UserMysqlInfo.beforeScore,UserMysqlInfo.rWinScore,
                           UserMysqlInfo.allBet,UserMysqlInfo.winScore,UserMysqlInfo.score,UserMysqlInfo.revenue,UserMysqlInfo.isAndroid,UserMysqlInfo.rankId,m_year,UserMysqlInfo.cardValue.c_str(),
                           UserMysqlInfo.strRoundId.c_str(),UserMysqlInfo.account.c_str(),UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.gameStartTime.c_str(),UserMysqlInfo.gameEndTime.c_str(),
                           UserMysqlInfo.cellScore.c_str(),uuid.c_str());
                   mysqlRecord+=bufer;

                   isFirst =true;

                }
                else
                {
                    string uuid=GlobalFunc::generateUUID();
                    sprintf(bufer,",(%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,'%s','%s','%s','%s','%s','%s','%s','%s')",
                            UserMysqlInfo.userId,UserMysqlInfo.agentId,UserMysqlInfo.gameId,UserMysqlInfo.roomId,UserMysqlInfo.tableId,
                            UserMysqlInfo.chairId,UserMysqlInfo.isBanker,UserMysqlInfo.userCount,UserMysqlInfo.beforeScore,UserMysqlInfo.rWinScore,
                            UserMysqlInfo.allBet,UserMysqlInfo.winScore,UserMysqlInfo.score,UserMysqlInfo.revenue,UserMysqlInfo.isAndroid,UserMysqlInfo.rankId,m_year,UserMysqlInfo.cardValue.c_str(),
                            UserMysqlInfo.strRoundId.c_str(),UserMysqlInfo.account.c_str(),UserMysqlInfo.lineCode.c_str(),UserMysqlInfo.gameStartTime.c_str(),UserMysqlInfo.gameEndTime.c_str(),
                            UserMysqlInfo.cellScore.c_str(),uuid.c_str());
                    mysqlRecord+=bufer;
                }
            }
            if(isFirst)
                m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::WtiteUserPlayRecordMysqlx, this,mysqlRecord));
            dbSession.commit_transaction();
        }
        catch (exception &e)
        {
            dbSession.abort_transaction();
            LOG_ERROR << "MongoDB WriteData Error:" << e.what();
            return false;
        }

        // 打印日志
        LOG_INFO << ">>> 耗时[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) << "]秒 " << __FUNCTION__;
    }
    return true;
}
bool CTableFrame::WritePersonalRedis(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo)
{
    bool res=false;
    if(m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::jcfish
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::lkpy
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::rycs
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::dntg)
    {

        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(userBaseInfo.userId)+to_string(m_pGameRoomInfo->roomId);
        //LOG_ERROR<<"roomId"<<m_pGameRoomInfo->roomId<<"    userId"<<userBaseInfo.userId;
        int64_t rake=0;
        //按照洗码量提成
        rake=scoreInfo->addScore+scoreInfo->fishRake;
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,rake ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,scoreInfo->betScore ,&result);
    }
    else
    {
        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(userBaseInfo.userId)+to_string(m_pGameRoomInfo->roomId);
        //LOG_ERROR<<"roomId"<<m_pGameRoomInfo->roomId<<"    userId"<<userBaseInfo.userId;

        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,scoreInfo->addScore+scoreInfo->revenue ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,scoreInfo->betScore ,&result);
    }
    return res;
}
bool CTableFrame::WritePersonalRedis(GSUserBaseInfo &userBaseInfo, tagSpecialScoreInfo *scoreInfo)
{
    bool res=false;
    if(m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::jcfish
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::lkpy
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::rycs
     ||m_pGameRoomInfo->gameId== (uint32_t)eGameKindId::dntg)
    {
        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(userBaseInfo.userId)+to_string(m_pGameRoomInfo->roomId);
        //LOG_ERROR<<"roomId"<<m_pGameRoomInfo->roomId<<"    userId"<<userBaseInfo.userId;
        int64_t rake=0;
        //按照洗码量提成
        rake=scoreInfo->addScore+scoreInfo->fishRake;
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,rake ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,scoreInfo->betScore ,&result);
    }
    else
    {
        int64_t result=0;
        string key1 = REDIS_SCORE_PREFIX + to_string(userBaseInfo.userId)+to_string(m_pGameRoomInfo->roomId);
        //LOG_ERROR<<"roomId"<<m_pGameRoomInfo->roomId<<"    userId"<<userBaseInfo.userId;

        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALPRO,scoreInfo->addScore+scoreInfo->revenue ,&result);
        res = REDISCLIENT.hincrby(key1, REDIS_PERSONALBET,scoreInfo->betScore ,&result);
    }
    return res;
}
void CTableFrame::UpdateUserRedisBasicInfo(int64_t userId, tagSpecialScoreInfo* pScoreInfo)
{
    string keyUserInfo = REDIS_USER_BASIC_INFO + to_string(userId);
    time_t tx = time(NULL); //获取目前秒时间
    struct tm * local;
    local = localtime(&tx);
    struct tm t;
    t.tm_year = local->tm_year ;
    t.tm_mon = local->tm_mon;
    t.tm_mday = local->tm_mday;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    time_t shijianlong= mktime(&t);
    int64_t beginStart=(int64_t)shijianlong;

    if(REDISCLIENT.ExistsUserBasicInfo(userId))
    {
        string date="";
        string nowdate=to_string(beginStart);
        REDISCLIENT.GetUserBasicInfo(userId,REDIS_USER_DATE ,date);

        string fields[2] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET};
        redis::RedisValue values;
        int ret = REDISCLIENT.hmget(keyUserInfo, fields, 2, values);
        int64_t redisdate = values[REDIS_USER_DATE].asInt64();
        int64_t todaybet =  values[REDIS_USER_TODAY_BET].asInt64();
        LOG_ERROR<<"redisdate"<<redisdate<<" beginStart "<<beginStart;
        if(redisdate==beginStart)
        {
             int64_t  current = todaybet+pScoreInfo->betScore;
             redis::RedisValue redisValue;
             redisValue[REDIS_USER_TODAY_BET] = to_string(current);
             REDISCLIENT.hmset(keyUserInfo, redisValue);

        }
        else
        {
            redis::RedisValue redisValue;
            redisValue[REDIS_USER_DATE] = to_string(beginStart);
            redisValue[REDIS_USER_TODAY_BET] = to_string(0);
            redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
            redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
            redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
            redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

            redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
            redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
            REDISCLIENT.hmset(keyUserInfo, redisValue);
        }
    }
    else
    {

        redis::RedisValue redisValue;
        redisValue[REDIS_USER_DATE] = to_string(beginStart);
        redisValue[REDIS_USER_TODAY_BET] = to_string(0);
        redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
        redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
        redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
        redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

        redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
        redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
        string keyUserInfo =REDIS_USER_BASIC_INFO+to_string(userId);
        REDISCLIENT.hmset(keyUserInfo, redisValue);

    }
}
void CTableFrame::UpdateUserRedisBasicInfo(int64_t userId, tagScoreInfo* pScoreInfo)
{
    string keyUserInfo = REDIS_USER_BASIC_INFO + to_string(userId);
    time_t tx = time(NULL); //获取目前秒时间
    struct tm * local;
    local = localtime(&tx);
    struct tm t;
    t.tm_year = local->tm_year ;
    t.tm_mon = local->tm_mon;
    t.tm_mday = local->tm_mday;
    t.tm_hour = 0;
    t.tm_min = 0;
    t.tm_sec = 0;
    time_t shijianlong= mktime(&t);
    int64_t beginStart=(int64_t)shijianlong;

    if(REDISCLIENT.ExistsUserBasicInfo(userId))
    {
        string date="";
        string nowdate=to_string(beginStart);
        REDISCLIENT.GetUserBasicInfo(userId,REDIS_USER_DATE ,date);

        string fields[2] = {REDIS_USER_DATE,REDIS_USER_TODAY_BET};
        redis::RedisValue values;
        int ret = REDISCLIENT.hmget(keyUserInfo, fields, 2, values);
        int64_t redisdate = values[REDIS_USER_DATE].asInt64();
        int64_t todaybet =  values[REDIS_USER_TODAY_BET].asInt64();
        //LOG_ERROR<<"redisdate"<<redisdate<<" beginStart "<<beginStart;
        if(redisdate==beginStart)
        {
             int64_t  current = todaybet+pScoreInfo->betScore;
             redis::RedisValue redisValue;
             redisValue[REDIS_USER_TODAY_BET] = to_string(current);
             REDISCLIENT.hmset(keyUserInfo, redisValue);

        }
        else
        {
            redis::RedisValue redisValue;
            redisValue[REDIS_USER_DATE] = to_string(beginStart);
            redisValue[REDIS_USER_TODAY_BET] = to_string(0);
            redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
            redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
            redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
            redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

            redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
            redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
            REDISCLIENT.hmset(keyUserInfo, redisValue);
        }
    }
    else
    {

        redis::RedisValue redisValue;
        redisValue[REDIS_USER_DATE] = to_string(beginStart);
        redisValue[REDIS_USER_TODAY_BET] = to_string(0);
        redisValue[REDIS_USER_TODAY_UPER] = to_string(0);
        redisValue[REDIS_USER_LEFT_TIMES] = to_string(0);
        redisValue[REDIS_USER_SPRING_STATUS] = to_string(0);
        redisValue[REDIS_USER_SUMMER_STATUS] = to_string(0);

        redisValue[REDIS_USER_FALL_STATUS] = to_string(0);
        redisValue[REDIS_USER_WINTER_STATUS] =to_string(0);
        string keyUserInfo =REDIS_USER_BASIC_INFO+to_string(userId);
        REDISCLIENT.hmset(keyUserInfo, redisValue);

    }
}
void CTableFrame::WtiteUserUserscoreRedisMysql(GSUserWriteMysqlInfo MysqlInfo)
{
    bool bSuccess = false;
    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << MysqlInfo.userId << finalize,
                        document{} << "$inc" << open_document
                                   << "gamerevenue" << MysqlInfo.revenue
                                   << "winorlosescore" << MysqlInfo.winScore
                                   << "integralvalue" << MysqlInfo.rWinScore
                                   << "totalvalidbet" << MysqlInfo.rWinScore
                                   << "score" << MysqlInfo.winScore << close_document
                                   << finalize);

        //LOG_ERROR<<"nihao "<<&MONGODBCLIENT;
        int64_t result = 0;
        string key = REDIS_SCORE_PREFIX + to_string(MysqlInfo.userId);
        bool res = REDISCLIENT.hincrby(key, REDIS_WINSCORE, MysqlInfo.winScore, &result);
        if (res)
        {
            int64_t addscore = 0, subscore = 0, totalWinScore = 0, totalBet = 0;

            string fields[3] = { REDIS_ADDSCORE, REDIS_SUBSCORE,REDIS_WINSCORE };
            redis::RedisValue values;
            bool ret = REDISCLIENT.hmget(key, fields, 3, values);
            if (ret && !values.empty())
            {
                addscore = values[REDIS_ADDSCORE].asInt64();
                subscore = values[REDIS_SUBSCORE].asInt64();
                totalWinScore = values[REDIS_WINSCORE].asInt64();
                totalBet = addscore - subscore;
                LOG_INFO << ">>> UpdateUserScoreToDB addscore[" << addscore << "] " << "subscore[" << subscore << "] " << "totalWinScore[" << totalWinScore << "]";
            }
            if (totalBet == 0)
            {
                totalBet = 1;
            }
            int64_t per = result * 100 / totalBet;
            if (totalWinScore > 0)
            {
                if (per > 200 || per < 0)
                {
                    REDISCLIENT.AddQuarantine(MysqlInfo.userId);
                }
                else
                {
                    REDISCLIENT.RemoveQuarantine(MysqlInfo.userId);
                }
            }
        }
        bSuccess = true;
    }
    catch (exception &e)
    {
        LOG_ERROR << "exception: " << e.what();
    }
}
bool CTableFrame::UpdateUserScoreToDB(int64_t userId, tagScoreInfo* pScoreInfo)
{
    bool bSuccess = false;
    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                                   << "gamerevenue" << pScoreInfo->revenue
                                   << "winorlosescore" << pScoreInfo->addScore
                                   << "integralvalue" << pScoreInfo->rWinScore
                                   << "totalvalidbet" << pScoreInfo->rWinScore
                                   << "score" << pScoreInfo->addScore << close_document
                                   << finalize);

        //LOG_ERROR<<"nihao "<<&MONGODBCLIENT;
        int64_t result = 0;
        string key = REDIS_SCORE_PREFIX + to_string(userId);
        bool res = REDISCLIENT.hincrby(key, REDIS_WINSCORE, pScoreInfo->addScore, &result);
        if (res)
        {
            int64_t addscore = 0, subscore = 0, totalWinScore = 0, totalBet = 0;

            string fields[3] = { REDIS_ADDSCORE, REDIS_SUBSCORE,REDIS_WINSCORE };
            redis::RedisValue values;
            bool ret = REDISCLIENT.hmget(key, fields, 3, values);
            if (ret && !values.empty())
            {
                addscore = values[REDIS_ADDSCORE].asInt64();
                subscore = values[REDIS_SUBSCORE].asInt64();
                totalWinScore = values[REDIS_WINSCORE].asInt64();
                totalBet = addscore - subscore;
                LOG_INFO << ">>> UpdateUserScoreToDB addscore[" << addscore << "] " << "subscore[" << subscore << "] " << "totalWinScore[" << totalWinScore << "]";
            }
            if (totalBet == 0)
            {
                totalBet = 1;
            }
            int64_t per = result * 100 / totalBet;
            if (totalWinScore > 0)
            {
                if (per > 200 || per < 0)
                {
                    REDISCLIENT.AddQuarantine(userId);
                }
                else
                {
                    REDISCLIENT.RemoveQuarantine(userId);
                }
            }
        }
        bSuccess = true;
    }
    catch (exception &e)
    {
        LOG_ERROR << "exception: " << e.what();
    }
    return bSuccess;
}

bool CTableFrame::UpdateUserScoreToDB(int64_t userId, tagSpecialScoreInfo* pScoreInfo)
{
    bool bSuccess = false;
    try
    {

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_user"];
        coll.update_one(document{} << "userid" << userId << finalize,
                        document{} << "$inc" << open_document
                                   << "gamerevenue" << pScoreInfo->revenue
                                   << "winorlosescore" << pScoreInfo->addScore
                                   << "integralvalue"  << pScoreInfo->rWinScore
                                   << "totalvalidbet"  << pScoreInfo->rWinScore
                                   << "score" << pScoreInfo->addScore << close_document
                                   << finalize);
        bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }

    return bSuccess;
}

//bool CTableFrame::AddUserGameInfoToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
//{
//    bool bSuccess = false;
//    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
//    int32_t rankId = 0;
//    if (m_pGameInfo->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
//        //解析-97
//        if (scoreInfo->cardValue.length() >= 5){
//            string str1= scoreInfo->cardValue.substr(0,5);
//            string str2= scoreInfo->cardValue.substr(5);
//            if(str1.compare("---97") == 0){
//                rankId = -97;
//                // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
//                scoreInfo->cardValue = str2;
//                // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
//                // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
//            }
//        }
//    }
//    else if (m_pGameInfo->gameId == (uint32_t)eGameKindId::bbqznn && scoreInfo->cardValue.compare("") != 0) //百变牛的喜钱
//    {
//        int pos = scoreInfo->cardValue.find(",");
//        if(pos >= 0){
//            rankId = -96;
//        }
//    }

//    try
//    {
//        bsoncxx::builder::stream::document builder{};
//        auto insert_value = builder
//                            << "gameinfoid" << strRoundId
//                            << "userid" << userBaseInfo.userId
//                            << "account" << userBaseInfo.account
//                            << "agentid" << (int32_t)userBaseInfo.agentId
//                            << "linecode" << userBaseInfo.lineCode
//                            << "gameid" << (int32_t)m_pGameInfo->gameId
//                            << "roomid" << (int32_t)m_pGameRoomInfo->roomId
//                            << "tableid" << (int32_t)m_TableState.nTableID
//                            << "chairid" << (int32_t)scoreInfo->chairId
//                            << "isBanker" << (int32_t)scoreInfo->isBanker
//                            << "cardvalue" << scoreInfo->cardValue
//                            << "usercount" << userCount
//                            << "rank" << rankId
//                            << "beforescore" << userBaseInfo.userScore
//                            << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
//                            << "cellscore" << open_array;

//        for (int64_t &betscore : scoreInfo->cellScore)
//            insert_value = insert_value << betscore;
            
//        auto after_array = insert_value << close_array;
//        after_array
//            << "allbet" << scoreInfo->betScore
//            << "winscore" << scoreInfo->addScore
//            << "score" << userBaseInfo.userScore + scoreInfo->addScore
//            << "revenue" << scoreInfo->revenue
//            << "isandroid" << bAndroidUser
//            << "gameevent" << scoreInfo->gameEvent
//            << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
//            << "gameendtime" << bsoncxx::types::b_date(scoreInfo->endTime);

//        auto doc = after_array << bsoncxx::builder::stream::finalize;

//        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
//        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
//        if (!result)
//        {
//            LOG_ERROR << "play_record insert exception: ";
//        }

//        bSuccess = true;
//    }
//    catch (const std::exception &e)
//    {
//        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
//    }

//    return bSuccess;
//}
void CTableFrame::WriteRecordMongodb(GSUserWriteMysqlInfo MysqlInfo)
{
    bool bSuccess = false;
    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
    int32_t rankId = 0;
    if (MysqlInfo.gameId == (int32_t)eGameKindId::sgj && MysqlInfo.cardValue.compare("") != 0){
        //解析-97
        if (MysqlInfo.cardValue.length() >= 5){
            string str1= MysqlInfo.cardValue.substr(0,5);
            string str2= MysqlInfo.cardValue.substr(5);
            if(str1.compare("---97") == 0){
                rankId = -97;
                // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
                MysqlInfo.cardValue = str2;
                // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
                // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
            }
        }
    }
    else if (MysqlInfo.gameId == (uint32_t)eGameKindId::bbqznn && MysqlInfo.cardValue.compare("") != 0) //百变牛的喜钱
    {
        int pos = MysqlInfo.cardValue.find(",");
        if(pos >= 0){
            rankId = -96;
        }
    }

    try
    {
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                            << "gameinfoid" << MysqlInfo.strRoundId
                            << "userid" << MysqlInfo.userId
                            << "account" << MysqlInfo.account
                            << "agentid" << (int32_t)MysqlInfo.agentId
                            << "linecode" << MysqlInfo.lineCode
                            << "gameid" << (int32_t)MysqlInfo.gameId
                            << "roomid" << (int32_t)MysqlInfo.roomId
                            << "tableid" << (int32_t)MysqlInfo.tableId
                            << "chairid" << (int32_t)MysqlInfo.chairId
                            << "isBanker" << (int32_t)MysqlInfo.isBanker
                            << "cardvalue" << MysqlInfo.cardValue
                            << "usercount" << MysqlInfo.userCount
                            << "rank" << MysqlInfo.rankId
                            << "beforescore" << MysqlInfo.beforeScore
                            << "rwinscore" << MysqlInfo.rWinScore //2019-6-14 有效投注额
                            << "currency" << MysqlInfo.currency
                            << "cellscore" << open_array;

        for (int64_t &betscore : MysqlInfo.cellScoreIntType)
            insert_value = insert_value << betscore;

        auto after_array = insert_value << close_array;
        after_array
            << "allbet" << MysqlInfo.allBet
            << "winscore" << MysqlInfo.winScore
            << "score" << MysqlInfo.beforeScore + MysqlInfo.winScore
            << "revenue" << MysqlInfo.revenue
            << "isandroid" << (bool)MysqlInfo.isAndroid
            << "gameevent" << MysqlInfo.gameEvent
            << "gamestarttime" << bsoncxx::types::b_date(MysqlInfo.startTime)
            << "gameendtime" << bsoncxx::types::b_date(MysqlInfo.endTime);

        auto doc = after_array << bsoncxx::builder::stream::finalize;

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
        if (!result)
        {
            LOG_ERROR << "play_record insert exception: ";
        }

        bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
}
//bool CTableFrame::AddUserGameInfoToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId, int32_t userCount, bool bAndroidUser)
//{
//    bool bSuccess = false;

//    // 为不重编译全部游戏临时增加的解决方法，后面需要优化
//    int32_t rankId = 0;
//    if (m_pGameInfo->gameId == (int32_t)eGameKindId::sgj && scoreInfo->cardValue.compare("") != 0){
//        //解析-97
//        if (scoreInfo->cardValue.length() >= 5){
//            string str1= scoreInfo->cardValue.substr(0,5);
//            string str2= scoreInfo->cardValue.substr(5);
//            if(str1.compare("---97") == 0){
//                rankId = -97;
//                // LOG_WARN << "提取前的值: " << scoreInfo->cardValue;
//                scoreInfo->cardValue = str2;
//                // LOG_WARN << "提取后的值3: " << str1 <<" "<< str2 <<" "<< rankId;
//                // LOG_WARN << "提取后的值4: " << scoreInfo->cardValue;
//            }
//        }
//    }
//    else if (m_pGameInfo->gameId == (uint32_t)eGameKindId::bbqznn && scoreInfo->cardValue.compare("") != 0) //百变牛的喜钱
//    {
//        int pos = scoreInfo->cardValue.find(",");
//        if(pos >= 0){
//            rankId = -96;
//        }
//    }
//    try
//    {
//        bsoncxx::builder::stream::document builder{};
//        auto insert_value = builder
//                            << "gameinfoid" << strRoundId
//                            << "userid" << scoreInfo->userId
//                            << "account" << scoreInfo->account
//                            << "agentid" << (int32_t)scoreInfo->agentId
//                            << "linecode" << scoreInfo->lineCode
//                            << "gameid" << (int32_t)m_pGameInfo->gameId
//                            << "roomid" << (int32_t)m_pGameRoomInfo->roomId
//                            << "tableid" << (int32_t)m_TableState.nTableID
//                            << "chairid" << (int32_t)scoreInfo->chairId
//                            << "isBanker" << (int32_t)scoreInfo->isBanker
//                            << "cardvalue" << scoreInfo->cardValue
//                            << "usercount" << userCount
//                            << "rank" << rankId
//                            << "beforescore" << scoreInfo->beforeScore
//                            << "rwinscore" << scoreInfo->rWinScore //2019-6-14 有效投注额
//                            << "cellscore" << open_array;
//        for (int64_t &betscore : scoreInfo->cellScore)
//            insert_value = insert_value << betscore;
//        auto after_array = insert_value << close_array;
//        after_array
//            << "allbet" << scoreInfo->betScore
//            << "winscore" << scoreInfo->addScore
//            << "score" << scoreInfo->beforeScore + scoreInfo->addScore
//            << "revenue" << scoreInfo->revenue
//            << "isandroid" << (bool)bAndroidUser
//            << "gameevent" << scoreInfo->gameEvent
//            << "gamestarttime" << bsoncxx::types::b_date(scoreInfo->startTime)
//            << "gameendtime" << bsoncxx::types::b_date(scoreInfo->endTime);

//        auto doc = after_array << bsoncxx::builder::stream::finalize;

//        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(doc);

//        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["play_record"];
//        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
//        if (!result)
//        {
//            LOG_ERROR << "play_record insert_one exception: ";
//        }

//        bSuccess = true;
//    }
//    catch (const std::exception &e)
//    {
//        LOG_ERROR << __FUNCTION__ << " MongoDB exception: " << e.what();
//    }
//    return bSuccess;
//}
void CTableFrame::WriteChangeToMongodb(GSUserWriteMysqlInfo MysqlInfo)
{
    bool bSuccess = false;
    try
    {
        auto insert_value = bsoncxx::builder::stream::document{}
                            << "userid" << MysqlInfo.userId
                            << "account" << MysqlInfo.account
                            << "agentid" << (int32_t)MysqlInfo.agentId
                            << "linecode" << MysqlInfo.lineCode
                            << "changetype" << (int32_t)MysqlInfo.roomId
                            << "changemoney" << MysqlInfo.winScore
                            << "beforechange" << MysqlInfo.beforeScore
                            << "afterchange" << MysqlInfo.beforeScore+MysqlInfo.winScore
                            << "logdate" << bsoncxx::types::b_date(MysqlInfo.endTime)
                            << bsoncxx::builder::stream::finalize;

        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(insert_value);

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if (!result)
        {
             LOG_ERROR << "user_score_record insert_one exception";
        }

        bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
}
//bool CTableFrame::AddScoreChangeRecordToDB(GSUserBaseInfo &userBaseInfo, int64_t sourceScore, int64_t addScore, int64_t targetScore)
//{
//    bool bSuccess = false;
//    try
//    {
//        auto insert_value = bsoncxx::builder::stream::document{}
//                            << "userid" << userBaseInfo.userId
//                            << "account" << userBaseInfo.account
//                            << "agentid" << (int32_t)userBaseInfo.agentId
//                            << "linecode" << userBaseInfo.lineCode
//                            << "changetype" << (int32_t)m_pGameRoomInfo->roomId
//                            << "changemoney" << addScore
//                            << "beforechange" << sourceScore
//                            << "afterchange" << targetScore
//                            << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
//                            << bsoncxx::builder::stream::finalize;

//        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(insert_value);

//        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
//        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
//        if (!result)
//        {
//             LOG_ERROR << "user_score_record insert_one exception";
//        }

//        bSuccess = true;
//    }
//    catch (const std::exception &e)
//    {
//        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
//    }
//    return bSuccess;
//}

//bool CTableFrame::AddScoreChangeRecordToDB(tagSpecialScoreInfo *scoreInfo)
//{
//    bool bSuccess = false;
//   try
//   {
//        auto insert_value = bsoncxx::builder::stream::document{}
//                << "userid" << scoreInfo->userId
//                << "account" << scoreInfo->account
//                << "agentid" << (int32_t)scoreInfo->agentId
//                << "linecode" << scoreInfo->lineCode
//                << "changetype" << (int32_t)m_pGameRoomInfo->roomId
//                << "changemoney" << scoreInfo->addScore
//                << "beforechange" << scoreInfo->beforeScore
//                << "afterchange" << scoreInfo->beforeScore + scoreInfo->addScore
//                << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
//                << bsoncxx::builder::stream::finalize;

//        LOG_DEBUG << "Insert Document:"<<bsoncxx::to_json(insert_value);

//        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["user_score_record"];
//        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
//        if(!result)
//        {
//            LOG_ERROR << "user_score_record insert_one exception: ";
//        }

//        bSuccess = true;
//    }
//    catch (const std::exception &e)
//    {
//        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
//    }
//    return bSuccess;
//}

bool CTableFrame::AddUserGameLogToDB(GSUserBaseInfo &userBaseInfo, tagScoreInfo *scoreInfo, string &strRoundId)
{
    bool bSuccess = false;
    try
    {
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

        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(insert_value);

        // mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_log"];
        // bsoncxx::stdx::optional<mongocxx::result::insert_one> ins_result = coll.insert_one(insert_value.view());
        // if (!ins_result)
        // {
        //     LOG_ERROR << "game_log insert_one exception";
        // }

        bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
    return bSuccess;
}

bool CTableFrame::AddUserGameLogToDB(tagSpecialScoreInfo *scoreInfo, string &strRoundId)
{
    bool bSuccess = false;
    try
    {
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

        LOG_DEBUG << "Insert Document:" << bsoncxx::to_json(insert_value);

        // mongocxx::collection coll = MONGODBCLIENT["gamemain"]["game_log"];
        // bsoncxx::stdx::optional<mongocxx::result::insert_one> ins_result = coll.insert_one(insert_value.view());
        // if (!ins_result)
        // {
        //     LOG_ERROR << "game_log insert_one exception 2";
        // }

        bSuccess = true;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
    return bSuccess;
}

// 奖池消息 JackpotBroadCast
//@ optype:0 为累加，1为设置
bool CTableFrame::UpdateJackpot(int32_t optype,int64_t incScore,int32_t JackpotId,int64_t *newJackPot)
{
    int maxSize = sizeof(m_pGameRoomInfo->totalJackPot)/sizeof(int64_t);
    // 
    // LOG_WARN << __FUNCTION__ << " " << optype << " "<< incScore <<" + "<< m_TotalJackPot[JackpotId] << " "<< maxSize;

    if(JackpotId >= maxSize){
        LOG_ERROR << " JackpotId Error : "<< incScore <<" to " << JackpotId;
        return false;
    }
    // 组合
    //特殊处理经典水果机和新水果机的奖池信息
    int32_t  gameid = m_pGameRoomInfo->gameId;
    if( gameid == (int32_t)eGameKindId::xsgj ) gameid = (int32_t)eGameKindId::sgj;
    string fieldName = std::to_string(gameid);
    string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_lkJackpot);
 
    // 累加
    bool res = REDISCLIENT.hincrby(keyName,fieldName,incScore,newJackPot);
    if(res){
        LOG_WARN << "Jackpot Changed From : "<< incScore << " + "<< m_TotalJackPot[JackpotId] <<" to " << (int64_t)(*newJackPot);
        m_TotalJackPot[JackpotId] = *newJackPot;
        m_pGameRoomInfo->totalJackPot[JackpotId] = *newJackPot;
        // LOG_WARN <<" "<<m_pGameRoomInfo->totalJackPot[0]<<" "<<m_pGameRoomInfo->totalJackPot[1]<<" "<<m_pGameRoomInfo->totalJackPot[2]<<" "<<m_pGameRoomInfo->totalJackPot[3]<<" "<<m_pGameRoomInfo->totalJackPot[4];
    }

    // LOG_ERROR << "hincrby Jackpot end ";
    return res;
}
// 读取彩金值
int64_t CTableFrame::ReadJackpot( int32_t JackpotId )
{
    int maxSize = sizeof(m_pGameRoomInfo->totalJackPot)/sizeof(int64_t);
    if( JackpotId >= maxSize ){ return -1; }
    // 
    if (m_pGameRoomInfo->totalJackPot[JackpotId] == 0){
        // 组装字段
        //特殊处理经典水果机和新水果机的奖池信息
        int32_t gameid=m_pGameRoomInfo->gameId;
        if(gameid == (int32_t)eGameKindId::xsgj)
        {
            gameid = (int32_t)eGameKindId::sgj;
        }
        string fieldName = std::to_string(gameid);
        string keyName = REDIS_KEY_ID + std::to_string((int)eRedisKey::has_lkJackpot);

        int64_t newJackPot = 0;
        // 累加
        bool res = REDISCLIENT.hincrby(keyName,fieldName,0,&newJackPot);
        if(res){
            m_TotalJackPot[JackpotId] = newJackPot;
            LOG_WARN << "ReadJackpot: "<< m_pGameRoomInfo->totalJackPot[JackpotId] << " + "<< m_TotalJackPot[JackpotId] <<" to " << newJackPot;
            m_pGameRoomInfo->totalJackPot[JackpotId] = newJackPot;
            // LOG_WARN <<" "<<m_pGameRoomInfo->totalJackPot[0]<<" "<<m_pGameRoomInfo->totalJackPot[1]<<" "<<m_pGameRoomInfo->totalJackPot[2]<<" "<<m_pGameRoomInfo->totalJackPot[3]<<" "<<m_pGameRoomInfo->totalJackPot[4];
        }
        else{
            LOG_ERROR <<keyName <<" "<<fieldName << " 读取彩金失败: "<<JackpotId <<" "<< (int64_t)m_TotalJackPot[JackpotId] <<" - " << (int64_t)m_pGameRoomInfo->totalJackPot[JackpotId];
        }
    }
    return m_pGameRoomInfo->totalJackPot[JackpotId];
}

// 公共方法
//  return -1,调用失败；返回0代表成功
int32_t CTableFrame::CommonFunc(eCommFuncId funcId, std::string &jsonInStr, std::string &jsonOutStr )
{  
    Json::Reader reader;
    Json::Value root;

    LOG_INFO << __FUNCTION__<<" "<<(int)funcId;

    // 写水果机奖池记录
    if(funcId == eCommFuncId::fn_sgj_jackpot_rec){
        if(reader.parse(jsonInStr, root))
        {
            int32_t msgCount = root["msgCount"].asInt64();
            int32_t msgType = root["msgType"].asInt();
            if (msgCount > 20)  msgCount = 20; //防止误传

            // 写
            if(msgType == 0){
                string msgstr = root["msgstr"].asString();
                if(!SaveJackpotRecordMsg(msgstr,msgCount)) return -1;  
            }
            else if(msgType == 1){// 读
                vector<string> msgLists;
                REDISCLIENT.lrangeCmd(eRedisKey::lst_sgj_JackpotMsg, msgLists, msgCount);
                // 返回
                int32_t idx = 0;
                Json::Value root;
                for(string msg : msgLists){
                    Json::Value value;
                    value["Idx"] = idx++;
                    value["msg"] = msg;
                    root.append(value);
                }
                Json::FastWriter write;
                jsonOutStr = write.write(root);
            }
        }
        return 0;
    }
    else if(funcId == eCommFuncId::fn_id_1)
    {
        // 拆分参数
        if(reader.parse(jsonInStr, root))
        {
            string details = jsonOutStr;
            int32_t agentid = m_pGameRoomInfo->agentId;//
            int32_t gameid = m_pGameInfo->gameId;// root["gameid"].asUInt();//游戏ID
            int32_t roomid = m_pGameRoomInfo->roomId;// root["roomid"].asUInt();//房间ID
            int32_t status = m_pGameRoomInfo->serverStatus;//root["status"].asInt();//桌子启用状态
            string roomName = m_pGameRoomInfo->roomName;//root["roomName"].asString();   //房间名称
            
            int32_t tableid = root["tableid"].asInt();//桌子ID
            int32_t gamestatus = root["gamestatus"].asInt(); //游戏状态
            int32_t time = root["time"].asInt();             //游戏倒计时
			int32_t goodroutetype = root["goodroutetype"].asInt(); //好路类型
			string routetypename = root["routetypename"].asString();   //好路名称
			string ip = m_pGameRoomInfo->serverIP;

            int32_t currency = m_pGameRoomInfo->currency;
			int32_t ipcode = 0;
			vector<string> addrVec;
			addrVec.clear();
			boost::algorithm::split(addrVec, ip, boost::is_any_of(":"));			
			if (addrVec.size() == 3)
			{
				vector<string> ipVec;
				ipVec.clear();
				boost::algorithm::split(ipVec, addrVec[1], boost::is_any_of("."));
				for (int i = 0; i < ipVec.size();i++)
				{
					ipcode += stoi(ipVec[i]);
				}
			}
            //LOG_DEBUG << gameid << " " << roomid << " " << tableid << " " << status << " " << gamestatus << " " << time << " " << roomName << " ip:" << ip << " agentid:" << agentid;

            try
            {
                //update options
                mongocxx::options::update options = mongocxx::options::update{};
                bsoncxx::document::value findValue = document{} << "agentid" << agentid << "gameid" << gameid << "roomid" << roomid << "tableid" << tableid << "ipcode" << ipcode << finalize;
                mongocxx::collection room_route_coll = MONGODBCLIENT["gamemain"]["room_route_record"];
                bsoncxx::document::value updateValue = document{}
                                                       << "$set" << open_document
                                                       << "agentid" << agentid
                                                       << "gameid" << gameid
                                                       << "ip" << ip
                                                       << "ipcode" << ipcode
                                                       << "roomid" << roomid
                                                       << "tableid" << tableid
                                                       << "gamestatus" << gamestatus
                                                       << "status" << status
                                                       << "time" << time
                                                       << "roomName" << roomName
                                                       << "goodroutetype" << goodroutetype
													   << "routetypename" << routetypename
                                                       << "details" << details
                                                       << "currency" << currency
                                                       << "updatetime" << bsoncxx::types::b_date{chrono::system_clock::now()} //更新时间
                                                       << close_document << finalize;
                // 没有记录内插入
                room_route_coll.update_one(findValue.view(), updateValue.view(), options.upsert(true));
            }
            catch (const std::exception &e)
            {
                LOG_ERROR << __FUNCTION__ << " MongoDB exception: " << e.what();
            }
        }
    }
 
    return 0;
}


// 存储奖池信息
// @msgStr 消息体
// @msgCount 消息数量
bool CTableFrame::SaveJackpotRecordMsg(string &msgStr,int32_t msgCount)
{
    bool result = false;
    int repeatCount = 10;//重试10次后放弃存储
    std::string lockStr = to_string(::getpid());
    do
    {
        int ret = REDISCLIENT.setnxCmd(eRedisKey::str_lockId_jp_2, lockStr, 2);
        if (ret == -1)
        {
            LOG_ERROR << "redis连接失败" << __FUNCTION__;
            break;
        }
        if (ret == 1)
        {
            long long msgListLen = 0;
            REDISCLIENT.lpushCmd(eRedisKey::lst_sgj_JackpotMsg, msgStr, msgListLen);

            //7.3移除超出的数量
            if (msgListLen > msgCount)
            {
                for (int idx = 0; idx < (msgListLen - msgCount); idx++)
                {
                    std::string lastElement;
                    REDISCLIENT.rpopCmd(eRedisKey::lst_sgj_JackpotMsg, lastElement);
                    LOG_WARN << "存储和移除消息[" << msgStr << " " << lastElement << "]";
                }
            }

            // 解锁(不是很有效)
            if (!REDISCLIENT.delnxCmd(eRedisKey::str_lockId_jp_2, lockStr)){
                LOG_ERROR << "解锁失败[" << lockStr << "]" << __FUNCTION__;;
            }

            result = true;
            break;
        }
        else{
            usleep(10 * 1000); // 10毫秒
            LOG_WARN << "10毫秒延时 " << repeatCount;
        }
    } 
    while (--repeatCount > 0);

    return result;
}

 
//===========================STORAGE SCORE===========================
int CTableFrame::UpdateStorageScore(int64_t changeStockScore)
{
    static int count = 0;
    m_CurStock += changeStockScore;
    if(++count > 0)
    {
        WriteGameChangeStorage(changeStockScore);
        count = 0;
    }
    return count;
}

bool CTableFrame::GetStorageScore(tagStorageInfo &storageInfo)
{
    /*
    storageInfo.lLowlimit = m_LowestStock;
    storageInfo.lUplimit = m_HighStock;
    storageInfo.lEndStorage = m_CurStock;
    storageInfo.lSysAllKillRatio = m_SystemAllKillRatio;
    storageInfo.lReduceUnit = m_SystemReduceRatio;
    storageInfo.lSysChangeCardRatio = m_ChangeCardRatio;
    */

    storageInfo.lLowlimit           = m_pGameRoomInfo->totalStockLowerLimit;;
    storageInfo.lUplimit            = m_pGameRoomInfo->totalStockHighLimit;
    storageInfo.lEndStorage         = m_pGameRoomInfo->totalStock;
    storageInfo.lSysAllKillRatio    = m_pGameRoomInfo->systemKillAllRatio;
    storageInfo.lReduceUnit         = m_pGameRoomInfo->systemReduceRatio;
    storageInfo.lSysChangeCardRatio = m_pGameRoomInfo->changeCardRatio;

    return true;
}

bool CTableFrame::ReadStorageScore()
{
//    try
//    {
//        auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_PLATFORM), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });

//        pstmt.reset(conn->prepareStatement("SELECT total_stock, total_stock_lower_limit, total_stock_high_limit, "
//                                           " system_all_kill_ratio, system_reduce_ratio, change_card_ratio "
//                                           " FROM room_server WHERE configid=?"));
//        pstmt->setInt(1, m_pRoomKindInfo->nConfigId);
//        res.reset(pstmt->executeQuery());
//        if(res->rowsCount() > 0 && res->next())
//        {
//            m_CurStock = res->getDouble(1);
//            m_LowestStock = res->getDouble(2);
//            m_HighStock = res->getDouble(3);

//            m_SystemAllKillRatio = res->getInt(4);
//            m_SystemReduceRatio = res->getInt(5);
//            m_ChangeCardRatio = res->getInt(6);
//        }
//    }catch(sql::SQLException& e)
//    {
//        LOG_ERROR << "SQLException: " << e.what() << "sql state:" << e.getSQLState() << ", error code:" << e.getErrorCode();
//    }

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
    try
    {
        int32_t   roomId = m_pGameRoomInfo->roomId;

        //特殊处理新水果机和经典水果公用数据问题
        if(roomId == XSGJ_ROOM_ID)
        {
            roomId = SGJ_ROOM_ID;
        }

        mongocxx::collection coll = MONGODBCLIENT["gameconfig"]["game_kind"];
        coll.update_one(document{} <<"agentid"<<(int32_t)m_pGameRoomInfo->agentId<<"rooms.roomid" << (int32_t)roomId << finalize,
                        document{} << "$inc" << open_document
                                   << "rooms.$.totalstock" << changeStockScore << close_document
                                   << finalize);
        int64_t newStock;
        if (REDISCLIENT.hincrby(REDIS_CUR_STOCKS+to_string(m_pGameRoomInfo->agentId), to_string(roomId), changeStockScore, &newStock))
        {
            LOG_INFO << "Stock Changed From : "<< changeStockScore <<" + " << (int64_t)m_pGameRoomInfo->totalStock << "=" << (int64_t)newStock;
            m_pGameRoomInfo->totalStock = newStock;
            m_CurStock = newStock;
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
    return true;
} 

bool CTableFrame::SaveReplay(tagGameReplay replay) {

    if(replay.saveAsStream)
    {

        m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::SaveReplayDetailBlob, this,replay));
        //SaveReplayDetailBlob(replay);
        return true;
    }
    else
    {
        m_game_dbsql_thread->getLoop()->runInLoop(bind(&CTableFrame::SaveReplayDetailJson, this,replay));
        //SaveReplayDetailJson(replay);
        return false;
    }
}

bool CTableFrame::SaveKillFishBossRecord(GSKillFishBossInfo& bossinfo)
{

    time_t todayHighest,weekHighest;
    time_t tx = time(NULL);
    struct tm * local;
    local = localtime(&tx);
    tm *tpNew = new tm;
    tpNew->tm_year = local->tm_year;
    tpNew->tm_mon = local->tm_mon;
    tpNew->tm_mday = local->tm_mday;
    tpNew->tm_hour = 0;
    tpNew->tm_min = 0;
    tpNew->tm_sec = 0;
    todayHighest = mktime(tpNew);
    weekHighest =todayHighest-(local->tm_wday-1)*24*60*60;
    delete tpNew;

    try
    {
        bsoncxx::builder::stream::document builder{};
        auto insert_value = builder
                            << "userid" << bossinfo.userId
                            << "agentid" << (int32_t)bossinfo.agentId
                            << "fishid" << bossinfo.fishType
                            << "gameid" << (int32_t)m_pGameRoomInfo->gameId
                            << "roomid" << (int32_t)m_pGameRoomInfo->roomId
                            << "paobei" << (int32_t)bossinfo.cannonLevel
                            << "odds" << (int32_t)bossinfo.fishAdds
                            << "win" << bossinfo.winScore
                            << "currency" << (int32_t)m_pGameRoomInfo->currency
                            << "logdate" << bsoncxx::types::b_date(chrono::system_clock::now())
                            << "weeklogtime" << weekHighest
                            << bsoncxx::builder::stream::finalize;

        mongocxx::collection coll = MONGODBCLIENT["gamemain"]["fishing_rank_record"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(insert_value.view());
        if (!result)
        {
            LOG_ERROR << "fishing_rank_record insert exception: ";
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ <<" MongoDB exception: " << e.what();
    }
}


bool CTableFrame::SaveReplayDetailBlob(tagGameReplay replay)
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

	insert_value << close_array << "bairendetails" << open_array;
	for (tagRecordDetail &detail : replay.bairendetails)
	{
		if (detail.flag)
		{
			insert_value = insert_value << bsoncxx::builder::stream::open_document
				<< "userid" << detail.userid
				<< "chairid" << detail.chairId
				<< "detailData" << bsoncxx::types::b_binary{ bsoncxx::binary_sub_type::k_binary, uint32_t(detail.detailData.size()), reinterpret_cast<uint8_t const*>(detail.detailData.data()) }
				<< bsoncxx::builder::stream::close_document;
		}
	}
    auto after_array = insert_value << close_array;

    auto doc = after_array << bsoncxx::builder::stream::finalize;

    try
    {
        mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
        bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << __FUNCTION__ << " MongoDB exception: " << e.what();
    }

//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });


//        string playersStr,stepsStr,resultsStr,bairendetailsStr;

//        Json::Value root;
//        Json::FastWriter write;
//        for(tagReplayPlayer &player : replay.players)
//        {
//            Json::Value value;
//            value["chairid"] = player.chairid;
//            value["account"] = player.accout;
//            value["score"] = player.score;
//            value["userid"] = player.userid;
//            root.append(value);
//        }
//        playersStr = write.write(root);

//        Json::Value root1;
//        Json::FastWriter write1;
//        for(tagReplayStep &step : replay.steps)
//        {
//            Json::Value value;
//            value["bet"] = step.bet;
//            value["time"] = step.time;
//            value["ty"] = step.ty;
//            value["round"] = step.round;
//            value["chairid"] = step.chairId;
//            value["pos"] = step.pos;
//            root1.append(value);
//        }
//        stepsStr=write1.write(root1);

//        Json::Value root2;
//        Json::FastWriter write2;
//        for(tagReplayResult &result : replay.results)
//        {
//            Json::Value value;
//            value["chairid"] = result.chairId;
//            value["bet"] = result.bet;
//            value["pos"] = result.pos;
//            value["win"] = result.win;
//            value["cardtype"] = result.cardtype;
//            value["isbanker"] = result.isbanker;
//            root2.append(value);
//        }
//        resultsStr=write2.write(root2);

//        Json::Value root3;
//        Json::FastWriter write3;
//        for(tagRecordDetail &detail : replay.bairendetails)
//        {
//            if (detail.flag)
//            {
//                Json::Value value;
//                value["userid"] = detail.userid;
//                value["chairid"] = detail.chairId;
//                value["detailData"] = detail.detailData;
//                root3.append(value);
//            }

//        }
//        bairendetailsStr=write3.write(root3);

//        conn->setSchema(DB_RECORD);

//        char bufer[2000];
//        string tableName ="";
//        string mysqlRecord ="";
//        time_t tx = time(NULL); //获取目前秒时间
//        struct tm * local;
//        local = localtime(&tx);
//        string months ="";
//        string days = "";
//        if(local->tm_mon+1>=10 ) months = to_string(local->tm_mon+1);
//        else months = "0"+to_string(local->tm_mon+1);
//        tableName = "game_replay";
//        int32_t m_year = (local->tm_year+1900)*100+local->tm_mon+1;
//        mysqlRecord="INSERT INTO "+ tableName+" (gameinfoid,"
//                                             "timestamp,roomname,cellscore,players,steps,"
//                                             "results,bairendetails,month) "
//                                             "VALUES(?,?,?,?,?,?,?,?,?)";


//        pstmt.reset(conn->prepareStatement(mysqlRecord));

//        pstmt->setString(1, replay.gameinfoid);
//        pstmt->setDateTime(2,GlobalFunc::InitialConversion(time(NULL)));
//        pstmt->setString(3, replay.roomname);
//        pstmt->setInt(4, replay.cellscore);
//        pstmt->setString(5, playersStr);
//        pstmt->setString(6, stepsStr);
//        pstmt->setString(7,  resultsStr);
//        pstmt->setString(8,  bairendetailsStr);
//        pstmt->setInt(9,  m_year);
//        pstmt->executeUpdate();
//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
    return true;
}

bool CTableFrame::SaveReplayDetailJson(tagGameReplay replay)
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
	
	insert_value << close_array << "bairendetails" << open_array;
	for (tagRecordDetail &detail : replay.bairendetails)
	{
		if (detail.flag)
		{
			insert_value = insert_value << bsoncxx::builder::stream::open_document
				<< "userid" << detail.userid
				<< "chairid" << detail.chairId
				<< "detailData" << std::move(detail.detailData)
			<< bsoncxx::builder::stream::close_document;
		}
	}
	auto after_array = insert_value << close_array;

	auto doc = after_array << bsoncxx::builder::stream::finalize;

	mongocxx::collection coll = MONGODBCLIENT["gamelog"]["game_replay"];
	bsoncxx::stdx::optional<mongocxx::result::insert_one> result = coll.insert_one(doc.view());
//mongdb插入多条数据的用法
//    std::vector< bsoncxx::document::value > documents;
//    for (size_t i = 0; i < list.size(); i++){
//           documents.push_back(document{}
//               << "field 1" << list[i].get_data_1()
//               << "field 2" << list[i].get_data_2()
//               << finalize);
//       }

//    coll.insert_many(documents);




//    try
//    {
//        auto fuc = std::bind(&ConnectionPool::ReleaseConnection,ConnectionPool::GetInstance(), std::placeholders::_1);
//        shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//        shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//        shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });


//        string playersStr,stepsStr,resultsStr,bairendetailsStr;

//        Json::Value root;
//        Json::FastWriter write;
//        for(tagReplayPlayer &player : replay.players)
//        {
//            Json::Value value;
//            value["chairid"] = player.chairid;
//            value["account"] = player.accout;
//            value["score"] = player.score;
//            value["userid"] = player.userid;
//            root.append(value);
//        }
//        playersStr = write.write(root);

//        Json::Value root1;
//        Json::FastWriter write1;
//        for(tagReplayStep &step : replay.steps)
//        {
//            Json::Value value;
//            value["bet"] = step.bet;
//            value["time"] = step.time;
//            value["ty"] = step.ty;
//            value["round"] = step.round;
//            value["chairid"] = step.chairId;
//            value["pos"] = step.pos;
//            root1.append(value);
//        }
//        stepsStr=write1.write(root1);

//        Json::Value root2;
//        Json::FastWriter write2;
//        for(tagReplayResult &result : replay.results)
//        {
//            Json::Value value;
//            value["chairid"] = result.chairId;
//            value["bet"] = result.bet;
//            value["pos"] = result.pos;
//            value["win"] = result.win;
//            value["cardtype"] = result.cardtype;
//            value["isbanker"] = result.isbanker;
//            root2.append(value);
//        }
//        resultsStr=write2.write(root2);

//        Json::Value root3;
//        Json::FastWriter write3;
//        for(tagRecordDetail &detail : replay.bairendetails)
//        {
//            if (detail.flag)
//            {
//                Json::Value value;
//                value["userid"] = detail.userid;
//                value["chairid"] = detail.chairId;
//                value["detailData"] = detail.detailData;
//                root3.append(value);
//            }

//        }
//        bairendetailsStr=write3.write(root3);

//        conn->setSchema(DB_RECORD);

//        char bufer[2000];
//        string tableName ="";
//        string mysqlRecord ="";
//        time_t tx = time(NULL); //获取目前秒时间
//        struct tm * local;
//        local = localtime(&tx);
//        string months ="";
//        string days = "";
//        if(local->tm_mon+1>=10 ) months = to_string(local->tm_mon+1);
//        else months = "0"+to_string(local->tm_mon+1);
//        tableName = "game_replay";
//        int32_t m_year = (local->tm_year+1900)*100+local->tm_mon+1;
//        mysqlRecord="INSERT INTO "+ tableName+" (gameinfoid,"
//                                             "timestamp,roomname,cellscore,players,steps,"
//                                             "results,bairendetails,month) "
//                                             "VALUES(?,?,?,?,?,?,?,?,?)";


//        pstmt.reset(conn->prepareStatement(mysqlRecord));

//        pstmt->setString(1, replay.gameinfoid);
//        pstmt->setDateTime(2,GlobalFunc::InitialConversion(time(NULL)));
//        pstmt->setString(3, replay.roomname);
//        pstmt->setInt(4, replay.cellscore);
//        pstmt->setString(5, playersStr);
//        pstmt->setString(6, stepsStr);
//        pstmt->setString(7,  resultsStr);
//        pstmt->setString(8,  bairendetailsStr);
//        pstmt->setInt(9,  m_year);
//        pstmt->executeUpdate();
//    }
//    catch (sql::SQLException &e)
//    {
//        std::cout << " (MySQL error code: " << e.getErrorCode();
//    }
	return true;
}

void CTableFrame::DeleteUserToProxy(shared_ptr<CServerUserItem>& pIServerUserItem, int32_t nKickType)
{
    do
    {
        if (!pIServerUserItem)
            break;

        int64_t userId = pIServerUserItem->GetUserId();
        LOG_DEBUG << " >>> DeleteUserToProxy userid:" << userId;

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
//	LOG_DEBUG << "--- *** " << "userid = " << userId;

    bool b1 = false, b2 = false;
    do
    {
        string strKeyName = REDIS_ONLINE_PREFIX + to_string(userId);
        if (bonlyExpired)
        {
//            b2 = redisClient->resetExpiredUserIdGameServerInfo(userid);
//            b1 = redisClient->resetExpired(strKeyName);

            b1 = REDISCLIENT.ResetExpiredUserOnlineInfo(userId);
//            LOG_ERROR << " >>> DelUserOnlineInfo, set expired only, key:" << strKeyName << ", b1:" << b1
 //                     << "online key:" << strKeyName << ", userid:" << userId;
        }
        else
        {
//            b2 = redisClient->delUserIdGameServerInfo(userId);
//            b1 = redisClient->del(strKeyName);
            if(REDISCLIENT.ExistsUserLoginInfo(userId)){
                LOG_ERROR<<"not instance";
            }

            b1 = REDISCLIENT.DelUserOnlineInfo(userId);

            LOG_ERROR << " >>> DelUserOnlineInfo  online key:" << strKeyName << "b1 = "<< b1 << ", userid:" << userId;
        }

    }while (0);
//Cleanup:
    return (b1);
}

bool CTableFrame::SetUserOnlineInfo(int64_t userId,int32_t tableId)
{
    bool bSuccess = false;

    REDISCLIENT.SetUserOnlineInfo(userId, m_pGameRoomInfo->gameId, m_pGameRoomInfo->roomId,tableId);

    //LOG_WARN << "---user["<< userId <<"] playing gameId[" << m_pGameRoomInfo->gameId <<"],roomId["<< m_pGameRoomInfo->roomId<<"]";

    return bSuccess;
}

bool CTableFrame::RoomSitChair(shared_ptr<CServerUserItem>& pIServerUserItem, Header *commandHeader, bool bDistribute)
{
//    muduo::MutexLockGuard lockSit(m_lockSitMutex);
//    muduo::RecursiveLockGuard lock(m_RecursiveMutextLockEnterLeave);
    int64_t userId = pIServerUserItem->GetUserId();
    // LOG_DEBUG << __FUNCTION__ <<  " userId:" << userId;

    do
    {
        if(!pIServerUserItem || pIServerUserItem->GetUserId() <= 0)
        {
            LOG_ERROR << "RoomSitChair Sit Error " << pIServerUserItem->GetUserId();
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
// 
            // LOG_DEBUG << ">>> room sit chairId:" << chairId << ", userId:" << userId;
            pIServerUserItem->setClientReady(true);
            pIServerUserItem->SetUserStatus(sSit);

            //
            OnUserEnterAction(pIServerUserItem, commandHeader, bDistribute);

            if(!pIServerUserItem->IsAndroidUser())
            {
                SetUserOnlineInfo(userId, pIServerUserItem->GetTableId());
//                redis::RedisValue redisValue;
//                string fields[1]   = {REDIS_FIELD_STATUS};
//                bool exists = REDISCLIENT.hmget(REDIS_BLACKLIST+to_string(userId),fields,1,redisValue);
//                if(exists)
//                    pIServerUserItem->GetBlacklistInfo().status = redisValue[REDIS_FIELD_STATUS].asUInt();
            }            


            int64_t userScore = pIServerUserItem->GetUserScore();

//            LOG_WARN << ">>> sit down userId: " << userId
//                     << " at table id(" << m_TableState.nTableID << "," << chairId
//                     << "), score:" << userScore ;

            return true;
        }
    }while(0);

    return false;
}

//bool CTableFrame::TableUserLeft(shared_ptr<CServerUserItem> &pUserItem, bool bSendToOther)
//{
//    int mainId = Game::Common::MSGGS_MAIN_FRAME;
//    int subId = GameServer::Message::SUB_S2C_USER_LEFT;
//    ::GameServer::Message::MSG_C2S_UserLeft_RES response;
//    ::Game::Common::Header *header = response.mutable_header();
//    header->set_sign(HEADER_SIGN);
//    header->set_crc(0);
//    header->set_mainid(mainId);
//    header->set_subid(subId);

//    response.set_userid(pUserItem->GetUserID());
//    response.set_gameid(m_pRoomKindInfo->nGameId);
//    response.set_score(pUserItem->GetUserScore());

//    response.set_retcode(0);
//    response.set_errormsg("UserLeft OK!");

//    string content = response.SerializeAsString();
//    if(bSendToOther)
//    {
//        int wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
//        for(int chairid = 0; chairid < wMaxPlayer; ++chairid)
//        {
//            shared_ptr<CServerUserItem> pTargetUserItem;
//            {
//                READ_LOCK(m_list_mutex);
//                pTargetUserItem = m_UserList[chairid];
//            }

//            // check the user item value now.
//            if (!pTargetUserItem)
//            {
//                LOG_INFO<<"++++++++++++++++++++++++pTargetUserItem==NULL,chairid:" << chairid;
//                continue;
//            }

//            SendPackedData(pTargetUserItem, mainId, subId, content.data(),content.size());
//        }
//    }else
//    {
//        SendPackedData(pUserItem, mainId, subId, content.data(), content.size());
//    }
//    return true;
//}

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
//        if(!bisOut)
//        {
//            // update the special user score information for content item now.
//            GS_UserScoreInfo& scoreInfo = pIserverUserItem->GetUserScoreInfo();
//            scoreInfo.nEnterGem = pIserverUserItem->GetUserGem();
//            scoreInfo.nEnterScore = pIserverUserItem->GetUserScore();
//            scoreInfo.nEnterTime = (dword)time(NULL);
//            ptime timeStart = second_clock::local_time();
//            string strTime = to_iso_extended_string(timeStart);
//            strncpy(scoreInfo.szEnterTime, strTime.c_str(), sizeof(scoreInfo.szEnterTime));

////            // get the special suer parameter data now.
////            int userId  = pIserverUserItem->GetUserID();
////            int kindId  = m_pRoomKindInfo->nKindId;
////            int nRoomId = m_pRoomKindInfo->nServerID;
////            int nConfigId = m_pRoomKindInfo->nConfigId;
////            SCORE lEnterGem = pIserverUserItem->GetUserGem();
////            SCORE lEnterScore = pIserverUserItem->GetUserScore();
////            string strEnterMachine = scoreInfo.szMachine;
////            string strIp = scoreInfo.szIp;
////            strncpy(scoreInfo.szCurMonth, curStrMonth.c_str(), 6);

////            string account = pIserverUserItem->getAccount();
////            string sql = "INSERT INTO record_user_inout_"+curStrMonth+
////                    "(userid, account,gameid, kindid, roomid, configid, enter_time, enter_gem, enter_score,"
////                    " enter_machine, enter_ip, bAndroid) VALUES (?,?,?,"
////                                           "?,?,?,NOW(),?,?,"
////                                           "?,?,?)";

////            // try to insert the special record the the player content to special record table.
////            pstmt.reset(conn->prepareStatement(sql));

////            // set parameter value.
////            pstmt->setInt(1, userId);
////            pstmt->setString(2, account);
////            pstmt->setInt(3, m_pRoomKindInfo->nGameId);
////            pstmt->setInt(4, kindId);
////            pstmt->setInt(5, nRoomId);
////            pstmt->setInt(6, nConfigId);
////            pstmt->setDouble(7, lEnterGem);
////            pstmt->setDouble(8, lEnterScore);
////            pstmt->setString(9, strEnterMachine);
////            pstmt->setString(10, strIp);
////            pstmt->setInt(11, pIserverUserItem->IsAndroidUser());
////            int ret = pstmt->executeUpdate();
//////            if (ret)
////            {
////                // try to query the last user id from the insert query content.
////                pstmt.reset(conn->prepareStatement("SELECT LAST_INSERT_ID()"));
////                res.reset(pstmt->executeQuery()); // reset.
////                if (res->rowsCount() > 0 && res->next())
////                {
////                    scoreInfo.nRecordID = res->getUInt(1);
////                    LOG_DEBUG << " >>> record inout last insert id:" << scoreInfo.nRecordID << ", bisOut:" << bisOut  << ", userid:" << userId;
////                }
////            }
//        }
//        else
//        {
//            auto fuc = boost::bind(&ConnectionPool::ReleaseConnection, ConnectionPool::GetInstance(), _1);
//            shared_ptr<sql::Connection> conn(ConnectionPool::GetInstance()->GetConnection(DB_RECORD), fuc);
//            shared_ptr<sql::PreparedStatement> pstmt(nullptr, [](sql::PreparedStatement *statmt) { if(statmt) { statmt->close(); delete statmt; } });
//            shared_ptr<sql::ResultSet> res(nullptr, [](sql::ResultSet *res) { if(res) { res->close(); delete res; } });


//            date curDate = day_clock::local_day();
//            string curStrMonth = to_iso_string(curDate).substr(0, 6);

//            // update the special user score information for content item now.
//            GS_UserScoreInfo& scoreInfo = pIserverUserItem->GetUserScoreInfo();

//            dword userId    = scoreInfo.nUserId;
//            string account  = pIserverUserItem->getAccount();
//            int   gameId    = m_pRoomKindInfo->nGameId;
//            int   kindId    = m_pRoomKindInfo->nKindId;
//            int   nRoomId   = m_pRoomKindInfo->nServerID;
//            int   nConfigId = m_pRoomKindInfo->nConfigId;
////            dword nEnterTime= scoreInfo.nEnterTime;
//            string strEnterTime = scoreInfo.szEnterTime;
//            SCORE lEnterGem = scoreInfo.nEnterGem;
//            SCORE lEnterScore = scoreInfo.nEnterScore;

//            string strMachine = scoreInfo.szMachine;
//            string strIp    = scoreInfo.szIp;
//            SCORE nLeave_Gem = pIserverUserItem->GetUserGem();
//            SCORE nLeave_Score = pIserverUserItem->GetUserScore();

//            SCORE lWinGem    = scoreInfo.nAddGem;
//            SCORE lWinScore  = scoreInfo.nAddScore;
//            SCORE nTotalGain = scoreInfo.nTotalGain;
//            SCORE nTotalLost = scoreInfo.nTotalLost;
//            SCORE lRevenue  = scoreInfo.nRevenue;

//            SCORE lAgentRevenue = scoreInfo.nAgentrevenue;
//            int nWinCount   = scoreInfo.nWinCount;
//            int nLostCount  = scoreInfo.nLostCount;
//            int nDrawCount  = scoreInfo.nDrawCount;
//            int nFleeCount  = scoreInfo.nFleeCount;
//            dword dwNow = (dword)time(NULL);
//            int nPlayTime   = dwNow - scoreInfo.nEnterTime;
//            int nAndroiduser = pIserverUserItem->IsAndroidUser();


//            string sql = "INSERT INTO record_user_inout_"+curStrMonth+
//                    " (userid, account, gameid, kindid, roomid, configid, enter_time, enter_gem, enter_score,"
//                    " enter_machine, enter_ip, leave_time, leave_score, leave_gem, leave_reason,"
//                    " leave_machine, leave_ip, win_score, win_gem, total_gain, total_lost, revenue, "
//                    " revenue_agent, win_count, lost_count, draw_count, flee_count, play_time, bAndroid) "
//                    " VALUES (?,?,?,?,?,?,?,?,?, "
//                    " ?, ?, NOW(), ?, ?, ?, "
//                    " ?, ?, ?, ?, ?, ?, ?, "
//                    " ?, ?, ?, ?, ?, ?, ?) ";
//            pstmt.reset(conn->prepareStatement(sql));

//            pstmt->setInt(1, userId);
//            pstmt->setString(2, account);
//            pstmt->setInt(3, gameId);
//            pstmt->setInt(4, kindId);
//            pstmt->setInt(5, nRoomId);
//            pstmt->setInt(6, nConfigId);
//            pstmt->setString(7, strEnterTime);
//            pstmt->setDouble(8, lEnterGem);
//            pstmt->setDouble(9, lEnterScore);

//            pstmt->setString(10, strMachine);
//            pstmt->setString(11, strIp);
//            pstmt->setDouble(12, nLeave_Score);
//            pstmt->setDouble(13, nLeave_Gem);
//            pstmt->setInt(14, nReason);

//            pstmt->setString(15, strMachine);
//            pstmt->setString(16, strIp);
//            pstmt->setDouble(17, -lWinScore);
//            pstmt->setDouble(18, -lWinGem);
//            pstmt->setDouble(19, nTotalLost);
//            pstmt->setDouble(20, nTotalGain);
//            pstmt->setDouble(21, lRevenue);

//            pstmt->setDouble(22, lAgentRevenue);
//            pstmt->setInt(23, nWinCount);
//            pstmt->setInt(24, nLostCount);
//            pstmt->setInt(25, nDrawCount);
//            pstmt->setInt(26, nFleeCount);
//            pstmt->setInt(27, nPlayTime);
//            pstmt->setInt(28, nAndroiduser);

//            pstmt->executeUpdate();

////            //score change table
////            GameGlobalFunc::InsertScoreChangeRecord(userId, lEnterScore, lEnterGem, lWinScore, lWinGem, nLeave_Score, nLeave_Gem, SCORE_CHANGE_PLAYGAME, nConfigId);

//            //----------------------today gain_score-----------------
//            //----round gain ----
//            double gain_score = nTotalGain;//-nTotalLost;
//            date cur_date = day_clock::local_day();
//            pstmt.reset(conn->prepareStatement("INSERT INTO db_record.record_today_rank(cur_date,userid,online_time,gain_score) VALUES(?, ?, 0, ?) "
//                                               " ON DUPLICATE KEY UPDATE gain_score=gain_score+?"));
//            pstmt->setDateTime(1, to_iso_extended_string(cur_date));
//            pstmt->setInt(2, userId);
//            pstmt->setDouble(3, gain_score);
//            pstmt->setDouble(4, gain_score);
//            pstmt->executeUpdate();

//            GS_UserScoreInfo scoreinfo = {0};
//            scoreinfo.nUserId    = userId;
//            scoreinfo.nAddScore  = 0;
//            scoreinfo.nRevenue   = 0;
//            scoreinfo.nWinCount  = 0;
//            scoreinfo.nLostCount = 0;
//            scoreinfo.nDrawCount = 0;
//            scoreinfo.nFleeCount = 0;
//            scoreinfo.nPlayTime  = 0;
//            scoreinfo.nEnterTime = 0;
//            pIserverUserItem->SetUserScoreInfo(scoreinfo);
//        }

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
//    boost::uuids::random_generator rgen;
//    boost::uuids::uuid u = rgen();
//    std::stringstream ss;
//    ss << u;

//    string json = "";
//    bool bSuccess = false;
//    std::string uuid = ss.str();
//    int real_size = rec.content.size();
//    if (real_size > 0)
//    {
//        // base64 to encode the special.
//        Landy::Crypto::base64Encode(rec.content, json);

////        std::string strout;
////        Landy::Crypto::base64Decode(json, &strout);

////        int datasize = strout.size();
////        int i = 1;

////        ::Game::Common::GamePlaybackRecord rec2;
////        if(!rec2.ParseFromArray(strout.data(), real_size))
////        {
////            strout.resize(strout.size()-1);
////            printf("fuck to parse error!");
////        }
//    }

//    int gameid = 0;
//    int configid = 0;
//    string room_name;
//    // get the room content.
//    if (m_pRoomKindInfo)
//    {
//        gameid    = m_pRoomKindInfo->nGameId;
//        configid  = m_pRoomKindInfo->nConfigId;
//        room_name = m_pRoomKindInfo->roomKindName;
//        room_name = filterName(room_name);
//    }

//    // write the collect data now.
//    {
//        // try to push the special game_score_storage content to the special sql content item data value.
//        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//        // loop to build the sql to write.
//        for(int i = 0; i < REC_MAXPLAYER; ++i)
//        {
//            // skip if the user id is invalidate value.
//            if(rec.player[i].userid < 20000)
//                continue;
////          string sqlconnect = "insert into db_record.record_gamerec_collect(userid,record_uuid) values(" + to_string(rec.player[i].userid) + ",'"+ uuid +"');";

//            string sqlcollect = "insert into db_record.record_gamerec_collect(gameid,configid,room_name,roundid,userid,bank_userid,"
//                                "user0,user1,user2,user3,user4,winscore0,winscore1,winscore2,winscore3,winscore4,"
//                                "record_uuid) values ("
//                                + to_string(gameid) + ","
//                                + to_string(configid) + ","
//                                + "'" + room_name + "',"
//                                + to_string(rec.rec_roundid) + ","
//                                + to_string(rec.player[i].userid) + ","
//                                + to_string(rec.banker_userid) + ","
//                                + to_string(rec.player[0].account) + ","
//                                + to_string(rec.player[1].account) + ","
//                                + to_string(rec.player[2].account) + ","
//                                + to_string(rec.player[3].account) + ","
//                                + to_string(rec.player[4].account) + ","
//                                + to_string(rec.player[0].changed_score) + ","
//                                + to_string(rec.player[1].changed_score) + ","
//                                + to_string(rec.player[2].changed_score) + ","
//                                + to_string(rec.player[3].changed_score) + ","
//                                + to_string(rec.player[4].changed_score) + ","
//                                + "'" + uuid + "');";
//            if(!redisClient->PushSQL(sqlcollect))
//            {
//                LOG_ERROR << "CTableFrame::SaveReplayRecord redisClient->PushSQL ERROR!";
//                return false;
//            }
//        }
//    }

//    // write the special detail informatino now.
//    {
//        // try to push the special game_score_storage content to the special sql content item data value.
//        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//        string sqldetail = "insert into db_record.record_gamerec_detail(real_size,record_uuid,game_record) values ("
//                + to_string(real_size) + ","
//                + "'" + uuid + "',"
//                + "'" + json + "'"
//                + ") on DUPLICATE key update "
//                + "real_size=" + to_string(real_size) + ","
//                + "game_record='" + json + "';";
//        if(!redisClient->PushSQL(sqldetail))
//        {
//            LOG_ERROR << "CTableFrame::SaveReplayRecord detail redisClient->PushSQL ERROR!";
//            return false;
//        }
//    }
//Cleanup:
    return true;
}

// dump the user list item value.
void CTableFrame::dumpUserList()
{
//    READ_LOCK(m_mutex);
    LOG_DEBUG << " dump user list:";
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

        LOG_DEBUG << "    tableid:[" << m_TableState.nTableID << "],  chairid:[" << i
                  << "], userid:" << userId << ", pIServerUserItem:" << ptr;
    }
}



























/*
// try to send all the table look on data content value item data value on data content value item data.
bool CTableFrame::SendLookOnData(word chairid, byte_t subid, const char* data, dword size, bool isrecord)
{
    LOG_ERROR << "CTableFrame::SendLookOnData NO IMPL yet!";
    return true;
}

bool CTableFrame::SendLookOnData(word chairid, word subid, void_t* buf, word size, bool_t isrecord)
{
    LOG_WARN << "SendLookOnData NO IMPL yet!";
    return true;
}

bool CTableFrame::SendGameMesage(dword chairId, const char* szMessage)
{
    LOG_WARN << "CTableFrame::SendGameMesage(wChairID) NO IMPL yet!";
    return true;
}
*/


#if 0
// set the special user on line information content.
bool CTableFrame::SetUserOnlineInfo(dword userid)
{
    bool bSuccess = false;
    do
    {
        string strKeyName = REDIS_ONLINE_PREFIX + to_string(userid);

        tagOnlineInfo onlineinfo={0};
        onlineinfo.nGameId = m_pRoomKindInfo->nGameId;
        onlineinfo.nKindId = m_pRoomKindInfo->nKindId;
        onlineinfo.nRoomKindId = m_pRoomKindInfo->nRoomKindId;

        redis::RedisValue values;
        values["nGameId"]     = onlineinfo.nGameId;
        values["nKindId"]     = onlineinfo.nKindId;
        values["nRoomKindId"] = onlineinfo.nRoomKindId;

        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
        bSuccess = redisClient->hmset(strKeyName, values);
    }   while (0);
//Cleanup:
    return (bSuccess);
}
#else
// set the special user on line information content.

#endif


//刷新个人累计输赢库存控制参数
void CTableFrame::RefreshKcControlParameter(shared_ptr<CServerUserItem> pIServerUserItem,int64_t addScore)
{
    if (m_pGameRoomInfo->gameId == (int64_t)eGameKindId::zjh)
    {
        int64_t current = pIServerUserItem->GetBlackRoomlistInfo().current;
        int64_t roomWin = pIServerUserItem->GetBlackRoomlistInfo().roomWin;
        string strKeyName = REDIS_BLACKROOM + to_string(pIServerUserItem->GetUserId()) + "_" + to_string(m_pGameRoomInfo->roomId);
        int64_t userWinScore = pIServerUserItem->GetBlackRoomlistInfo().roomWin + addScore;
        if (userWinScore < 0) //本局累计为负
        {
            if (pIServerUserItem->GetBlackRoomlistInfo().roomWin > 0)
            {
                pIServerUserItem->GetBlackRoomlistInfo().roomWin = 0;
                pIServerUserItem->GetBlackRoomlistInfo().roomWin += userWinScore*(100-m_kRatiocReduction)/100.0;
            }
            else 
            {
                if (addScore < 0)
                {
                    pIServerUserItem->GetBlackRoomlistInfo().roomWin += addScore*(100-m_kRatiocReduction)/100.0;
                }
                else
                {
                    pIServerUserItem->GetBlackRoomlistInfo().roomWin += addScore;
                }
            }
            pIServerUserItem->GetBlackRoomlistInfo().current = 0;
        }
        else
        {
            if (pIServerUserItem->GetBlackRoomlistInfo().roomWin > 0)
            {
                pIServerUserItem->GetBlackRoomlistInfo().roomWin += addScore;
                pIServerUserItem->GetBlackRoomlistInfo().current += addScore;
            }
            else if (addScore > 0)
            {
                pIServerUserItem->GetBlackRoomlistInfo().roomWin = 0;
                pIServerUserItem->GetBlackRoomlistInfo().current = 0;
                pIServerUserItem->GetBlackRoomlistInfo().roomWin += userWinScore;
            }            
        }
        
        if (pIServerUserItem->GetBlackRoomlistInfo().current < 0 || pIServerUserItem->GetBlackRoomlistInfo().roomWin <= 0)
        {
            pIServerUserItem->GetBlackRoomlistInfo().current = 0;
        }
        int curTimes = pIServerUserItem->GetBlackRoomlistInfo().roomWin/m_pGameRoomInfo->floorScore;
        if(curTimes<0) curTimes = 0;
        int newLv = GetCurrentControlLv(curTimes);
        int curLv = GetCurrentControlLv(pIServerUserItem->GetBlackRoomlistInfo().controlTimes);
        if (newLv>curLv)
        {
            // 当前控制结束,刷新控制等级
            pIServerUserItem->GetBlackRoomlistInfo().controlTimes = curTimes;
            float f = 1.0f*(GlobalFunc::RandomInt64(0, 50)+50)/100;
            pIServerUserItem->GetBlackRoomlistInfo().current = pIServerUserItem->GetBlackRoomlistInfo().roomWin*f;
        }
        else if (newLv >= 0 && pIServerUserItem->GetBlackRoomlistInfo().current == 0 && pIServerUserItem->GetBlackRoomlistInfo().roomWin > 0)
        {
            // 上一等级控制结束,刷新新一局的参数
            pIServerUserItem->GetBlackRoomlistInfo().controlTimes = curTimes;
            float f = 1.0f*(GlobalFunc::RandomInt64(0, 50)+50)/100;
            pIServerUserItem->GetBlackRoomlistInfo().current = pIServerUserItem->GetBlackRoomlistInfo().roomWin*f;
        }
        else if ((current==0 && roomWin==0 && pIServerUserItem->GetBlackRoomlistInfo().roomWin > 0) || (pIServerUserItem->GetBlackRoomlistInfo().current == 0 && pIServerUserItem->GetBlackRoomlistInfo().roomWin == 0))
        {
            pIServerUserItem->GetBlackRoomlistInfo().controlTimes = curTimes;
            float f = 1.0f*(GlobalFunc::RandomInt64(0, 50)+50)/100;
            pIServerUserItem->GetBlackRoomlistInfo().current = pIServerUserItem->GetBlackRoomlistInfo().roomWin*f;
        }
        if (pIServerUserItem->GetBlackRoomlistInfo().current == 0 && pIServerUserItem->GetBlackRoomlistInfo().roomWin <= 0)
            pIServerUserItem->GetBlackRoomlistInfo().controlTimes = curTimes;

        redis::RedisValue redisValue;
        redisValue["current"] = pIServerUserItem->GetBlackRoomlistInfo().current;
        redisValue["controlTimes"] = pIServerUserItem->GetBlackRoomlistInfo().controlTimes;
        redisValue["roomWin"] = pIServerUserItem->GetBlackRoomlistInfo().roomWin;
        redisValue["status"] = pIServerUserItem->GetBlackRoomlistInfo().status;
        int timeout = m_kcTimeoutHour*3600; //小时
        bool ret = REDISCLIENT.hmset(strKeyName, redisValue, timeout);
        if (ret)
        {
            redis::RedisValue values; 
            string fields[4] = {"current","controlTimes","roomWin","status"};
            bool res = REDISCLIENT.hmget(strKeyName,fields,4,values);
            if (res && !values.empty())
            {
                if (values["status"].asInt() == 1)
                {
                    if (values["current"].asInt() <= 0)
                    {
                        redis::RedisValue redisValue;
                        redisValue["status"] = (int32_t)0;
                        REDISCLIENT.hmset(strKeyName, redisValue);
                        pIServerUserItem->GetBlackRoomlistInfo().status = 0;
                    }
                }
                else
                {
                    pIServerUserItem->GetBlackRoomlistInfo().status = 0;
                }
            }
        }
    }
}
//获取当前的控制等级
int CTableFrame::GetCurrentControlLv(int curTime)
{
    int conIndex = 0;   
    for (int i = 5; i >= 0; --i)
    {
        if (curTime>=m_kcBaseTimes[i])
        {
            conIndex = i;
            break;
        }
    }
    return conIndex;
}
//设置个人库存控制等级
void CTableFrame::setkcBaseTimes(int kcBaseTimes[],double timeout,double reduction)
{
    memcpy(m_kcBaseTimes,kcBaseTimes,sizeof(m_kcBaseTimes));
    m_kcTimeoutHour = timeout;
    m_kRatiocReduction = reduction;
}

/*
bool CTableFrame::WriteTableScore(tagScoreInfo* pScoreInfo, SCORE wScoreCount)
{
    LOG_WARN << "CTableFrame::WriteTableScore NO IMPL yet!";
    do
    {
        if (!pScoreInfo) break;
        if (wScoreCount!=GAME_PLAYER) break;
        for (int wChairID=0;wChairID<wScoreCount;wChairID++)
        {
            WriteUserScore(wChairID, pScoreInfo[wChairID]);
        }
    } while (0);
//Cleanup:
    return (true);
}
*/


//// try to do on user left when user client try to do on user left when user client try to do on user
//void CTableFrame::OnUserLeft(IServerUserItem *pIBaseUserItem, bool bSendToSelf, bool bForceLeave)
//{
//    do
//    {
//        // try to convert the special base user data item to the server user data item now.
//        CServerUserItem* pIServerUserItem = static_cast<CServerUserItem*>(pIBaseUserItem);
//        if (pIServerUserItem == NULL)
//        {
//            break;
//        }

//        // try to get the special user id item value.
//        dword userid = pIServerUserItem->GetUserID();
//        bool isAndroid = pIServerUserItem->IsAndroidUser();
//        LOG_DEBUG << " >>> OnUserLeft cbGameStatus:" << m_cbGameStatus
//                  << ", isLeaveAnyTime:" << m_pRoomKindInfo->bisLeaveAnyTime
//                  << ", userid:" << userid << ", isAndroid:" << isAndroid;

//        // modify by James 180806-dynamic join game can leave on any time can leave on any content.
//        bool bGameStarted = ((m_cbGameStatus>=GAME_STATUS_START) && (m_cbGameStatus<GAME_STATUS_END));
//        if ((bGameStarted) && !m_pRoomKindInfo->bisLeaveAnyTime)
//        {
//            LOG_INFO << " >>> CTableFrame::OnUserLeft trustship userid:" << pIServerUserItem->GetUserID();
//            pIServerUserItem->SetUserStatus(sOffline);
//            pIServerUserItem->SetTrustship(true);
//            BroadcastUserStatus(pIServerUserItem,bSendToSelf);
////            DeleteUserToProxy(pIServerUserItem);
//            pIServerUserItem->ResetUserConnect();
//            // pIServerUserItem->SetClientNetAddr(0);
//            pIServerUserItem->ResetClientNetAddr();
//            m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserID(), false);
//        }
//        else
//        {
//            m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserID(), false);
//            OnUserStandup(pIServerUserItem, true, bSendToSelf);
//            DelUserOnlineInfo(pIServerUserItem->GetUserID());
////            DeleteUserToProxy(pIServerUserItem);

//            // try to delete the special user item from the server user manager content.
//            CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);

//            // check is ready to start game item content value item now.
//            if ((m_pRoomKindInfo->bisQipai) && (CheckGameStart()))
//            {
//                if (GAME_STATUS_FREE == m_cbGameStatus)
//                {
//                    GameRoundStartDeploy();
//                }

//                StartGame();
//            }
//        }

//    } while (0);
////Cleanup:
//    return;
//}


//void CTableFrame::OnUserOffline(IServerUserItem *pIBaseUserItem, bool bLeaveGs)
//{
//    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

//    uint64_t userId = pIBaseUserItem->GetUserId();
//    LOG_DEBUG << " >>> OnUserOffline userId:" << userId;

//    do
//    {
//        // convert the special base server user item pointer to the server user item now.
//        CServerUserItem* pIServerUserItem = static_cast<CServerUserItem*>(pIBaseUserItem);
//        if(!pIServerUserItem)
//        {
//            break;
//        }

//        // skip already offline user content user status value.
//        if(sOffline == pIServerUserItem->GetUserStatus())
//        {
//            LOG_DEBUG << " >>> skip already offline user, userid:" << pIServerUserItem->GetUserId();
//            break;
//        }

////        bool bGameStarted = ((m_cbGameStatus>=GAME_STATUS_START) && (m_cbGameStatus<GAME_STATUS_END));
////        if((bGameStarted) && (m_pRoomKindInfo->bisLeaveAnyTime == false))
////        {
//            pIServerUserItem->SetUserStatus(sOffline);
//            pIServerUserItem->SetTrustship(true);
//            BroadcastUserStatus(pIServerUserItem, true);
//            DeleteUserToProxy(pIServerUserItem);
////            pIServerUserItem->ResetUserConnect();
//            // pIServerUserItem->SetClientNetAddr(0);
////            pIServerUserItem->ResetClientNetAddr();
//            m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserId(), false);
////        }
////        else
////        {
////            // call the special user left content message for later user item.
////            m_pTableFrameSink->OnUserLeft(pIServerUserItem->GetUserId(), false);
////            OnUserStandup(pIServerUserItem, true, false);
////            // set the expired state.
////            bool bonlyExipred = true;
////            if(bLeaveGs)
////                bonlyExipred = false;
////            DelUserOnlineInfo(pIServerUserItem->GetUserId(), bonlyExipred);
////            DeleteUserToProxy(pIServerUserItem);

////            CServerUserManager::get_mutable_instance().DeleteUserItem(pIServerUserItem);

//////            if ((m_pRoomKindInfo->bisQipai) && (CheckGameStart()))
//////            {
//////                if (GAME_STATUS_FREE == m_cbGameStatus)
//////                {
//////                    GameRoundStartDeploy();
//////                }

//////                StartGame();
//////            }
////        }
//    }while(0);
//    return;
//}


//// clear the special table user item.
//void CTableFrame::ClearTableUser(dword wChairID, bool bSendState, bool bSendToSelf, byte_t cbSendErrorCode)
//{
//    LOG_DEBUG << " >>> ClearTableUser tableid:" << m_TableState.nTableID << ", chairid:" << wChairID;

//    int kickType = KICK_GS;
//    if((INVALID_CARD == wChairID) && (ERROR_ENTERROOM_SERVERSTOP == cbSendErrorCode))
//    {
//        kickType |= (KICK_HALL);
//    }

//    // check the table id content.
//    if(wChairID != INVALID_CHAIR)
//    {
//        CServerUserItem* pUserItem = NULL;
//        {
//            MutexLockGuard lock(m_mutexLock);
//            pUserItem = m_UserList[wChairID];
//        }

//		// check if the user item exist.
//        if (!pUserItem)
//            return;

//        // try to get the special user id now.
//        dword userId = pUserItem->GetUserID();
//        // check the error code.
//        if(cbSendErrorCode != 0)
//        {
//            int mainid = Game::Common::MSGGS_MAIN_FRAME;
//            int subid  = GameServer::Message::SUB_S2C_ENTER_ROOM_FAIL;
//            GameServer::Message::MSG_SC_GameErrorCode errCode;
//            Game::Common::Header* header = errCode.mutable_header();
//            header->set_sign(HEADER_SIGN);
//            header->set_crc(0);
//            header->set_mainid(mainid);
//            header->set_subid(subid);
//            errCode.set_ierrorcode(cbSendErrorCode);

//            string content = errCode.SerializeAsString();
//            SendPackedData(pUserItem, mainid, subid, content.data(), content.size());
//        }

//        // try to call the special on user standup action.
//        OnUserStandup(pUserItem, bSendState, bSendToSelf);

//        if(!pUserItem->IsAndroidUser())
//        {
//            DelUserOnlineInfo(pUserItem->GetUserID());
//            DeleteUserToProxy(pUserItem, kickType|KICK_CLOSEONLY);
//            CServerUserManager::get_mutable_instance().DeleteUserItem(pUserItem);
//        }else
//        {
//            CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//        }

//        if((GetPlayerCount() == 0) && (!m_pRoomKindInfo->bisDynamicJoin))
//        {
//            // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//            if(m_pRoomKindInfo->bisQipai)
//            {
//                ListTableFrame& listUseTable = CGameTableManager::get_mutable_instance().GetUsedTableFrame();
//                if (listUseTable.size() > 1)
//                {
//                    CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//                }
//            }
//            else
//            {
//                CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//            }
//        }
////        log_userlist("ClearTableUser wChairID:[%d], userid:[%d].", wChairID, userid);
//        return;
//    }

//    // wChairID == INVALID_CHAIR

//    // loop to kick all offline user.
//    word wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
//    for (dword idx = 0; idx < wMaxPlayer; ++idx)
//    {
//        CServerUserItem* pUserItem = NULL;
//        {
//            MutexLockGuard lock(m_mutexLock);
//            pUserItem = m_UserList[idx];
//        }

//        // check if user item now.
//        if (!pUserItem)
//        {
//            continue;
//        }

//        // given benefit reward before kick.
//        /*if (!pUserItem->IsAndroid())
//        {
//            if ((pUserItem->GetUserScore() < CServerCfg::m_BenefitRewardCfg.dwLessScore) && CServerCfg::m_nBenefitStatus != 0)
//            {
//                GetBenefitReward(pUserItem);
//            }
//        }  */

//		dword userid = pUserItem->GetUserID();
//        byte_t cbUserStatus = pUserItem->GetUserStatus();
//        SCORE lUserScore   = pUserItem->GetUserScore();
//        if ((cbUserStatus == sOffline) ||
//            (m_pRoomKindInfo->nServerStatus == SERVER_STOPPED) ||
//            ((!m_pRoomKindInfo->bisDynamicJoin) && (lUserScore < m_pRoomKindInfo->nEnterMinScore)))
//        {
//            if (lUserScore <= m_pRoomKindInfo->nEnterMinScore)
//            {
//                int mainid = Game::Common::MSGGS_MAIN_FRAME;
//                int subid  = GameServer::Message::SUB_S2C_ENTER_ROOM_FAIL;
//                GameServer::Message::MSG_SC_GameErrorCode errCode;
//                Game::Common::Header* header = errCode.mutable_header();
////                header->set_sign(HEADER_SIGN);
//                header->set_crc(0);
//                header->set_mainid(mainid);
//                header->set_subid(subid);
//                errCode.set_ierrorcode(ERROR_ENTERROOM_SCORENOENOUGH);

//                string content = errCode.SerializeAsString();
//                SendPackedData(pUserItem, mainid, subid, content.data(), content.size());
//            }

//            // try to check the sepcial server status value item.
//            if (m_pRoomKindInfo->nServerStatus == SERVER_STOPPED)
//            {
//                int mainid = Game::Common::MSGGS_MAIN_FRAME;
//                int subid  = GameServer::Message::SUB_S2C_ENTER_ROOM_FAIL;
//                GameServer::Message::MSG_SC_GameErrorCode errCode;
//                Game::Common::Header* header = errCode.mutable_header();
//				header->set_sign(HEADER_SIGN);
//                header->set_crc(0);
//                header->set_mainid(mainid);
//                header->set_subid(subid);
//                errCode.set_ierrorcode(ERROR_ENTERROOM_SERVERSTOP);

//                string content = errCode.SerializeAsString();
//                SendPackedData(pUserItem, mainid, subid, content.data(), content.size());
//            }

//            // set the special player left the table now.
//			m_pTableFrameSink->OnUserLeft(userid, false);
//            OnUserStandup(pUserItem, bSendState, true);
//            if (!pUserItem->IsAndroidUser())
//            {
//                DelUserOnlineInfo(pUserItem->GetUserID());
//                DeleteUserToProxy(pUserItem, kickType);
//                CServerUserManager::get_mutable_instance().DeleteUserItem(pUserItem);
//            }
//            else
//            {
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }
//        else
//        {
//            pUserItem->SetUserStatus(sSit);
//            LOG_ERROR << "Sit3";
//        }
//    }

//    if(m_pRoomKindInfo->nServerStatus == SERVER_STOPPED && GetPlayerCount() == 0)
//    {
//        LOG_WARN << "free table, table id: " << GetTableId();
//        CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//        return;
//    }

//    // on game end to store android played round item now.
//    wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
//    for (dword i = 0; i < wMaxPlayer; ++i)
//    {
//        CServerUserItem* pUserItem = NULL;
//        {
//            MutexLockGuard lock(m_mutexLock);
//            pUserItem = m_UserList[i];
//        }

//        // check the user item now.
//        if(!pUserItem)
//        {
//            continue;
//        }

//        // check if the specail android
//        if (pUserItem->IsAndroidUser())
//        {
//            pUserItem->AddPlayCount();

//            IsLetAndroidOffline(pUserItem);

//            // qipai, one time only let one android left.
//            // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//            if (m_pRoomKindInfo->bisQipai)
//            {
//                break;
//            }
//        }
//    }

//    // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//    if (m_pRoomKindInfo->bisKeepAndroidin)
//    {
//        dword wRealPlayer = GetPlayerCount(false);
//        dword wMaxAnroidCount = CAndroidUserManager::get_mutable_instance().GetActiveAndroidCount();
//        dword wLeftAndroidCount = 0;

//        if (wMaxAnroidCount != 0)
//        {
//            if (wRealPlayer == 1 || wRealPlayer == 0)
//            {
//                wLeftAndroidCount = wMaxAnroidCount;
//            }
//            else if (wRealPlayer == 2)
//            {
//                wLeftAndroidCount = 1;
//            }
//            else if (wRealPlayer == 3)
//            {
//                wLeftAndroidCount = rand() % 2 ? 1 : 0;
//            }
//            else
//            {
//                wLeftAndroidCount = 0;
//            }
//        }

//        // loop to check the special content.
//        for (dword i = 0; i < wMaxPlayer; ++i)
//        {
//            CServerUserItem* pUserItem = NULL;
//            {
//                MutexLockGuard lock(m_mutexLock);
//                pUserItem = m_UserList[i];
//            }

//            // check the user item.
//            if (pUserItem == NULL)
//            {timer
//                continue;
//            }

//            // try to check all the android content value.
//            dword dwAndroidCount = GetAndroidPlayerCount();
//            if (dwAndroidCount <= wLeftAndroidCount)
//            {
//                break;
//            }

//            // check the android user item.
//            if (pUserItem->IsAndroidUser())
//            {
//                LOG_WARN << "decrease android count.";
//                OnUserStandup(pUserItem, true);
//                //DelUserOnlineInfo(pUserItem->GetUserID());
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }

//        // free all table frame now.
//        if (GetPlayerCount() == 0)
//        {
//            ListTableFrame& listUseTable = CGameTableManager::get_mutable_instance().GetUsedTableFrame();
//            if (listUseTable.size() > 1)
//            {
//                LOG_WARN << "free table, table id: " << GetTableID();
//                CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//            }
//        }timer
//    }else if ((!m_pRoomKindInfo->bisDynamicJoin) && (GetPlayerCount(false) == 0))
//    {
//        // kick all android if no real player.
//        for (dword i = 0; i < wMaxPlayer; ++i)
//        {
//            CServerUserItem* pUserItem = NULL;
//            {
//                MutexLockGuard lock(m_mutexLock);
//                pUserItem = m_UserList[i];
//            }

//            // check the user item now.
//            if(pUserItem == NULL)
//            {
//                continue;
//            }

//            if(pUserItem->IsAndroidUser())
//            {
//                OnUserStandup(pUserItem, true);
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }

//        // free current table item if kick all player on current able.
//        CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//    }
//}


//// try to write the special write game storage content item.
//int CTableFrame::WriteGameStorage(tagStorageInfo storageInfo)
//{
//	int status = -1;
////	static tagStorageInfo lstStorage = {0};
//	if ((storageInfo.lEndStorage != storageInfo.lEndStorage) ||
//		(storageInfo.lLowlimit != storageInfo.lLowlimit)     ||
//		(storageInfo.lReduceUnit != storageInfo.lReduceUnit) ||
//		(storageInfo.lUplimit != sttimerorageInfo.lUplimit))
//	{
////		time_t tt;
////		time(&tt);
////		struct tm* ptm = localtime(&tt);
////        char szDate[32] = { 0 };
////		  snprintf(szDate,sizeof(szDate),"%04d-%02d-%2d",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday);
//        char sql[1024] = { 0 };
//        snprintf(sql,sizeof(sql),"INSERT INTO game_score_storage(end_storage, lower_limit, up_limit, reduce_unit) VALUES(%0.02lf, %0.02lf, %0.02lf, %0.02lf)"
//                                 " ON DUPLICATE KEY UPDATE end_storage=%0.02lf, lower_limit=%0.02lf, up_limit=%0.02lf, reduce_unit=%0.02lf",
//			storageInfo.lEndStorage, storageInfo.lLowlimit, storageInfo.lUplimit, storageInfo.lReduceUnit,
//			storageInfo.lEndStorage, storageInfo.lLowlimit, storageInfo.lUplimit, storageInfo.lReduceUnit);
//		// try to push the special game_score_storage content to the special sql content item data value.
//		auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//		shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//        if(redisClient->PushSQL(sql))
//        {
//			status = 0;
//		}
//	}
////Cleanup:
//    return (status);
//}


//    // wChairID == INVALID_CHAIR

//    // loop to kick all offline user.
//    word wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
//    for (dword idx = 0; idx < wMaxPlayer; ++idx)
//    {
//        CServerUserItem* pUserItem = NULL;
//        {
//            MutexLockGuard lock(m_mutexLock);
//            pUserItem = m_UserList[idx];
//        }

//        // check if user item now.
//        if (!pUserItem)
//        {
//            continue;
//        }

//        // given benefit reward before kick.
//        /*if (!pUserItem->IsAndroid())
//        {
//            if ((pUserItem->GetUserScore() < CServerCfg::m_BenefitRewardCfg.dwLessScore) && CServerCfg::m_nBenefitStatus != 0)
//            {
//                GetBenefitReward(pUserItem);
//            }
//        }  */

//        dword userid = pUserItem->GetUserID();
//        byte_t cbUserStatus = pUserItem->GetUserStatus();
//        SCORE lUserScore   = pUserItem->GetUserScore();
//        if ((cbUserStatus == sOffline) ||
//            (m_pRoomKindInfo->nServerStatus == SERVER_STOPPED) ||
//            ((!m_pRoomKindInfo->bisDynamicJoin) && (lUserScore < m_pRoomKindInfo->nEnterMinScore)))
//        {
//            if (lUserScore < m_pRoomKindInfo->nEnterMinScore)
//            {
//                int mainid = Game::Common::MSGGS_MAIN_FRAME;
//                int subid  = GameServer::Message::SUB_S2C_ENTER_ROOM_FAIL;
//                GameServer::Message::MSG_SC_GameErrorCode errCode;
//                Game::Common::Header* header = errCode.mutable_header();
////                header->set_sign(HEADER_SIGN);
//                header->set_crc(0);
//                header->set_mainid(mainid);
//                header->set_subid(subid);
//                errCode.set_ierrorcode(ERROR_ENTERROOM_SCORENOENOUGH);

//                string content = errCode.SerializeAsString();
//                SendPackedData(pUserItem, mainid, subid, content.data(), content.size());
//            }

//            // try to check the sepcial server status value item.
//            if (m_pRoomKindInfo->nServerStatus == SERVER_STOPPED)
//            {
//                int mainid = Game::Common::MSGGS_MAIN_FRAME;
//                int subid  = GameServer::Message::SUB_S2C_ENTER_ROOM_FAIL;
//                GameServer::Message::MSG_SC_GameErrorCode errCode;
//                Game::Common::Header* header = errCode.mutable_header();
//                header->set_sign(HEADER_SIGN);
//                header->set_crc(0);
//                header->set_mainid(mainid);
//                header->set_subid(subid);
//                errCode.set_ierrorcode(ERROR_ENTERROOM_SERVERSTOP);

//                string content = errCode.SerializeAsString();
//                SendPackedData(pUserItem, mainid, subid, content.data(), content.size());
//            }

//            // set the special player left the table now.
////            m_pTableFrameSink->OnUserLeft(userid, false);
//            OnUserStandup(pUserItem, bSendState, true);
//            if (!pUserItem->IsAndroidUser())
//            {
////                DelUserOnlineInfo(pUserItem->GetUserID());
////                DeleteUserToProxy(pUserItem, kickType);
////                CServerUserManager::get_mutable_instance().DeleteUserItem(pUserItem);

//                DelUserOnlineInfo(pUserItem->GetUserID());
////                int expected = 1;
////                if(pUserItem->m_LeftStatus.compare_exchange_strong(expected, 1))
//                    DeleteUserToProxy(pUserItem, kickType|KICK_CLOSEONLY);
//                pUserItem->ResetUserConnect();
//                pUserItem->ResetClientNetAddr();
//                CServerUserManager::get_mutable_instance().DeleteUserItem(pUserItem);
//            }
//            else
//            {
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }
//        else
//        {
//            pUserItem->SetUserStatus(sSit);
//            LOG_ERROR << "Sit3";
//        }
//    }

//    if(m_pRoomKindInfo->nServerStatus == SERVER_STOPPED && GetPlayerCount() == 0)
//    {
//        LOG_WARN << "free table, table id: " << GetTableID();
//        CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//        return;
//    }

//    // on game end to store android played round item now.
//    wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;
//    for (dword i = 0; i < wMaxPlayer; ++i)
//    {
//        CServerUserItem* pUserItem = NULL;
//        {
//            MutexLockGuard lock(m_mutexLock);
//            pUserItem = m_UserList[i];
//        }

//        // check the user item now.
//        if(!pUserItem)
//        {
//            continue;
//        }

//        // check if the specail android
//        if (pUserItem->IsAndroidUser())
//        {
//            pUserItem->AddPlayCount();

//            IsLetAndroidOffline(pUserItem);

//            // qipai, one time only let one android left.
//            // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//            if (m_pRoomKindInfo->bisQipai)
//            {
//                break;
//            }
//        }
//    }

//    // if (m_pRoomKindInfo->wGameID == GAME_NN || m_pRoomKindInfo->wGameID == GAME_ZJH)
//    if (m_pRoomKindInfo->bisKeepAndroidin)
//    {
//        dword wRealPlayer = GetPlayerCount(false);
//        dword wMaxAnroidCount = CAndroidUserManager::get_mutable_instance().GetActiveAndroidCount();
//        dword wLeftAndroidCount = 0;

//        if (wMaxAnroidCount != 0)
//        {
//            if (wRealPlayer == 1 || wRealPlayer == 0)
//            {
//                wLeftAndroidCount = wMaxAnroidCount;
//            }
//            else if (wRealPlayer == 2)
//            {
//                wLeftAndroidCount = 1;
//            }
//            else if (wRealPlayer == 3)
//            {
//                wLeftAndroidCount = rand() % 2 ? 1 : 0;
//            }
//            else
//            {
//                wLeftAndroidCount = 0;
//            }
//        }

//        // loop to check the special content.
//        for (dword i = 0; i < wMaxPlayer; ++i)
//        {
//            CServerUserItem* pUserItem = NULL;
//            {
//                MutexLockGuard lock(m_mutexLock);
//                pUserItem = m_UserList[i];
//            }

//            // check the user item.
//            if (pUserItem == NULL)
//            {
//                continue;
//            }

//            // try to check all the android content value.
//            dword dwAndroidCount = GetAndroidPlayerCount();
//            if (dwAndroidCount <= wLeftAndroidCount)
//            {
//                break;
//            }

//            // check the android user item.
//            if (pUserItem->IsAndroidUser())
//            {
//                LOG_WARN << "decrease android count.";
//                OnUserStandup(pUserItem, true);
//                //DelUserOnlineInfo(pUserItem->GetUserID());
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }

//        // free all table frame now.
//        if (GetPlayerCount() == 0)
//        {
//            ListTableFrame listUseTable = CGameTableManager::get_mutable_instance().GetUsedTableFrame();
//            if (listUseTable.size() > 1)
//            {
//                LOG_WARN << "free table, table id: " << GetTableID();
//                CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//            }
//        }
//    }else if ((!m_pRoomKindInfo->bisDynamicJoin) && (GetPlayerCount(false) == 0))
//    {
//        // kick all android if no real player.
//        for (dword i = 0; i < wMaxPlayer; ++i)
//        {
//            CServerUserItem* pUserItem = NULL;
//            {
//                MutexLockGuard lock(m_mutexLock);
//                pUserItem = m_UserList[i];
//            }

//            // check the user item now.
//            if(pUserItem == NULL)
//            {
//                continue;
//            }

//            if(pUserItem->IsAndroidUser())
//            {
//                OnUserStandup(pUserItem, true);
//                CAndroidUserManager::get_mutable_instance().DeleteAndroidUserItem(pUserItem);
//            }
//        }

//        // free current table item if kick all player on current able.
//        CGameTableManager::get_mutable_instance().FreeNormalTable(this);
//    }
//}


//// set the special game timer.
//bool CTableFrame::SetGameTimer(dword timerid, dword dwElapse, dword nRepeat, dword dwParam)
//{
//    int nElapse = dwElapse;
//    // update the elapse value.
//    nElapse = nElapse/m_nDived;
//    // min time is 1/0.1 second.
//    if(nElapse<=0)
//        nElapse=1;

//    MutexLockGuard lock(m_TimeTickMutexLock);

//    bool bExist = false;
//    DEQTIMER::iterator iter;
//    for(iter=m_deqTimer.begin(); iter!=m_deqTimer.end(); ++iter)
//    {
//        // try to update the timer.
//        tagTimerInfo& timer = *iter;
//        if(timer.timerid == timerid)
//        {
//            LOG_DEBUG << " >>> SetGameTimer replace timerid:" << timerid << ", elapse:" << dwElapse << ", repeat:" << nRepeat << ", seqid:" << (g_timerSeqid+1) << ", tableid:" << m_TableState.nTableID;

//            timer.dwRepeat = nRepeat;
//            timer.dwElapse = nElapse;
//            timer.dwInitElapse = nElapse;
//            timer.nSeqid = g_timerSeqid++;
//            timer.dwParam  = dwParam;
//            bExist = true;
//            break;
//        }
//    }

//    // exist now.
//    if(!bExist)
//    {
//        LOG_ERROR << " >>> SetGameTimer add timerid:" << timerid << ", elapse:" << dwElapse << ", repeat:" << nRepeat << ", seqid:" << (g_timerSeqid+1) << ", tableid:" << m_TableState.nTableID;

//        tagTimerInfo timer = {0};
//        timer.timerid  = timerid;
//        timer.dwElapse = nElapse;
//        timer.dwRepeat = nRepeat;
//        timer.dwInitElapse = nElapse;
//        timer.nSeqid = g_timerSeqid++;
//        timer.dwParam  = dwParam;
//        m_deqTimer.push_back(timer);
//    }
////Cleanup:
//    return true;
//}

//// one the timer has been ticked.
//void CTableFrame::OnTimerTick(float delta)
//{
//	int tick = 0;
//    static float fDeltasum = 0;
//    fDeltasum += delta;
//    if (fDeltasum > 1.0f)
//    {
//		tick = (int)fDeltasum;
//        fDeltasum -= tick;
//    }

//    DEQTIMER tmpTimer;
//    DEQTIMER::iterator iter;

//    {
//        MutexLockGuard lock(m_TimeTickMutexLock);

//        for (iter=m_deqTimer.begin();iter!=m_deqTimer.end();)
//        {
//            tagTimerInfo& timer = *iter;
//            timer.dwElapse -= tick; // elapse.
//            if (timer.dwElapse <= 0)
//            {
//                // check clean timer.
//                bool bClean = false;
//                if (timer.dwRepeat<=0)
//                {
//                    bClean = true;
//                }else if (timer.dwRepeat != REPEAT_INFINITE)
//                {
//                    // sub repeat time.
//                    timer.dwRepeat--;
//                    if (timer.dwRepeat<=0)
//                    {
//                        bClean = true;
//                    }
//                }

//                // infinite mode, reset the elapse.
//                timer.dwElapse = timer.dwInitElapse;
//                // add the timer to timer.
//                tmpTimer.push_back(timer);
//                // Cleanup.
//                if (bClean)
//                {
//                    // try to notify the timer now.
//                    iter = m_deqTimer.erase(iter);
//                }else
//                {
//                    // next.
//                    iter++;
//                }
//            }
//            else
//            {
//                iter++;
//            }
//        }
//    }

//    // add timer2 to skip, when ontimer set time, the loop will dead.
//    {
//        DEQTIMER::iterator iter;
//        for(iter=tmpTimer.begin();iter!=tmpTimer.end();iter++)
//        {
//            tagTimerInfo& timer2 = *iter;
//            // dump no repeat timer item log content.
//            if(REPEAT_INFINITE != timer2.dwRepeat)
//            {
//                // log the special log content for the special timer has been arrived content value data item value now.
//                LOG_ERROR << " >>> OnTimerArrived tableid: " << m_TableState.nTableID << ", timerid:" << timer2.timerid
//                          << ", elapse:" << timer2.dwInitElapse << ", repeat:" << timer2.dwRepeat << ", tick:"
//                          << tick << ", seqid:" << timer2.nSeqid;
//            }

//            // get the special tick value item now.
//            long tm_start = Utility::gettimetick();
//            // call table frame sink.
//            if(m_pTableFrameSink)
//            {
//                m_pTableFrameSink->OnTimerMessage(timer2.timerid, timer2.dwParam);
//            }

//            // try to get the special sub value data now.
//            long sub = Utility::gettimetick() - tm_start;
//            if(sub > TIMEOUT_PERFRAME)
//            {
//                LOG_ERROR << " >>> processRequest timeout m_pTableFrameSink->OnTimerMessage timeid:" << timer2.timerid << ", time used:" << sub;
//            }
//        }
//    }
//}

//// try to kill the special timer content value.
//bool CTableFrame::KillGameTimer(dword timerid)
//{
//    bool bExist = false;
//    DEQTIMER::iterator iter;

//    MutexLockGuard lock(m_TimeTickMutexLock);

//    for (iter=m_deqTimer.begin();iter!=m_deqTimer.end();iter++)
//    {
//        tagTimerInfo& timer = *iter;
//        if (timer.timerid == timerid)
//        {
//            LOG_DEBUG << " >>> killTimer timerid:" << timer.timerid << ", elapse:" << timer.dwInitElapse << ", repeat:" << timer.dwRepeat << ", seqid:" << timer.nSeqid;

//            m_deqTimer.erase(iter);
//            bExist = true;
//            break;
//        }
//    }
////Cleanup:
//    return (bExist);
//}

//// check if the timer has been exist item now.
//bool CTableFrame::IsTimerExist(dword timerid)
//{
//    bool bExist = false;
//    DEQTIMER::iterator iter;
//    for (iter=m_deqTimer.begin();iter!=m_deqTimer.end();iter++)
//    {
//        tagTimerInfo& timer = *iter;
//        if (timer.timerid == timerid)
//        {
//            bExist = true;
//            break;
//        }
//    }
////Cleanup:
//    return (bExist);
//}


//void CTableFrame::log_userlist(const char* fmt,...)
//{
//    va_list va;
//    char sztime[1024]={0};
//    char buf[1024]={0};
//    va_start(va,fmt);
//    vsnprintf(buf,sizeof(buf),fmt,va);
//    va_end(va);

//    time_t tt;
//    time( &tt);
//    struct tm* tm = localtime(&tt);
//    snprintf(sztime,sizeof(sztime),"%02d:%02d:%02d : ",tm->tm_mday,tm->tm_hour,tm->tm_min);
//    string content = sztime;
//    content += buf;
//    content += "\n";

//    char buff[1024]={0};
//	word wMaxPlayer = m_pRoomKindInfo->nStartMaxPlayer;

//    READ_LOCK(m_list_mutex);
//    for (int i = 0; i < wMaxPlayer; ++i)
//	{
//        shared_ptr<CServerUserItem> pIServerUserItem;
//        pIServerUserItem.reset();
//		{
//            if (!m_UserList[i])
//                continue;
//			pIServerUserItem = m_UserList[i];
//		}

//        if(pIServerUserItem)
//        {
//            word tableid = pIServerUserItem->GetTableID();
//            word chairid = pIServerUserItem->GetChairID();
//            dword userid = pIServerUserItem->GetUserID();
//            snprintf(buff,sizeof(buff),"chair(%d,%d), userid:[%d], pIServerUserItem:[%x]\n", tableid, chairid, userid, pIServerUserItem.get());
//            content += buff;
//        }
//    }

////Cleanup:
//    FILE* fp = fopen("log_userlist.log","a");
//    fputs(content.c_str(),fp);
//    fclose(fp);
//}


//void CTableFrame::log_writescore(const char* fmt,...)
//{
//    va_list va;
//    char sztime[1024]={0};
//    char buf[1024]={0};
//    va_start(va,fmt);
//    vsnprintf(buf,sizeof(buf),fmt,va);
//    va_end(va);

//    time_t tt;
//    time( &tt);
//    struct tm* tm = localtime(&tt);
//    snprintf(sztime,sizeof(sztime),"%02d:%02d:%02d : ",tm->tm_mday,tm->tm_hour,tm->tm_min);
//    string content = sztime;
//    content += buf;
//    content += "\n";

//    FILE* fp = fopen("writescore.log","a");
//    fputs(content.c_str(),fp);
//    fclose(fp);
//}


//bool CTableFrame::WriteScoreChangeRecordLogToDB(string &curMonth, dword userId, dword nRound, tagScoreInfo& scoreInfo, shared_ptr<CServerUserItem>& pServerUserItem, SCORE &sourceScore, SCORE &newScore)
//{
//    LOG_ERROR << "WriteScoreChangeRecordLogToDB NO IMPL yet!";

//    bool bSuccess = false;

//    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);

//    string sql = "INSERT INTO db_record.record_gameround_detail_"+curMonth+"(config_id, round_id, user_id, account, table_id, chair_id, bet_score, score, revenue, agent_revenue, "
//                 "source_score, target_score, bAndroid) VALUES("
//            + to_string(m_pRoomKindInfo->nConfigId) + ","
//            + to_string(nRound) + ","
//            + to_string(userId) + ",'"
//            + pServerUserItem->getAccount() + "',"
//            + to_string(GetTableID())+ ","
//            + to_string(scoreInfo.nChairId) + ","
//            + to_string(scoreInfo.nBetScore) + ","
//            + to_string(scoreInfo.nAddScore) + ","
//            + to_string(scoreInfo.nRevenue) + ","
//            + to_string(scoreInfo.nAgentRevenueScore) + ","
//            + to_string(sourceScore) + ","
//            + to_string(newScore) + ","
//            + to_string((int)pServerUserItem->IsAndroidUser()) +
//            ")";

//    bSuccess = redisClient->PushSQL(sql);

////Cleanup:
//    return (bSuccess);
//}

//bool CTableFrame::WriteRoundScoreChangeRecordLogToDB(string &curMonth, dword nRound, SCORE &waste_all, SCORE &waste_system, SCORE &waste_android, SCORE &revenue, SCORE &agent_revenue)
//{
//    LOG_ERROR << "WriteRoundScoreChangeRecordLogToDB NO IMPL yet!";

//    bool bSuccess = false;

//    int nAllPlayerCount = 0, nAndroidPlayerCount = 0;
//    GetPlayerCount(nAllPlayerCount, nAndroidPlayerCount);

//    auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//    shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);

//    string sql = "INSERT INTO db_record.record_gameround_collect_"+curMonth+"(config_id, round_id, table_id, user_count_all, android_count, "
//                 "waste_all, waste_system, waste_android, revenue, agent_revenue, start_time) VALUES("
//            + to_string(m_pRoomKindInfo->nConfigId) + ","
//            + to_string(nRound) + ","
//            + to_string(GetTableID()) + ","
//            + to_string(nAllPlayerCount) + ","
//            + to_string(nAndroidPlayerCount) + ","
//            + to_string(waste_all) + ","
//            + to_string(waste_system) + ","
//            + to_string(waste_android) + ","
//            + to_string(revenue) + ","
//            + to_string(agent_revenue) + ",'"
//            + to_iso_extended_string(m_startDateTime) +
//            + "')";

//    bSuccess = redisClient->PushSQL(sql);

////Cleanup:
//    return (bSuccess);
//}

//bool CTableFrame::RedisLoginInfoScoreBankScoreChange(int32_t userId, SCORE nAddscore, SCORE nAddBankScore, SCORE &newScore, SCORE &newBankScore)
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

//// insert the special score changed record to the db_record database for log user changed.
//bool CTableFrame::InsertScoreChangeRecord(dword userid, SCORE lUserScore, SCORE lBankScore,
//                                         SCORE lAddScore, SCORE lAddBankScore,
//                                         SCORE lTargetUserScore, SCORE lTargetBankScore,
//                                         int type)
//{
//    bool bSuccess = false;
//    do
//    {
//        char sql[1024]={0};
//        snprintf(sql,sizeof(sql),"INSERT INTO db_record.record_score_change(userid, source_score,"
//                                 " source_bankscore, change_score, change_bank, target_score, target_bankscore, type) VALUES(%d, %lf,"
//                                 " %lf, %lf, %lf, %lf, %lf, %d)", userid, lUserScore, lBankScore,
//                                 lAddScore, lAddBankScore, lTargetUserScore, lTargetBankScore, type);
//        // try to bind the special redis client item value data for later user content item value content.
//        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
//        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);
//        bSuccess = redisClient->PushSQL(sql);

//        LOG_DEBUG << " >>> InsertScoreChangeRecord push sql to redis, sql: " << sql;
//    }   while (0);
////Cleanup:
//    return (bSuccess);
//}

//bool CTableFrame::AddUserScore(uint32_t chairId, int64_t addScore, bool bBroadcast)
//{
//    bool bSuccess = false;
//    do
//    {
//        uint32_t maxPlayer = m_pGameRoomInfo->maxPlayerNum;

//        if (chairId >= maxPlayer)
//            break;

//        // try to get the special server user item data value now.
//        shared_ptr<CServerUserItem> pIServerUserItem;
//        {
////            READ_LOCK(m_mutex);
//            pIServerUserItem = m_UserList[maxPlayer];
//        }

//        if (!pIServerUserItem)
//        {
//            break;
//        }

//        // update the special player score value now.
//        AddUserScore(pIServerUserItem, addScore);
//        if(bBroadcast)
//        {
//            BroadcastUserScore(pIServerUserItem);
//        }

//    }while(0);
//Cleanup:
//    return (bSuccess);
//}

//bool CTableFrame::AddUserScore(shared_ptr<CServerUserItem>& pIServerUserItem, int64_t addScore, int64_t revenue)
//{
//    bool bSuccess = false;

//    do
//    {
//        if (!pIServerUserItem)
//            break;

//        int64_t newScore = 0;
//        int64_t userId = pIServerUserItem->GetUserId();

//        if(!pIServerUserItem->IsAndroidUser()) //not a android
//        {
////            GameGlobalFunc::RedisLoginInfoScoreBankScoreChange(userId, llScore, 0, newScore, newBankScore);

//            int64_t sourceScore = pIServerUserItem->GetUserScore();
//            int64_t targetScore = sourceScore + addScore;
//            pIServerUserItem->SetUserScore(targetScore);

//            LOG_DEBUG << "AddUserScore userId:" << userId << ", Source Score:" << sourceScore << ", AddScore:" << addScore << ", lNewScore:" << targetScore;

//            GSUserBaseInfo &userBaseInfo = pIServerUserItem->GetUserBaseInfo();

//            UpdateUserScoreToDB(userBaseInfo, addScore, revenue);

//            GameGlobalFunc::InsertScoreChangeRecord(userId, sourceScore, newBankScore, llScore, 0, targetScore, newBankScore,
//                                                        SCORE_CHANGE_PLAYGAME, m_pRoomKindInfo->nConfigId);

//        }else//is android
//        {
//            int64_t targetScore = pIServerUserItem->GetUserScore() + addScore;
//            pIServerUserItem->SetUserScore(targetScore);

////            // update the special user score to database now.
////            UpdateUserScoreToDB(userId, llScore, false);
////            // UpdateUserScoreToCenterServer(userid, nOldScore, nAddScore, SCORE_CHANGE_PLAYGAME);

////            if(m_pGameKindInfo->bWriteAndroidDetail)
////            {
////                GameGlobalFunc::InsertScoreChangeRecord(userId, sourceScore, 0, llScore, 0, targetScore, 0, SCORE_CHANGE_PLAYGAME, m_pRoomKindInfo->nConfigId);
////            }
//        }
//    } while (0);
//Cleanup:
//    return bSuccess;
//}


//bool CTableFrame::AddEarnScoreInfo(shared_ptr<CServerUserItem>& pIServerUserItem, int64_t llAddEarnScore)
//{
//    bool bSuccess = false;
////    do
////    {
////        string strValue;
////        dword userid = pIServerUserItem->GetUserID();
////        string strKeyName = REDIS_EARNSCORE_PREFIX + to_string(userid);

////        auto fun = boost::bind(&RedisClientPool::ReleaseRedisClient, RedisClientPool::GetInstance(), _1);
////        shared_ptr<RedisClient> redisClient(RedisClientPool::GetInstance()->GetRedisClient(), fun);

////        SCORE llResultScore = 0;
////        if (redisClient->hincrby_float(strKeyName,"nEarnScore", llAddEarnScore, &llResultScore)==false)
////        {
////            LOG_WARN << "AddEarnScoreInfo failed, userid:" << userid;
////        }

////        // check the result value.
////        if (llResultScore < 0) {
////            LOG_WARN << "AddEarnScoreInfo < 0 error!, llReturnScore:" << llResultScore;
////        }

////    } while (0);
//////Cleanup:
//    return (bSuccess);
//}

//bool CTableFrame::SendUserWinScoreInfo(shared_ptr<CServerUserItem>& pIServerUserItem, const tagScoreInfo& scoreInfo)
//{
//    bool bSuccess = false;
//    do
//    {
//        LOG_ERROR << "SendUserWinScoreInfo NO IMPL yet!";

//    } while (0);
////Cleanup:
//    return bSuccess;
//}


//bool CTableFrame::setWeakconn(dword userid, TcpConnectionWeakPtr& weakconn)
//{
//    LOG_INFO << "setWeakconn for userid:" << userid;
//    m_weakconn = weakconn;
//    return true;
//}

//// try to clean the special weak conn now.
//bool CTableFrame::cleanWeakconn(dword userid)
//{
//    LOG_INFO << "cleanWeakconn for userid:" << userid;
//    m_weakconn.reset();
//    return true;
//}



