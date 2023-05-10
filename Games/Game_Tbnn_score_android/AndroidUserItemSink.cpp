#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>

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
#include <utility>
#include <functional>
#include <chrono>
#include <random>

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

#include "proto/Tbnn.Message.pb.h"

#include "../Game_Tbnn/CMD_Game.h"

using namespace Tbnn;
#include "AndroidUserItemSink.h"

#define ID_TIME_CALL_BANKER				100					//叫庄定时器(3S)
#define ID_TIME_ADD_SCORE				101					//下注定时器(3S)
#define ID_TIME_OPEN_CARD				102					//开牌定时器(5S)

//定时器时间
#define TIME_BASE						1000				//基础定时器
//#define TIME_CALL_BANKER				5					//叫庄定时器(3S)
//#define TIME_ADD_SCORE					7					//下注定时器(3S)
//#define TIME_OPEN_CARD					7					//开牌定时器(5S)


uint32_t	CAndroidUserItemSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 5,10,15,20 };	//下注倍数
//BYTE	CAndroidUserItemSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,4 };		//叫庄倍数

CAndroidUserItemSink::CAndroidUserItemSink()
//    : m_pTableFrame(NULL)
//    , m_wChairID(INVALID_CHAIR)
//    , m_pAndroidUserItem(NULL)
{
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    randomRound=0;
    CountrandomRound=0;
    srand(time(NULL));
    ClearAndroidCount=0;
    m_AndroidPlayerCount=RandomInt(10,20);
    m_icishu=0;
    ZeroMemory(m_jiabeibeishu,sizeof(m_jiabeibeishu));
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

//        //查询接口
//        // m_pIAndroidUserItem=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,IAndroidUserItem);
//        // if (m_pIAndroidUserItem==NULL) return false;
        m_pTableFrame   =  pTableFrame;
         m_TimerLoopThread = m_pTableFrame->GetLoopThread();
        m_pAndroidUserItem = pUserItem;

//        // try to update the specdial room config data item.
        m_pGameRoomKind = m_pTableFrame->GetGameRoomInfo();
        if (m_pGameRoomKind) {
            m_szRoomName = "GameServer_"+m_pGameRoomKind->roomName;
        }

//        // check if user is android user item.
        if (!pUserItem->IsAndroidUser()) {
//            // LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserID();
        }
//        srand((unsigned)time(NULL));
        ReadConfigInformation();
//        randomRound=RandomInt(10,20);
//        CountrandomRound=0;
        bSuccess = true;
    }   while (0);
    //Cleanup:
    return (bSuccess);
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
    if(pUserItem)
    {
        m_pAndroidUserItem = pUserItem;
    }
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    if(pTableFrame)
    {
        m_pTableFrame   =  pTableFrame;
        m_pGameRoomKind  =  m_pTableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pTableFrame->GetLoopThread();
        if(m_pGameRoomKind)
        {
            m_szRoomName = "GameServer_" + m_pGameRoomKind->roomName;
        }
        ReadConfigInformation();
    }
}
bool CAndroidUserItemSink::RepositionSink()
{
//    m_pTableFrame = NULL;
    m_wChairID = INVALID_CHAIR;
//    m_pAndroidUserItem = NULL;
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    return true;
}

void CAndroidUserItemSink::GameTimeAddScore()
{
    //m_pAndroidUserItem->KillTimer(ID_TIME_ADD_SCORE);
//    m_TimerAddScore.stop();
    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    NN_CMD_C_AddScore addScore;

    int   m_jiabeibeishutmp[4]={0};
    if(m_cbOxCardData == 0)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[0],sizeof(m_jiabeibeishutmp));
    else if(m_isMaxCard == 1)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[3],sizeof(m_jiabeibeishutmp));
    else if(m_cbOxCardData <= 6)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[1],sizeof(m_jiabeibeishutmp));
    else if(m_cbOxCardData >= 7)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[2],sizeof(m_jiabeibeishutmp));


    int TotalWeight = 0;
    for(int i=0;i<4;i++)
        TotalWeight +=m_jiabeibeishutmp[i];

    int randWeight = rand()%TotalWeight+1;
    int tempweightsize = 0;
    BYTE cbJettonIndex = 0;
    for (int i = 0; i < 4; i++)
    {
        tempweightsize += m_jiabeibeishutmp[i];
        if (randWeight <= tempweightsize)
        {
            if(i==0)
            {
                //openLog("GameTimeAddScore 0 randWeight=%d",randWeight);
                cbJettonIndex = 0;
                break;
            }
            else if(i==1)
            {
                //openLog("GameTimeAddScore 1 randWeight=%d",randWeight);
                cbJettonIndex = 1;
                break;
            }
            else if(i==2)
            {
                //openLog("GameTimeAddScore 2 randWeight=%d",randWeight);
                cbJettonIndex = 2;
                break;
            }
            else if(i==3)
            {
                //openLog("GameTimeAddScore 3 randWeight=%d",randWeight);
                cbJettonIndex = 3;
                break;
            }
        }
    }

    openLog("111111 i=%d cbJettonIndex=%d",m_pAndroidUserItem->GetChairId(),(int)cbJettonIndex);

    addScore.set_cbjettonindex( cbJettonIndex);
    string content = addScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),NN_SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send NN_SUB_C_ADD_SCORE";
    LOG(INFO)<<"******************************cbJettonIndex"<<(int)cbJettonIndex;
}
void CAndroidUserItemSink::GameTimeOpencard()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(), NN_SUB_C_OPEN_CARD, NULL, 0);
}


bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    switch (wSubCmdID)
    {
    case NN_SUB_S_GAME_START_AI:
    {
        Tbnn::CMD_S_GameStartAi GameStartAi;
        if (!GameStartAi.ParseFromArray(pData,wDataSize))
            return false;
        m_cbOxCardData = GameStartAi.cboxcarddata();
        m_isMaxCard = GameStartAi.ismaxcard();
        openLog("NN_SUB_S_GAME_START_AI m_cbOxCardData=%d m_isMaxCard=%d",m_cbOxCardData,m_isMaxCard);
        m_m_cbPlayStatus = true;
        return true;
    }
    case SC_GAMESCENE_FREE:
    {
        m_m_cbPlayStatus = false;
        return OnFreeGameScene(wSubCmdID,pData, wDataSize);
    }
    case NN_SUB_S_GAME_START:
    {
        ClearAndroidCount=0;
        m_m_cbPlayStatus = true;
        return OnSubGameStart(wSubCmdID,pData, wDataSize);
    }
        break;
    case NN_SUB_S_ADD_SCORE_RESULT:
    {
        return OnSubAddScore(wSubCmdID,pData, wDataSize);
    }
        break;
    case NN_SUB_S_SEND_CARD:
    {
        return OnSubSendCard(wSubCmdID,pData, wDataSize);
    }
    case NN_SUB_S_OPEN_CARD_RESULT:
    {
        return OnSubOpenCard(wSubCmdID,pData, wDataSize);
    }
        break;
    case NN_SUB_S_GAME_END:
    {
        return ClearAllTimer();
    }
        break;
    default: break;
    }
    return true;
}

bool CAndroidUserItemSink::OnFreeGameScene(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_MSG_GS_FREE GSFree;
    if (!GSFree.ParseFromArray(pData,wDataSize))
        return false;

    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&JettonMultipl = GSFree.cbjettonmultiple();
    for(int i =0;i<JettonMultipl.size();i++)
        m_cbJettonMultiple[i] = JettonMultipl[i];

    return true;
}

bool CAndroidUserItemSink::OnSubGameStart(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_GameStart GameStart;
    if (!GameStart.ParseFromArray(pData,wDataSize))
        return false;
    m_icishu++;
    CountrandomRound++;
    m_wPlayerCount = GameStart.cbplaystatus().size();

    int dwShowTime = 2;//发牌动画  1.5
    int jettontime = GameStart.cbaddjettontime();
    int dwAddScore = dwShowTime + rand() % (jettontime -1);

    m_TimerAddScore = m_TimerLoopThread->getLoop()->runAfter(dwAddScore, boost::bind(&CAndroidUserItemSink::GameTimeAddScore, this));
    return true;
}

bool CAndroidUserItemSink::OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_AddScoreResult AddScoreResult;
    if (!AddScoreResult.ParseFromArray(pData,wDataSize))
        return false;

    if (AddScoreResult.waddjettonuser()== m_wChairID && m_m_cbPlayStatus)
    {
        //m_pAndroidUserItem->KillTimer(ID_TIME_ADD_SCORE);
//        m_TimerAddScore.stop();
        m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
        m_wAddScoreNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_SendCard SendCard;
    if (!SendCard.ParseFromArray(pData,wDataSize))
        return false;

    if(!m_m_cbPlayStatus)
        return true;

    int dwShowTime = 2;//抢庄动画  3.0
    int opentime = SendCard.cbopentime();
    int dwOpenCardTime = dwShowTime + rand() % (opentime - 1);//((opentime - 1)* TIME_BASE);
    //m_pAndroidUserItem->SetTimer(ID_TIME_OPEN_CARD, dwOpenCardTime);
    m_TimerOpenCard = m_TimerLoopThread->getLoop()->runAfter(dwOpenCardTime,bind(&CAndroidUserItemSink::GameTimeOpencard, this));
//    m_TimerOpenCard.start(dwOpenCardTime, bind(&CAndroidUserItemSink::GameTimeOpencard, this, std::placeholders::_1), NULL, 1, false);
    return true;
}

bool CAndroidUserItemSink::OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_OpenCardResult OpenCardResult;
    if (!OpenCardResult.ParseFromArray(pData,wDataSize))
        return false;

    if (OpenCardResult.wopencarduser() == m_wChairID)
    {
        //m_pAndroidUserItem->KillTimer(ID_TIME_OPEN_CARD);
//        m_TimerOpenCard.stop();
        m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
    }



    return true;
}

bool CAndroidUserItemSink::OnSubOpenCardResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    return true;
}

bool CAndroidUserItemSink::ClearAllTimer()
{
    if(CountrandomRound>=randomRound)
    {
        m_pAndroidUserItem->setOffline();
        CountrandomRound=0;
    }
    int getout=RandomInt(0,100);
    if(m_pTableFrame->GetPlayerCount(false)>=3&&getout<50)
    {
        m_pAndroidUserItem->setOffline();
    }
    if(m_pAndroidUserItem->GetUserScore()<m_pTableFrame->GetGameRoomInfo()->enterMinScore
            || m_pAndroidUserItem->GetUserScore()>20000  )
    {
        m_pAndroidUserItem->setOffline();
    }

    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
    return true;
}


//读取配置
void CAndroidUserItemSink::ReadConfigInformation()
{
    //设置文件名
    const TCHAR * p = nullptr;
    TCHAR szPath[MAX_PATH] = TEXT("");
    TCHAR szConfigFileName[MAX_PATH] = TEXT("");
    TCHAR OutBuf[255] = TEXT("");
    CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
    myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/tbnn_config.ini"), szPath);

    TCHAR INI_SECTION_CONFIG[30] = TEXT("");
    myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"), m_pTableFrame->GetGameRoomInfo()->roomId);

    TCHAR KeyName[64] = TEXT("");
    memset(KeyName, 0, sizeof(KeyName));
    {
        for(int i=0;i<4;i++)
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("JIAB_%d"), i );

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"35,35,15,15", OutBuf, sizeof(OutBuf), szConfigFileName);

            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr)
                return;
            float temp = 0;
            temp = atoi(p);
            m_jiabeibeishu[i][0] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_jiabeibeishu[i][1] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_jiabeibeishu[i][2] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_jiabeibeishu[i][3] = temp;
        }
    }
    int test = 0;
    test =1;
}

void  CAndroidUserItemSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//qznn//qznn_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pTableFrame->GetGameRoomInfo()->roomId);
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
