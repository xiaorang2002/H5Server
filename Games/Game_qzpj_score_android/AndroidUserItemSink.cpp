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
//#include "public/pb2Json.h"

#include "proto/qzpj.Message.pb.h"

#include "../Game_qzpj/CMD_Game.h"
#include "../QPAlgorithm/PJ.h"

#include "AndroidUserItemSink.h"

#define ID_TIME_CALL_BANKER				100					//叫庄定时器(3S)
#define ID_TIME_ADD_SCORE				101					//下注定时器(3S)
#define ID_TIME_OPEN_CARD				102					//开牌定时器(5S)

//定时器时间
#define TIME_BASE						1000				//基础定时器
//#define TIME_CALL_BANKER				5					//叫庄定时器(3S)
//#define TIME_ADD_SCORE					7					//下注定时器(3S)
//#define TIME_OPEN_CARD					7					//开牌定时器(5S)

BYTE	CAndroidUserItemSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,3,4 };		//叫庄倍数
BYTE	CAndroidUserItemSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 1,5,15,22,30 };//下注倍数

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
    ZeroMemory(m_qzbeishu,sizeof(m_qzbeishu));
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
   // m_wChairID = INVALID_CHAIR;
//    m_pAndroidUserItem = NULL;
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    return true;
}

//void CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, int wChairID, shared_ptr<IServerUserItem>& pUserItem)
//{
//    m_pTableFrame = pTableFrame;

//    m_pAndroidUserItem = pUserItem;
//    m_wChairID = wChairID;

//}
void CAndroidUserItemSink::GameTimerCallBanker()
{
    ThisThreadTimer->cancel(m_TimerCallBanker);
    //随机叫庄
    int  m_qzbeishutmp[4]={0};
	if (m_isMaxCard == 1)
		CopyMemory(m_qzbeishutmp, m_qzbeishu[3], sizeof(m_qzbeishutmp));
    else if(m_cbOxCardData <= PJ::PP5)
        CopyMemory(m_qzbeishutmp,m_qzbeishu[0],sizeof(m_qzbeishutmp));
	else if (m_cbOxCardData <= PJ::PP9)
		CopyMemory(m_qzbeishutmp, m_qzbeishu[1], sizeof(m_qzbeishutmp));
    else if(m_cbOxCardData <= PJ::PLL)
        CopyMemory(m_qzbeishutmp,m_qzbeishu[2],sizeof(m_qzbeishutmp));
    else if(m_cbOxCardData >= PJ::PGJ)
        CopyMemory(m_qzbeishutmp,m_qzbeishu[3],sizeof(m_qzbeishutmp));

    int TotalWeight = 0;
    for(int i=0;i<4;i++)
        TotalWeight +=m_qzbeishutmp[i];

    int randWeight = rand()%TotalWeight+1;
    int tempweightsize = 0;
    BYTE cbCallBankerMul = 0;
    for (int i = 0; i < 4; i++)
    {
        tempweightsize += m_qzbeishutmp[i];
        if (randWeight <= tempweightsize)
        {
            if(i==0)
            {
                openLog("GameTimerCallBanker 0 randWeight=%d",randWeight);
                cbCallBankerMul = 0;
                break;
            }
            else if(i==1)
            {
                openLog("GameTimerCallBanker 1 randWeight=%d",randWeight);
                cbCallBankerMul = m_cbBankerMultiple[0];
                break;
            }
            else if(i==2)
            {
                openLog("GameTimerCallBanker 2 randWeight=%d",randWeight);
                cbCallBankerMul = m_cbBankerMultiple[1];
                break;
            }
            else if(i==3)
            {
                openLog("GameTimerCallBanker 3 randWeight=%d",randWeight);
                cbCallBankerMul = m_cbBankerMultiple[2];
                break;
            }
        }
    }

    openLog("111111 i=%d cbCallBankerMul=%d",m_pAndroidUserItem->GetChairId(),(int)cbCallBankerMul);

    qzpj::CMD_C_CallBanker callBanker;
    callBanker.set_cbcallflag(cbCallBankerMul);


    //发送信息
    string content = callBanker.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),qzpj::SUB_C_CALL_BANKER, (uint8_t*)content.data(), content.size());
}

void CAndroidUserItemSink::GameTimeAddScore()
{
    //m_pAndroidUserItem->KillTimer(ID_TIME_ADD_SCORE);
//    m_TimerAddScore.stop();
    ThisThreadTimer->cancel(m_TimerAddScore);
    qzpj::CMD_C_AddScore addScore;

    int   m_jiabeibeishutmp[4]={0};
	if (m_isMaxCard == 1)
		CopyMemory(m_jiabeibeishutmp, m_jiabeibeishu[3], sizeof(m_jiabeibeishutmp));
    else if(m_cbOxCardData <= PJ::PP5)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[0],sizeof(m_jiabeibeishutmp));
    else if(m_cbOxCardData <= PJ::PP9)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[1],sizeof(m_jiabeibeishutmp));
	else if (m_cbOxCardData <= PJ::PLL)
		CopyMemory(m_jiabeibeishutmp, m_jiabeibeishu[2], sizeof(m_jiabeibeishutmp));
    else if(m_cbOxCardData >= PJ::PGJ)
        CopyMemory(m_jiabeibeishutmp,m_jiabeibeishu[3],sizeof(m_jiabeibeishutmp));

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

    //openLog("111111 i=%d cbJettonIndex=%d",m_pAndroidUserItem->GetChairId(),(int)cbJettonIndex);

    addScore.set_cbjettonindex( cbJettonIndex);
    string content = addScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),qzpj::SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
}

void CAndroidUserItemSink::GameTimeOpencard()
{
    ThisThreadTimer->cancel(m_TimerOpenCard);
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(), qzpj::SUB_C_OPEN_CARD, NULL, 0);
}

bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    switch (wSubCmdID)
    {
    case qzpj::SUB_S_GAME_START_AI:
    {
		qzpj::CMD_S_GameStartAi GameStartAi;
        if (!GameStartAi.ParseFromArray(pData,wDataSize))
            return false;
        m_cbOxCardData = GameStartAi.cboxcarddata();
        m_isMaxCard = GameStartAi.ismaxcard();
        openLog("qzpj::SUB_S_GAME_START_AI m_cbOxCardData=%d m_isMaxCard=%d",m_cbOxCardData,m_isMaxCard);
        m_m_cbPlayStatus = true;
        return true;
    }
    case qzpj::SC_GAMESCENE_FREE:
    {
        m_m_cbPlayStatus = false;
        return OnFreeGameScene(wSubCmdID,pData, wDataSize);
    }
    case qzpj::SUB_S_GAME_START:
    {
        ClearAndroidCount=0;
        m_m_cbPlayStatus = true;
        return OnSubGameStart(wSubCmdID,pData, wDataSize);
    }
        break;
    case qzpj::SUB_S_CALL_BANKER:
    {
        return OnSubCallBanker(wSubCmdID,pData, wDataSize);
    }
        break;
    case qzpj::SUB_S_CALL_BANKER_RESULT:
    {
        return OnSubCallBankerResult(wSubCmdID,pData, wDataSize);
    }
        break;
    case qzpj::SUB_S_ADD_SCORE_RESULT:
    {
        return OnSubAddScore(wSubCmdID,pData, wDataSize);
    }
        break;
    //case qzpj::SUB_S_SEND_CARD:
    //{
    //    return OnSubSendCard(wSubCmdID,pData, wDataSize);
    //}
    //case qzpj::SUB_S_OPEN_CARD_RESULT:
    //{
    //   return OnSubOpenCard(wSubCmdID,pData, wDataSize);
    //}
        break;
    case qzpj::SUB_S_GAME_END:
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
    qzpj::MSG_GS_FREE GSFree;
//     if (!GSFree.ParseFromArray(pData,wDataSize))
//         return false;
// 
//     const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&JettonMultipl = GSFree.cbjettonmultiple();
//     for(int i =0;i<JettonMultipl.size();i++)
//         m_cbJettonMultiple[i] = JettonMultipl[i];
// 
//     const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&BankerMultiple= GSFree.cbcallbankermultiple();
//     for(int i =0;i<BankerMultiple.size();i++)
//         m_cbBankerMultiple[i] = BankerMultiple[i];

    return true;
}

bool CAndroidUserItemSink::OnSubGameStart(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    qzpj::CMD_S_GameStart GameStart;
    if (!GameStart.ParseFromArray(pData,wDataSize))
        return false;
    m_icishu++;
    CountrandomRound++;
    m_wPlayerCount = GameStart.cbplaystatus().size();

    int dwShowTime = 2;//叫庄动画  1.5
    int callTime = GameStart.cbcallbankertime();
    
	//计算浮动时长
	int j = RandomBetween(1, callTime - 4);
	//中间间隔时长
	double interval = 1.0f;
	//分片等待时间
	double sliceSeconds = 0.2f;
	//一定概率无中间间隔时长
	if (RandomBetween(1, 100) <= 60) {
		interval = 0;
	}
	//动画延迟时长 + 中间间隔时长 + 浮动时长
	double dwCallBankerTime = (double)dwShowTime + interval + sliceSeconds * RandomBetween(1, 5) * j;
	LOG(WARNING) << "------------------ callTime = " << dwCallBankerTime;
    m_TimerCallBanker = ThisThreadTimer->runAfter(dwCallBankerTime, boost::bind(&CAndroidUserItemSink::GameTimerCallBanker, this));
    return true;
}

bool CAndroidUserItemSink::OnSubCallBanker(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    qzpj::CMD_S_CallBanker CallBanker;
    if (!CallBanker.ParseFromArray(pData,wDataSize))
        return false;

    //自己已经叫庄 则检测定时器是否还存在
    if (CallBanker.wcallbankeruser() == ThisChairId)
    {
        ThisThreadTimer->cancel(m_TimerCallBanker);
        m_wCallBankerNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubCallBankerResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    qzpj::CMD_S_CallBankerResult BankerResult;
    if (!BankerResult.ParseFromArray(pData,wDataSize))
        return false;

    if (BankerResult.dwbankeruser() != ThisChairId)
    {
        int dwShowTime = 2;//下注动画  3.0
        int jettontime = BankerResult.cbaddjettontime();

		//计算浮动时长
		int j = RandomBetween(1, jettontime - 4);
		//中间间隔时长
		double interval = 1.0f;
		//分片等待时间
		double sliceSeconds = 0.2f;
		//一定概率无中间间隔时长
		if (RandomBetween(1, 100) <= 60) {
			interval = 0;
		}
		//动画延迟时长 + 中间间隔时长 + 浮动时长
		double dwAddScore = (double)dwShowTime + interval + sliceSeconds * RandomBetween(1, 5) * j;
		LOG(WARNING) << "------------------ jettontime = " << dwAddScore;
		m_TimerAddScore = ThisThreadTimer->runAfter(dwAddScore, boost::bind(&CAndroidUserItemSink::GameTimeAddScore, this));
    }

    return true;
}

bool CAndroidUserItemSink::OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    qzpj::CMD_S_AddScoreResult AddScoreResult;
    if (!AddScoreResult.ParseFromArray(pData,wDataSize))
        return false;

    if (AddScoreResult.waddjettonuser()== ThisChairId)
    {
        ThisThreadTimer->cancel(m_TimerAddScore);
        m_wAddScoreNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubSendCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    qzpj::CMD_S_SendCard SendCard;
    if (!SendCard.ParseFromArray(pData,wDataSize))
        return false;

    if(!m_m_cbPlayStatus)
        return true;

    int dwShowTime = 1;//摊牌动画  3.0
    int opentime = SendCard.cbopentime();

	//计算浮动时长
	int j = RandomBetween(1, opentime - 4);
	//中间间隔时长
	double interval = 1.0f;
	//分片等待时间
	double sliceSeconds = 0.2f;
	//一定概率无中间间隔时长
	if (RandomBetween(1, 100) <= 60) {
		interval = 0;
	}
	//动画延迟时长 + 中间间隔时长 + 浮动时长
	double dwOpenCardTime = (double)dwShowTime + interval + sliceSeconds * RandomBetween(1, 5) * j;
	LOG(WARNING) << "------------------ opentime = " << dwOpenCardTime;
    m_TimerOpenCard = ThisThreadTimer->runAfter(dwOpenCardTime,bind(&CAndroidUserItemSink::GameTimeOpencard, this));
    return true;
}

bool CAndroidUserItemSink::OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
//     qzpj::CMD_S_OpenCardResult OpenCardResult;
//     if (!OpenCardResult.ParseFromArray(pData,wDataSize))
//         return false;
// 
//     if (OpenCardResult.wopencarduser() == ThisChairId)
//     {
//         ThisThreadTimer->cancel(m_TimerOpenCard);
//     }



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
    ThisThreadTimer->cancel(m_TimerCallBanker);
    ThisThreadTimer->cancel(m_TimerAddScore);
    ThisThreadTimer->cancel(m_TimerOpenCard);
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
    myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/qzpj_config.ini"), szPath);

    TCHAR INI_SECTION_CONFIG[30] = TEXT("");
    myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"), m_pTableFrame->GetGameRoomInfo()->roomId);

    TCHAR KeyName[64] = TEXT("");
    memset(KeyName, 0, sizeof(KeyName));
    {
		//抢庄倍数权值 //////
        for(int i=0;i<4;i++)
        {
            ZeroMemory(KeyName,sizeof(KeyName));
            wsprintf(KeyName, TEXT("QIANGZ_%d"), i );

            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"35,35,15,15", OutBuf, sizeof(OutBuf), szConfigFileName);

            ASSERT(readSize > 0);
            p = OutBuf;
            if(p==nullptr)
                return;
            float temp = 0;
            temp = atoi(p);
            m_qzbeishu[i][0] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_qzbeishu[i][1] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_qzbeishu[i][2] = temp;
            p = mystchr(p, ',') + 1;
            temp = atoi(p);
            m_qzbeishu[i][3] = temp;
        }
    }

    memset(KeyName, 0, sizeof(KeyName));
    {
		//下注倍数权值 //////
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

    sprintf(Filename, "log/qzpj/qzpj_%d%d%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pTableFrame->GetGameRoomInfo()->roomId);
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
