#include <glog/logging.h>

#include <boost/filesystem.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string> 
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <functional>
#include <chrono>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

#include "proto/Blackjack.Message.pb.h"

#include "../Game_hjkNew/MSG_HJK.h"
#include "../Game_hjkNew/GameLogic.h"

using namespace Blackjack;

#include "AndroidUserItemSink.h"

CAndroidUserItemSink::CAndroidUserItemSink()
{
    memset(&m_cardData, 0, sizeof(m_cardData));
//    m_wPlayerCount = 0;
    srand(time(NULL));
    // ZeroMemory(m_bets,sizeof(m_bets));
    m_m_cbPlayStatus = false;
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{

}
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    //LOG(INFO) << "CAndroidUserItemSink::Initialization pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
    // //openLog("Initialization。。。");
    bool bSuccess = false;
    do
    {
//        //  check the table and user item now.
        if ((!pTableFrame) || (!pUserItem)) {
//            //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
            break;
        }
        m_pTableFrame   =  pTableFrame;
        m_TimerLoopThread = m_pTableFrame->GetLoopThread();
        m_pAndroidUserItem = pUserItem;
        m_wChairID = pUserItem->GetChairId();
//		m_userId = pUserItem->GetUserId();
//        // try to update the specdial room config data item.
        m_pGameRoomKind = m_pTableFrame->GetGameRoomInfo();
        if (m_pGameRoomKind) {
            m_szRoomName = "GameServer_"+m_pGameRoomKind->roomName;
        }

//        // check if user is android user item.
        if (!pUserItem->IsAndroidUser()) {
//            // LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
        }
        ReadConfigInformation();
        bSuccess = true;
    }   while (0);
    //Cleanup:
    return (bSuccess);
}

bool CAndroidUserItemSink::RepositionSink()
{
//    m_pTableFrame = NULL;
    m_wChairID = INVALID_CHAIR;
//    m_pAndroidUserItem = NULL;
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_cardpoint = 0;
    m_cardtype = CT_POINTS;
//    m_wPlayerCount = 0;
    return true;
}

void CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, int wChairID, shared_ptr<IServerUserItem>& pUserItem)
{
    m_pTableFrame = pTableFrame;

    m_pAndroidUserItem = pUserItem;
    m_wChairID = wChairID;
//    m_userId = pUserItem->GetUserId();
}

void CAndroidUserItemSink::GameTimeInsure()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerInsure);
    CMD_C_Insure Insure;
    Insure.set_wbuy(rand()%10 > 8 );
    string content = Insure.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),SUB_C_INSURE, (uint8_t*)content.data(), content.size());
}

void CAndroidUserItemSink::GameTimerOperate()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerOperate);
    CMD_C_Operate Oper;
    int randCnt;
    do{
        if( (m_cOperCode & OPER_CALL) != 0)
        {
            randCnt = rand()%10;
//            LOG(INFO) << "Android Oper rand:" << randCnt << ", m_cardpoint" << (int)m_cardpoint << ", hasA" << m_hasA;
            if( m_cardpoint >= 17 || (m_hasA && m_cardpoint + 10 >= 17))
            {
                Oper.set_wopercode(OPER_STOP);
                break;
            }

            if(m_cardpoint <= 16 && randCnt > 4)
            {
                Oper.set_wopercode(OPER_CALL);
                break;
            }

            if( m_cardpoint <= 15 && randCnt > 8 && ( (m_cOperCode & OPER_DOUBLE) != 0))
            {
                Oper.set_wopercode(OPER_DOUBLE);
                break;
            }

            if(m_cardpoint <= 11)
            {
                Oper.set_wopercode(OPER_CALL);
                break;
            }
             Oper.set_wopercode(OPER_STOP);
        }else
        {
             Oper.set_wopercode(OPER_STOP);
        }
        break;
    }while(0);

    string content = Oper.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),SUB_C_OPERATE, (uint8_t*)content.data(), content.size());
}

void CAndroidUserItemSink::GameTimeAddScore()
{
    //m_pAndroidUserItem->KillTimer(ID_TIME_ADD_SCORE);
    // m_TimerAddScore.stop();
    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    CMD_C_AddScore addScore;

//    openLog("111111 i=%d cbJettonIndex=%d",m_pAndroidUserItem->GetChairId(),(int)cbJettonIndex);
    int64_t floorScore = m_pGameRoomKind->floorScore;
    int64_t randNum = rand()%1000;
    int targetIdx = 0;
    int64_t score,mul;
    for(int i = 0; i < 5; i++)
    {
        if( randNum < m_bets[i])
        {
            targetIdx = i;
            break;
        }
    }
    if(targetIdx != 0)
    {
        mul = rand()%(m_times[targetIdx]-m_times[targetIdx-1])+m_times[targetIdx-1];
    }else
    {
        mul = 1;
    }
    score = mul * floorScore;
    int64_t userScore = m_pAndroidUserItem->GetUserScore();
    score = score < userScore ? score : userScore;
    score = score/100 * 100;
//    LOG(WARNING) << "targetIdx: " << targetIdx << "("<<m_times[targetIdx]<<")";
//    LOG(WARNING) << "rand:" << (int)randNum <<",Mul:" << (int)mul << ", Score:" << (int)score << ", user score :" << (int)userScore;
    // int64_t ceilScore = m_pAndroidUserItem->GetUserScore();
    // ceilScore = m_pGameRoomKind->ceilScore > ceilScore ? ceilScore : m_pGameRoomKind->ceilScore;
    // int64_t score = GlobalFunc::RandomInt64(m_pGameRoomKind->floorScore, ceilScore)/100 * 100;
//    addScore.set_dscore( m_cbJettonMultiple[cbJettonIndex]);
    addScore.set_dscore(score);
    string content = addScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send SUB_C_ADD_SCORE";
//    LOG(INFO)<<"******************************cbJettonIndex"<<m_jiabeibeishutmp[cbJettonIndex];
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),SUB_C_END_SCORE, (uint8_t*)(""), 0);
}


bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    switch (wSubCmdID)
    {
    case SUB_S_GAME_START_AI:
    {
        Blackjack::CMD_S_GameStartAi GameStartAi;
        if (!GameStartAi.ParseFromArray(pData,wDataSize))
            return false;

        m_m_cbPlayStatus = true;
        return true;
    }
    case SUB_SC_GAMESCENE_FREE:
    {
        m_m_cbPlayStatus = false;
        return OnFreeGameScene(pData, wDataSize);
    }
    case SUB_SC_GAMESCENE_SCORE:
    {
        return OnScoreGameScene(pData, wDataSize);
    }
    case SUB_SC_GAMESCENE_INSURE:
    {
        return  OnInsureGameScene(pData, wDataSize);
    }
    case SUB_SC_GAMESCENE_PLAY:
    {
        return OnPlayGameScene(pData, wDataSize);
    }
        break;
    case SUB_S_GAME_START:
    {
        return OnSubGameStart(pData, wDataSize);
    }
        break;
    case SUB_S_DEAL_CARD:
    {
        return OnSubDealCard(pData, wDataSize);
    }
        break;
    case SUB_S_INSURE:
    {
        return OnSubInsure(pData, wDataSize);
    }
        break;
    case SUB_S_OPERATE:
    {
        return OnSubOperate(pData, wDataSize);
    }
    case SUB_S_OPER_RESULT:
    {
        return OnSubOperateResult(pData, wDataSize);
        break;
    }

    case SUB_S_GAME_END:
    {
        return OnSubGameend(pData, wDataSize);
        break;
    }
    case SUB_S_ADD_SCORE:
    {
        return OnSubAddScore(pData, wDataSize);
        break;
    }
    case SUB_S_BANKER_DEAL:
    {
        return OnBankerDeals(pData, wDataSize);
    }
    default: break;
    }
    return true;
}

bool CAndroidUserItemSink::OnBankerDeals(const uint8_t *pData, uint32_t wDataSize)
{
    ClearAllTimer();
    return true;
}

bool CAndroidUserItemSink::OnFreeGameScene(const uint8_t* pData, uint32_t wDataSize)
{
    CMD_S_StatusFree GSFree;
    if (!GSFree.ParseFromArray(pData,wDataSize))
        return false;

    return true;
}

bool CAndroidUserItemSink::OnScoreGameScene(const uint8_t* pData, uint32_t wDataSize)
{
    CMD_S_StatusScore GSScore;
    if (!GSScore.ParseFromArray(pData,wDataSize))
        return false;

    return true;
}

bool CAndroidUserItemSink::OnInsureGameScene(const uint8_t* pData, uint32_t wDataSize)
{
    CMD_S_StatusInsure GSScore;
    if (!GSScore.ParseFromArray(pData,wDataSize))
        return false;

    return true;
}

bool CAndroidUserItemSink::OnPlayGameScene(const uint8_t* pData, uint32_t wDataSize)
{
    CMD_S_StatusPlay GSScore;
    if (!GSScore.ParseFromArray(pData,wDataSize))
        return false;

    return true;
}

void CAndroidUserItemSink::AnalyseCard()
{
    m_cardpoint = 0;
    uint8_t i = 0;
    for(;i < MAX_COUNT; i++)
    {
        if(m_cardData[i] == 0) break;
        m_cardpoint += GetCardLogicValue(m_cardData[i]);
        m_hasA = m_hasA || (GetCardLogicValue(m_cardData[i]) ==1);
    }
    //ignore balckjack because no chance to operate
    m_cardtype = CT_POINTS;
}

uint8_t CAndroidUserItemSink::GetCardLogicValue(uint8_t carddata)
{
    uint8_t value = carddata & LOGIC_MASK_VALUE;
    return value < 10 ? value : 10;
}

bool CAndroidUserItemSink::OnSubDealCard(const uint8_t *pData, uint32_t wDataSize)
{
    CMD_S_DealCard DealCard;
    if (!DealCard.ParseFromArray(pData,wDataSize))
        return false;

     bool needInsure = DealCard.wneedinsure();

     int timeLeft = DealCard.wtimeleft();
     if( timeLeft > 6)
     {
         timeLeft = 6;
     }
//     LOG(INFO) << "Need Insure ? " << needInsure;
     if(needInsure)
     {
         int dwInsureTime = rand() % (timeLeft -1);
         m_TimerInsure = m_TimerLoopThread->getLoop()->runAfter(dwInsureTime, boost::bind(&CAndroidUserItemSink::GameTimeInsure, this));
//         LOG(INFO)<<"dwInsureTime"<<"="<<dwInsureTime;
     }
     uint8_t handLen = DealCard.cbhandcards().size();
    for(uint8_t i = 0; i < handLen; i++)
    {

        if(DealCard.cbhandcards().Get(i).wchairid() == m_wChairID)
        {
//            LOG(ERROR) << "----my chair:" << m_wChairID << ", this chair:" << DealCard.cbhandcards().Get(i).wchairid();
            HandCard m_hand = DealCard.cbhandcards().Get(i);
            m_cardpoint = 0;
            m_cardtype = m_hand.cbhandcardtype();
            uint8 cardLen = m_hand.cbhandcarddata().size();
            for(uint8_t j = 0; j < cardLen; j++)
            {
                m_cardData[j] = m_hand.cbhandcarddata().Get(j);
                m_cardpoint += GetCardLogicValue(m_cardData[j]);
                m_hasA = m_hasA || GetCardLogicValue(m_cardData[j]) == 1;
            }
            break;
        }
    }

    return true;
}

bool CAndroidUserItemSink::OnSubInsure(const uint8_t *pData, uint32_t wDataSize)
{
    return true;
}

bool CAndroidUserItemSink::OnSubOperate(const uint8_t *pData, uint32_t wDataSize)
{
    CMD_S_Operate OperInfo;
    if (!OperInfo.ParseFromArray(pData,wDataSize))
        return false;

    uint64_t curUser = OperInfo.woperuser();
//    LOG(INFO) << "On Operate, curUser:" << (int)curUser << ", chair:" << m_wChairID;
    if(curUser == m_wChairID)
    {
        m_cOperCode = OperInfo.wopercode();
        uint32_t curSeat = OperInfo.wseat();
        uint32_t curTimeleft = OperInfo.wtimeleft()-1;
        m_dwOpEndTime  = (uint32_t)time(NULL) + curTimeleft;
        m_wTotalOpTime = curTimeleft;

        curTimeleft = curTimeleft > 6 ? 6 : curTimeleft;

        int dwOperTime = curTimeleft > 2 ? (rand() % (curTimeleft - 2) + 2) : curTimeleft;
        m_TimerOperate = m_TimerLoopThread->getLoop()->runAfter(dwOperTime, bind(&CAndroidUserItemSink::GameTimerOperate, this));
    }
   
    return true;
}

bool CAndroidUserItemSink::OnSubOperateResult(const uint8_t *pData, uint32_t wDataSize)
{
    CMD_S_Oper_Result OperRes;
    if (!OperRes.ParseFromArray(pData,wDataSize))
        return false;

    uint64_t curUser = OperRes.woperuser();
    m_TimerLoopThread->getLoop()->cancel(m_TimerOperate);
    if(curUser == m_wChairID)
    {
        m_cOperCode = OperRes.wopercode();
//        uint32_t timeLeft = (uint32_t)time(NULL) - m_dwOpEndTime;
        LOG(INFO) << "Robot oper Opercode:" <<  (int)m_cOperCode;

        if(m_cOperCode != 0)
        {
            if(OperRes.has_cbfirsthandcarddata())
            {
                for(int8_t i = 0; i < MAX_COUNT; i++)
                {
                    if(m_cardData[i] == 0)
                    {
                        m_cardData[i] = OperRes.cbfirsthandcarddata();
                        m_cardtype = OperRes.cbfirsthandcardtype();
                        if(m_cardtype == CT_POINTS)
                        {
                            m_cardpoint += GetCardLogicValue(m_cardData[i]);
                            LOG(INFO) << "Robot oper result. card type" <<  (int)m_cardtype << ", m_cardpoint:" << (int)m_cardpoint << ", new card" << (int)m_cardData[i];
                            int dwOperTime = rand() % 3 + 1;
                            m_TimerOperate = m_TimerLoopThread->getLoop()->runAfter(dwOperTime, bind(&CAndroidUserItemSink::GameTimerOperate, this));
                        }
                        break;
                    }
                }
            }
        }
    }
    return true;
}

bool CAndroidUserItemSink::OnSubGameend(const uint8_t *pData, uint32_t wDataSize)
{
    ClearAllTimer();
    m_cardpoint = 0;
    m_hasA = false;
    m_cardtype = CT_BLACKJACK;
    m_cOperCode = 0;
    memset(m_cardData,0, sizeof(m_cardData));
    return true;
}

bool CAndroidUserItemSink::OnSubGameStart(const uint8_t* pData, uint32_t wDataSize)
{
    CMD_S_GameStart GameStart;
    if (!GameStart.ParseFromArray(pData,wDataSize))
        return false;
//    m_wPlayerCount = GameStart.cbplaystatus().size();
    m_wChairID = m_pAndroidUserItem->GetChairId();
    int betTime = GameStart.wtimeleft();
    betTime = betTime > 6 ? 6 : betTime;
    int dwAddScoreTime = 1 + rand() % (betTime -1);
    m_hasA = false;
    m_TimerAddScore = m_TimerLoopThread->getLoop()->runAfter(dwAddScoreTime, bind(&CAndroidUserItemSink::GameTimeAddScore, this));

//    LOG(INFO)<<"dwAddScoreTime"<<"="<<dwAddScoreTime;

//    LOG(WARNING) << "Game start  chair id: " << m_wChairID;
    return true;
}

bool CAndroidUserItemSink::OnSubAddScore(const uint8_t* pData, uint32_t wDataSize)
{
//    CMD_S_AddScore AddScoreResult;
//    if (!AddScoreResult.ParseFromArray(pData,wDataSize))
//        return false;


    return true;
}

bool CAndroidUserItemSink::ClearAllTimer()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerInsure);
    m_TimerLoopThread->getLoop()->cancel(m_TimerOperate);
    return true;
}


//读取配置
void CAndroidUserItemSink::ReadConfigInformation()
{

    const TCHAR * p = nullptr;
    TCHAR szPath[MAX_PATH] = TEXT("");
    TCHAR szConfigFileName[MAX_PATH] = TEXT("");
    TCHAR OutBuf[255] = TEXT("");
    CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
    myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/hjk_config.ini"), szPath);

    TCHAR INI_SECTION_CONFIG[30] = TEXT("");
    myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"),  m_pTableFrame->GetGameRoomInfo()->roomId);

    TCHAR KeyName[64] = TEXT("BET_0");
    // memset(KeyName, 0, sizeof(KeyName));
    // {
        // for(int i=0;i<4;i++)
        // {

            // ZeroMemory(KeyName,sizeof(KeyName));
            // wsprintf(KeyName, TEXT("BET_0"), i );

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"350,700,900,970,1000", OutBuf, sizeof(OutBuf), szConfigFileName);

            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr)
                return;
            float temp = 0;
            temp = atoi(p);
            m_bets[0] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_bets[1] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_bets[2] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_bets[3] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_bets[4] = temp;

            myprintf(INI_SECTION_CONFIG,sizeof(INI_SECTION_CONFIG), _T("%sBET_TIMES"),"");
            ZeroMemory(KeyName,sizeof(KeyName));
            myprintf(KeyName,sizeof(KeyName), _T("%sTIMES"),"");
            readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"1,5,10,30,100", OutBuf, sizeof(OutBuf), szConfigFileName);
            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr)
                return;
            temp = 0;
            temp = atoi(p);
            m_times[0] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_times[1] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_times[2] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_times[3] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_times[4] = temp;
        // }
    // }

}

void  CAndroidUserItemSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//hjk//hjk_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pTableFrame->GetGameRoomInfo()->roomId);
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] [Android TABLEID:%d usserid=%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,
            m_pTableFrame->GetTableId(),m_pAndroidUserItem->GetUserId());
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}



bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
    if ( (!pUserItem))
    {
       return false;
    }

    m_pAndroidUserItem = pUserItem;
    srand((unsigned)time(NULL));
//    ReadConfigInformation();
    m_wChairID = pUserItem->GetChairId();
    return true;
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    if (!pTableFrame)
    {
         return ;
    }

    m_pTableFrame   =  pTableFrame;
    m_pGameRoomKind  =  m_pTableFrame->GetGameRoomInfo();
     m_TimerLoopThread = m_pTableFrame->GetLoopThread();
    ReadConfigInformation();
    m_szRoomName = "GameServer_" + m_pGameRoomKind->roomName;
    vector<int64_t> jettons = m_pGameRoomKind->jettons;
    m_cbJettonMultiple.assign(jettons.begin(), jettons.end());
}

extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

// reset all the data now.
extern "C" void DeleteAndroidSink(IAndroidUserItemSink* pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    //Cleanup:
    delete pSink;
    pSink = NULL;
}
