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
#include "public/StdRandom.h"

#include "proto/KPQznn.Message.pb.h"

#include "stdafx.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include "../Game_KPQznnLandy/CMD_Game.h"

using namespace KPQznn;

#include "AndroidUserItemSink.h"

#define ID_TIME_CALL_BANKER				100					//叫庄定时器(3S)
#define ID_TIME_ADD_SCORE				101					//下注定时器(3S)
#define ID_TIME_OPEN_CARD				102					//开牌定时器(5S)

//定时器时间
#define TIME_BASE						1000				//基础定时器

uint32_t	CAndroidUserItemSink::m_cbJettonMultiple[MAX_JETTON_MUL] = { 5,10,15,20 };	//下注倍数
uint32_t	CAndroidUserItemSink::m_cbBankerMultiple[MAX_BANKER_MUL] = { 1,2,4 };		//叫庄倍数



CAndroidUserItemSink::CAndroidUserItemSink()
{
    memset(&m_cardData, 0, sizeof(m_cardData));
    m_wCallBankerNum = 0;
    m_wAddScoreNum = 0;
    m_wOpenCardNum = 0;
    m_wPlayerCount = 0;
    randomRound=0;
    ZeroMemory(openCardTime,sizeof(openCardTime));
    CountrandomRound=0;
    srand(time(NULL));
    ClearAndroidCount=0;
    m_AndroidPlayerCount=RandomInt(10,20);
    m_icishu=0;
    ZeroMemory(m_qzbeishu,sizeof(m_qzbeishu));
    ZeroMemory(m_jiabeibeishu,sizeof(m_jiabeibeishu));
    m_m_cbPlayStatus = false;
    IswinforPlayer=0;

    //抢庄阶段机器人抢庄概率分布表,牌大0，牌小1
     ZeroMemory(RobZhuangProbability0,sizeof(RobZhuangProbability0));
     ZeroMemory(RobZhuangProbability1,sizeof(RobZhuangProbability1));
     ZeroMemory(RobZhuangProbability2,sizeof(RobZhuangProbability2));
     ZeroMemory(RobZhuangProbability4,sizeof(RobZhuangProbability4));
    //庄家叫1倍下注阶段机器人下注概率分布,牌大0，牌小1
     ZeroMemory(Bank1RobotBetProbability5,sizeof(Bank1RobotBetProbability5));
     ZeroMemory(Bank1RobotBetProbability10,sizeof(Bank1RobotBetProbability10));
     ZeroMemory(Bank1RobotBetProbability15,sizeof(Bank1RobotBetProbability15));
     ZeroMemory(Bank1RobotBetProbability20,sizeof(Bank1RobotBetProbability20));
    //庄家叫2倍下注阶段机器人下注概率分布,牌大0，牌小1

     ZeroMemory(Bank2RobotBetProbability5,sizeof(Bank2RobotBetProbability5));
     ZeroMemory(Bank2RobotBetProbability10,sizeof(Bank2RobotBetProbability10));
     ZeroMemory(Bank2RobotBetProbability15,sizeof(Bank2RobotBetProbability15));
     ZeroMemory(Bank2RobotBetProbability20,sizeof(Bank2RobotBetProbability20));
    //庄家叫4倍下注阶段机器人下注概率分布,牌大0，牌小1

     ZeroMemory(Bank4RobotBetProbability5,sizeof(Bank4RobotBetProbability5));
     ZeroMemory(Bank4RobotBetProbability10,sizeof(Bank4RobotBetProbability10));
     ZeroMemory(Bank4RobotBetProbability15,sizeof(Bank4RobotBetProbability15));
     ZeroMemory(Bank4RobotBetProbability20,sizeof(Bank4RobotBetProbability20));
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{

}

bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
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

//        // try to update the specdial room config data item.
        m_pGameRoomKind = m_pTableFrame->GetGameRoomInfo();
        if (m_pGameRoomKind) 
        {
            m_szRoomName = "GameServer_"+m_pGameRoomKind->roomName;
        }


        ReadConfigInformation();
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
       m_wChairID=pUserItem->GetChairId();
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

void CAndroidUserItemSink::GameTimerCallBanker()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerCallBanker);
    //LOG(WARNING) << "android user call banker, user: " << m_wChairID;

    uint8 cbCallBankerMul=0;

    //是特殊牌型
    if(GetSpecialValue(m_cardData))
    {
        //IswinforPlayer---0是输，1是赢
        int jieudan1=RobZhuangProbability0[IswinforPlayer][11];
        int jieudan2=RobZhuangProbability0[IswinforPlayer][11]+ RobZhuangProbability1[IswinforPlayer][11];
        int jieudan3=RobZhuangProbability0[IswinforPlayer][11]+RobZhuangProbability1[IswinforPlayer][11]+ RobZhuangProbability2[IswinforPlayer][11]
                ;
        int jieudan4=RobZhuangProbability0[IswinforPlayer][11]+RobZhuangProbability1[IswinforPlayer][11]+ RobZhuangProbability2[IswinforPlayer][11]
                + RobZhuangProbability4[IswinforPlayer][11];
        int ran=m_random.betweenInt(0,100).randInt_mt(true);
        //概率命中
        if(ran<=jieudan1)
        {
            cbCallBankerMul=0;
        }
        else if(ran>jieudan1&&ran<=jieudan2)
        {
            cbCallBankerMul=1;
        }
        else if(ran>jieudan2&&ran<=jieudan3)
        {
            cbCallBankerMul=2;
        }
        else
        {
            cbCallBankerMul=4;
        }
    }
    else
    {
        int ran=m_random.betweenInt(0,100).randInt_mt(true);
        //无牛到牛牛
        int cardvalue=GetCardType(m_cardData,4);

        int jieudan1=RobZhuangProbability0[IswinforPlayer][cardvalue];
        int jieudan2=RobZhuangProbability0[IswinforPlayer][cardvalue]+ RobZhuangProbability1[IswinforPlayer][cardvalue];
        int jieudan3=RobZhuangProbability0[IswinforPlayer][cardvalue]+RobZhuangProbability1[IswinforPlayer][cardvalue]+ RobZhuangProbability2[IswinforPlayer][cardvalue]
                ;
        int jieudan4=RobZhuangProbability0[IswinforPlayer][cardvalue]+RobZhuangProbability1[IswinforPlayer][cardvalue]+ RobZhuangProbability2[IswinforPlayer][cardvalue]
                + RobZhuangProbability4[IswinforPlayer][cardvalue];
        if(ran<=jieudan1)
        {
            cbCallBankerMul=0;
        }
        else if(ran>jieudan1&&ran<=jieudan2)
        {
            cbCallBankerMul=1;
        }
        else if(ran>jieudan2&&ran<=jieudan3)
        {
            cbCallBankerMul=2;
        }
        else
        {
            cbCallBankerMul=4;
        }

    }


    NN_CMD_C_CallBanker callBanker;
    callBanker.set_cbcallflag(cbCallBankerMul);


    //发送信息
    string content = callBanker.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),NN_SUB_C_CALL_BANKER, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send NN_SUB_C_CALL_BANKER";
    LOG(INFO)<<"******************************cbCallBankerMul"<<(int)cbCallBankerMul;
}
void CAndroidUserItemSink::GameTimeAddScore()
{
    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    NN_CMD_C_AddScore addScore;

    //IswinforPlayer---0是输，1是赢
    int cbCallBankerMul=0;
    if(bankerCall>=1&&bankerCall<2)
    {
        //是特殊牌型
        if(GetSpecialValue(m_cardData))
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //概率命中
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank1RobotBetProbability5[IswinforPlayer][11];
            int jieudan2=Bank1RobotBetProbability5[IswinforPlayer][11]+ Bank1RobotBetProbability10[IswinforPlayer][11];
            int jieudan3=Bank1RobotBetProbability5[IswinforPlayer][11]+ Bank1RobotBetProbability10[IswinforPlayer][11]
                       + Bank1RobotBetProbability15[IswinforPlayer][11];
            int jieudan4=Bank1RobotBetProbability5[IswinforPlayer][11]+ Bank1RobotBetProbability10[IswinforPlayer][11]
                    + Bank1RobotBetProbability15[IswinforPlayer][11]+   Bank1RobotBetProbability20[IswinforPlayer][11];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }
        }
        else
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //无牛到牛牛
            int cardvalue=GetCardType(m_cardData,4);

            if(cardvalue<0||cardvalue>11)
            {
                //出现错误牌型
                return;
            }
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank1RobotBetProbability5[IswinforPlayer][cardvalue];
            int jieudan2=Bank1RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank1RobotBetProbability10[IswinforPlayer][cardvalue];
            int jieudan3=Bank1RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank1RobotBetProbability10[IswinforPlayer][cardvalue]
                       + Bank1RobotBetProbability15[IswinforPlayer][cardvalue];
            int jieudan4=Bank1RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank1RobotBetProbability10[IswinforPlayer][cardvalue]
                    + Bank1RobotBetProbability15[IswinforPlayer][cardvalue]+   Bank1RobotBetProbability20[IswinforPlayer][cardvalue];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }

        }
    }
    else if(bankerCall>=2&&bankerCall<4)
    {
        //是特殊牌型
        if(GetSpecialValue(m_cardData))
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //概率命中
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank2RobotBetProbability5[IswinforPlayer][11];
            int jieudan2=Bank2RobotBetProbability5[IswinforPlayer][11]+ Bank2RobotBetProbability10[IswinforPlayer][11];
            int jieudan3=Bank2RobotBetProbability5[IswinforPlayer][11]+ Bank2RobotBetProbability10[IswinforPlayer][11]
                       + Bank2RobotBetProbability15[IswinforPlayer][11];
            int jieudan4=Bank2RobotBetProbability5[IswinforPlayer][11]+ Bank2RobotBetProbability10[IswinforPlayer][11]
                       + Bank2RobotBetProbability15[IswinforPlayer][11]+Bank2RobotBetProbability20[IswinforPlayer][11];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }
        }
        else
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //无牛到牛牛
            int cardvalue=GetCardType(m_cardData,4);

            if(cardvalue<0||cardvalue>11)
            {
                //出现错误牌型
                return;
            }
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank2RobotBetProbability5[IswinforPlayer][cardvalue];
            int jieudan2=Bank2RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank2RobotBetProbability10[IswinforPlayer][cardvalue];
            int jieudan3=Bank2RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank2RobotBetProbability10[IswinforPlayer][cardvalue]
                       + Bank2RobotBetProbability15[IswinforPlayer][cardvalue];
            int jieudan4=Bank2RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank2RobotBetProbability10[IswinforPlayer][cardvalue]
                       + Bank2RobotBetProbability15[IswinforPlayer][cardvalue]+Bank2RobotBetProbability20[IswinforPlayer][cardvalue];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }

        }
    }
    else
    {
        //是特殊牌型
        if(GetSpecialValue(m_cardData))
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //概率命中
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank4RobotBetProbability5[IswinforPlayer][11];
            int jieudan2=Bank4RobotBetProbability5[IswinforPlayer][11]+ Bank4RobotBetProbability10[IswinforPlayer][11];
            int jieudan3=Bank4RobotBetProbability5[IswinforPlayer][11]+ Bank4RobotBetProbability10[IswinforPlayer][11]
                       + Bank4RobotBetProbability15[IswinforPlayer][11];
            int jieudan4=Bank4RobotBetProbability5[IswinforPlayer][11]+ Bank4RobotBetProbability10[IswinforPlayer][11]
                       + Bank4RobotBetProbability15[IswinforPlayer][11]+Bank4RobotBetProbability20[IswinforPlayer][11];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }
        }
        else
        {
            int ran=m_random.betweenInt(0,100).randInt_mt(true);
            //无牛到牛牛
            int cardvalue=GetCardType(m_cardData,4);

            if(cardvalue<0||cardvalue>11)
            {
                //出现错误牌型
                return;
            }
            if(IswinforPlayer<0||IswinforPlayer>1)
            {
                //出现错误数据
                return;
            }
            int jieudan1=Bank4RobotBetProbability5[IswinforPlayer][cardvalue];
            int jieudan2=Bank4RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank4RobotBetProbability10[IswinforPlayer][cardvalue];
            int jieudan3=Bank4RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank4RobotBetProbability10[IswinforPlayer][cardvalue]
                       + Bank4RobotBetProbability15[IswinforPlayer][cardvalue];
            int jieudan4=Bank4RobotBetProbability5[IswinforPlayer][cardvalue]+ Bank4RobotBetProbability10[IswinforPlayer][cardvalue]
                       + Bank4RobotBetProbability15[IswinforPlayer][cardvalue]+Bank4RobotBetProbability20[IswinforPlayer][cardvalue];
            if(ran<=jieudan1)
            {
                cbCallBankerMul=0;
            }
            else if(ran>jieudan1&&ran<=jieudan2)
            {
                cbCallBankerMul=1;
            }
            else if(ran>jieudan2&&ran<=jieudan3)
            {
                cbCallBankerMul=2;
            }
            else if(ran>jieudan3&&ran<=jieudan4)
            {
                cbCallBankerMul=3;
            }

        }
    }


    addScore.set_cbjettonindex( cbCallBankerMul);
    string content = addScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_pAndroidUserItem->GetChairId(),NN_SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
    LOG(INFO)<<"******************************Send NN_SUB_C_ADD_SCORE";
    LOG(INFO)<<"******************************cbJettonIndex"<<(int)cbCallBankerMul;
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
        KPQznn::CMD_S_GameStartAi GameStartAi;
        if (!GameStartAi.ParseFromArray(pData,wDataSize))
            return false;
        m_cbOxCardData = GameStartAi.cboxcarddata();
        m_isMaxCard = GameStartAi.ismaxcard();
        IswinforPlayer= GameStartAi.ismaxcard();
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
    case NN_SUB_S_CALL_BANKER:
    {
        return OnSubCallBanker(wSubCmdID,pData, wDataSize);
    }
        break;
    case NN_SUB_S_CALL_BANKER_RESULT:
    {
        return OnSubCallBankerResult(wSubCmdID,pData, wDataSize);
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

    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&BankerMultiple= GSFree.cbcallbankermultiple();
    for(int i =0;i<BankerMultiple.size();i++)
        m_cbBankerMultiple[i] = BankerMultiple[i];


    return true;
}
bool CAndroidUserItemSink::GetSpecialValue(BYTE m_cardData[])
{
    //计算前四张牌是不是可以组成五小牛
    bool IswuXiaoNiu=false;
    bool IszhaDanNiu=false;
    bool IsjinNiu=false;
    bool IsyinNiu=false;


    int wuXiaoNiuValue=0;
    int cardType = 0;
    //四张牌是否存在五小牛可能
    for(int i=0;i<4;i++)
    {
        if(GetCardValue( m_cardData[i])<=5)
        {
            wuXiaoNiuValue+=GetCardValue( m_cardData[i]);
            IswuXiaoNiu=true;
        }
        else
        {
            IswuXiaoNiu=false;
            break;
        }
    }
    if(IswuXiaoNiu&&IswuXiaoNiu<=9)
    {
        IswuXiaoNiu=true;
    }
    else
    {
        IswuXiaoNiu=false;
    }
    //四张牌是否存在炸弹可能
    if(GetCardValue( m_cardData[0])==GetCardValue( m_cardData[1])==GetCardValue( m_cardData[2])==GetCardValue( m_cardData[3]))
    {
        IszhaDanNiu=true;
    }
    //四张牌是否存在五花牛可能
    int jisu=0;
    for(int i=0;i<4;i++)
    {
        if(GetCardValue( m_cardData[i])<=10)
        {
            IsjinNiu=false;
            break;
        }
        jisu++;
    }
    if(jisu==4)
    {
         IsjinNiu=true;
    }
    //四张牌是否存在银牛
    int huashu=0;
    bool is10=false;
    for(int i=0;i<4;i++)
    {
        if(GetCardValue( m_cardData[i])>10)
        {
            huashu++;
        }
        else
        {
            if(m_cardData[i]==10)
                is10=true;
        }
    }
    if(huashu==3&&is10)
    {
        IsyinNiu=true;
    }
    if(!IswuXiaoNiu&&!IszhaDanNiu&&!IsjinNiu&&!IsyinNiu)
    {
        return false;
    }
    else
    {
        return true;
    }
}
//逻辑数值
uint8_t CAndroidUserItemSink::GetLogicValue(uint8_t cbCardData)
{
    //扑克属性
    uint8_t bCardValue = GetCardValue(cbCardData);

    //转换数值
    return (bCardValue > 10) ? (10) : bCardValue;
}
//获取类型
uint8_t CAndroidUserItemSink::GetCardType(uint8_t cbCardData[], uint8_t cbCardCount)
{

    //牛1-10
    uint8_t bTemp[MAX_COUNT];
    uint8_t bSum = 0;
    for (uint8_t i = 0; i < cbCardCount; ++i)
    {
        bTemp[i] = GetLogicValue(cbCardData[i]);
        bSum += bTemp[i];
    }
    for (uint8_t i = 0; i < cbCardCount ; i++)
    {
        if ((bSum - bTemp[i] ) % 10 == 0)
        {
            return bTemp[i]>10?bTemp[i]-10:bTemp[i];
        }
    }

    return OX_VALUE0;	//无牛
}
uint8_t CAndroidUserItemSink::GetCardValueDian(BYTE m_cardData[])
{
    if(GetSpecialValue(m_cardData))
    {
        //特殊牌型
        return -1;
    }
    else
    {
        int valueCardx=0 ;
        for(int i=0;i<4;i++)
        {
            valueCardx+=GetCardValue(m_cardData[i]);
        }
        return valueCardx%10;
    }
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
    int callTime = GameStart.cbcallbankertime();
    int dwCallBankerTime = dwShowTime + rand() % (callTime -1);//((callTime -1)*TIME_BASE);

    for(int i=0;i<(int)GameStart.cbhandcard_size();i++)
    {
        m_cardData[i]=GameStart.cbhandcard(i);
    }


    m_TimerCallBanker = m_TimerLoopThread->getLoop()->runAfter(dwCallBankerTime, boost::bind(&CAndroidUserItemSink::GameTimerCallBanker, this));
    LOG(INFO)<<"dwCallBankerTime"<<"="<<dwCallBankerTime;

    //LOG(WARNING) << "set call banker time, time: " << dwCallBankerTime << ", chair id: " << m_wChairID;
    return true;
}

bool CAndroidUserItemSink::OnSubCallBanker(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_CallBanker CallBanker;
    if (!CallBanker.ParseFromArray(pData,wDataSize))
        return false;

    //自己已经叫庄 则检测定时器是否还存在
    if (CallBanker.wcallbankeruser() == m_wChairID)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerCallBanker);
        m_wCallBankerNum++;
    }


    return true;
}

bool CAndroidUserItemSink::OnSubCallBankerResult(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_CallBankerResult BankerResult;
    if (!BankerResult.ParseFromArray(pData,wDataSize))
        return false;


    if (BankerResult.dwbankeruser() != m_wChairID && m_m_cbPlayStatus)
    {
        int dwShowTime = 3;//抢庄动画  3.0
        int jettontime = BankerResult.cbaddjettontime();
        int bankerid=BankerResult.dwbankeruser();
        bankerCall = BankerResult.dwbankmuticalutical();
        float dwAddScore = dwShowTime + rand() % (jettontime - dwShowTime);//((jettontime - 1)* TIME_BASE);
        dwAddScore+=m_random.betweenFloat(0.01,0.99).randFloat_mt(true)-1;
        m_TimerAddScore = m_TimerLoopThread->getLoop()->runAfter(dwAddScore, boost::bind(&CAndroidUserItemSink::GameTimeAddScore, this));
    }

    return true;
}

bool CAndroidUserItemSink::OnSubAddScore(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_AddScoreResult AddScoreResult;
    if (!AddScoreResult.ParseFromArray(pData,wDataSize))
        return false;

    if (AddScoreResult.waddjettonuser()== m_wChairID && m_m_cbPlayStatus)
    {
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

    //int dwShowTime = 2;//抢庄动画  3.0
    //int opentime = SendCard.cbopentime();
    //int dwOpenCardTime = dwShowTime + rand() % (opentime - 1);//((opentime - 1)* TIME_BASE);
    //m_TimerOpenCard = m_TimerLoopThread->getLoop()->runAfter(dwOpenCardTime,bind(&CAndroidUserItemSink::GameTimeOpencard, this));

    //if(!user->IsAndroidUser())
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
        int sum = 0;
        int index=0;
        int shijian[5]{2,4,6,8,10};
        for (int i = 0; i < 5; ++i) {
            sum += openCardTime[i];
        }
        if (sum <= 1) {
            index = 0;
        }
        int r = m_random.betweenInt(1,sum).randInt_mt(true) ,c = r;
        for (int i = 0; i < 5; ++i) {
            c -= openCardTime[i];
            if (c <= 0) {
                index = i;
                break;
            }
        }
        float timemer=shijian[index];
        timemer=m_random.betweenFloat(timemer-1,timemer).randFloat_mt(true);
        m_TimerOpenCard = m_TimerLoopThread->getLoop()->runAfter(timemer,bind(&CAndroidUserItemSink::GameTimeOpencard, this));
    }
    return true;
}

bool CAndroidUserItemSink::OnSubOpenCard(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    NN_CMD_S_OpenCardResult OpenCardResult;
    if (!OpenCardResult.ParseFromArray(pData,wDataSize))
        return false;

    if (OpenCardResult.wopencarduser() == m_wChairID)
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
    }
    shared_ptr<CServerUserItem> user = m_pTableFrame->GetTableUserItem(OpenCardResult.wopencarduser());

    if(!user->IsAndroidUser())
    {
        m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
        int sum = 0;
        int index=0;
        int shijian[5]{2,4,6,8,10};
        for (int i = 0; i < 5; ++i) {
            sum += openCardTime[i];
        }
        if (sum <= 1) {
            index = 0;
        }
        int r = m_random.betweenInt(1,sum).randInt_mt(true) ,c = r;
        for (int i = 0; i < 5; ++i) {
            c -= openCardTime[i];
            if (c <= 0) {
                index = i;
                break;
            }
        }
        float timemer=shijian[index];
        timemer=m_random.betweenFloat(timemer-1,timemer).randFloat_mt(true);
        m_TimerOpenCard = m_TimerLoopThread->getLoop()->runAfter(timemer,bind(&CAndroidUserItemSink::GameTimeOpencard, this));
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

    m_TimerLoopThread->getLoop()->cancel(m_TimerCallBanker);
    m_TimerLoopThread->getLoop()->cancel(m_TimerAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_TimerOpenCard);
    return true;
}
//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/kpqznn_config.ini"))
    {
        LOG(INFO) << "./conf/kpqznn_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/kpqznn_config.ini", pt);

    openCardTime[0]=pt.get<int64_t>("GAME_CONFIG.OPEN_CARDTIME_1-2",3);
    openCardTime[1]=pt.get<int64_t>("GAME_CONFIG.OPEN_CARDTIME_3-4",3);
    openCardTime[2]=pt.get<int64_t>("GAME_CONFIG.OPEN_CARDTIME_5-6",3);
    openCardTime[3]=pt.get<int64_t>("GAME_CONFIG.OPEN_CARDTIME_7-8",3);
    openCardTime[4]=pt.get<int64_t>("GAME_CONFIG.OPEN_CARDTIME_9-10",3);
    //抢庄阶段 不抢 机器人抢庄概率分布表,牌大0，牌大1===0
    RobZhuangProbability0[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_0", 0);
    RobZhuangProbability0[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_1", 1);
    RobZhuangProbability0[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_2", 2);
    RobZhuangProbability0[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_3", 3);
    RobZhuangProbability0[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_4", 5);
    RobZhuangProbability0[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_5", 10);
    RobZhuangProbability0[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_6", 10);
    RobZhuangProbability0[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_7", 10);
    RobZhuangProbability0[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_8", 10);
    RobZhuangProbability0[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_9", 10);
    RobZhuangProbability0[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_10", 10);
    RobZhuangProbability0[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M0_RXBPKTYPE_11", 10);

    //抢庄阶段 不抢 机器人抢庄概率分布表,牌大0，牌小1===1
    RobZhuangProbability0[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_0", 0);
    RobZhuangProbability0[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_1", 1);
    RobZhuangProbability0[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_2", 2);
    RobZhuangProbability0[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_3", 3);
    RobZhuangProbability0[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_4", 5);
    RobZhuangProbability0[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_5", 10);
    RobZhuangProbability0[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_6", 10);
    RobZhuangProbability0[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_7", 10);
    RobZhuangProbability0[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_8", 10);
    RobZhuangProbability0[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_9", 10);
    RobZhuangProbability0[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_10", 10);
    RobZhuangProbability0[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M0_RXBPKTYPE_11", 10);
    //抢庄阶段 抢一倍 机器人抢庄概率分布表,牌大0，牌小1===0
    RobZhuangProbability1[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_0", 0);
    RobZhuangProbability1[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_1", 1);
    RobZhuangProbability1[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_2", 2);
    RobZhuangProbability1[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_3", 3);
    RobZhuangProbability1[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_4", 5);
    RobZhuangProbability1[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_5", 10);
    RobZhuangProbability1[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_6", 10);
    RobZhuangProbability1[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_7", 10);
    RobZhuangProbability1[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_8", 10);
    RobZhuangProbability1[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_9", 10);
    RobZhuangProbability1[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_10", 10);
    RobZhuangProbability1[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M1_RXBPKTYPE_11", 10);
    //抢庄阶段 抢一倍 机器人抢庄概率分布表,牌大0，牌小1===1
    RobZhuangProbability1[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_0", 0);
    RobZhuangProbability1[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_1", 1);
    RobZhuangProbability1[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_2", 2);
    RobZhuangProbability1[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_3", 3);
    RobZhuangProbability1[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_4", 5);
    RobZhuangProbability1[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_5", 10);
    RobZhuangProbability1[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_6", 10);
    RobZhuangProbability1[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_7", 10);
    RobZhuangProbability1[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_8", 10);
    RobZhuangProbability1[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_9", 10);
    RobZhuangProbability1[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_10", 10);
    RobZhuangProbability1[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M1_RXBPKTYPE_11", 10);
    //抢庄阶段 抢二倍  机器人抢庄概率分布表,牌大0，牌小1===1
    RobZhuangProbability2[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_0", 0);
    RobZhuangProbability2[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_1", 1);
    RobZhuangProbability2[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_2", 2);
    RobZhuangProbability2[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_3", 3);
    RobZhuangProbability2[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_4", 5);
    RobZhuangProbability2[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_5", 10);
    RobZhuangProbability2[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_6", 10);
    RobZhuangProbability2[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_7", 10);
    RobZhuangProbability2[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_8", 10);
    RobZhuangProbability2[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_9", 10);
    RobZhuangProbability2[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_10", 10);
    RobZhuangProbability2[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M2_RXBPKTYPE_11", 10);
    //抢庄阶段 抢二倍  机器人抢庄概率分布表,牌大0，牌小1
    RobZhuangProbability2[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_0", 0);
    RobZhuangProbability2[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_1", 1);
    RobZhuangProbability2[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_2", 2);
    RobZhuangProbability2[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_3", 3);
    RobZhuangProbability2[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_4", 5);
    RobZhuangProbability2[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_5", 10);
    RobZhuangProbability2[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_6", 10);
    RobZhuangProbability2[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_7", 10);
    RobZhuangProbability2[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_8", 10);
    RobZhuangProbability2[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_9", 10);
    RobZhuangProbability2[0][10] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_10", 10);
    RobZhuangProbability2[0][11] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M2_RXBPKTYPE_11", 10);
    //抢庄阶段 抢四倍  机器人抢庄概率分布表,牌大0，牌小1===1
    RobZhuangProbability4[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_0", 0);
    RobZhuangProbability4[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_1", 1);
    RobZhuangProbability4[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_2", 2);
    RobZhuangProbability4[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_3", 3);
    RobZhuangProbability4[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_4", 5);
    RobZhuangProbability4[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_5", 10);
    RobZhuangProbability4[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_6", 10);
    RobZhuangProbability4[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_7", 10);
    RobZhuangProbability4[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_8", 10);
    RobZhuangProbability4[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_9", 10);
    RobZhuangProbability4[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_10", 10);
    RobZhuangProbability4[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_DA.M4_RXBPKTYPE_11", 10);
    //抢庄阶段 抢四倍  机器人抢庄概率分布表,牌大0，牌小1
    RobZhuangProbability4[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_0", 0);
    RobZhuangProbability4[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_1", 1);
    RobZhuangProbability4[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_2", 2);
    RobZhuangProbability4[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_3", 3);
    RobZhuangProbability4[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_4", 5);
    RobZhuangProbability4[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_5", 10);
    RobZhuangProbability4[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_6", 10);
    RobZhuangProbability4[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_7", 10);
    RobZhuangProbability4[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_8", 10);
    RobZhuangProbability4[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_9", 10);
    RobZhuangProbability4[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_10", 10);
    RobZhuangProbability4[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_QAINGZHUANG_XIAO.M4_RXBPKTYPE_11", 10);


    //加注阶段 一倍庄  机器人抢庄概率分布表,牌大0，牌小1
    Bank1RobotBetProbability5[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_0", 0);
    Bank1RobotBetProbability5[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_1", 1);
    Bank1RobotBetProbability5[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_2", 2);
    Bank1RobotBetProbability5[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_3", 3);
    Bank1RobotBetProbability5[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_4", 5);
    Bank1RobotBetProbability5[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_5", 10);
    Bank1RobotBetProbability5[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_6", 10);
    Bank1RobotBetProbability5[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_7", 10);
    Bank1RobotBetProbability5[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_8", 10);
    Bank1RobotBetProbability5[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_9", 10);
    Bank1RobotBetProbability5[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_10", 10);
    Bank1RobotBetProbability5[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M5BET_11", 10);

    Bank1RobotBetProbability5[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_0", 0);
    Bank1RobotBetProbability5[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_1", 1);
    Bank1RobotBetProbability5[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_2", 2);
    Bank1RobotBetProbability5[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_3", 3);
    Bank1RobotBetProbability5[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_4", 5);
    Bank1RobotBetProbability5[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_5", 10);
    Bank1RobotBetProbability5[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_6", 10);
    Bank1RobotBetProbability5[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_7", 10);
    Bank1RobotBetProbability5[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_8", 10);
    Bank1RobotBetProbability5[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_9", 10);
    Bank1RobotBetProbability5[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_10", 10);
    Bank1RobotBetProbability5[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M5BET_11", 10);

    Bank1RobotBetProbability10[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_0", 0);
    Bank1RobotBetProbability10[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_1", 1);
    Bank1RobotBetProbability10[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_2", 2);
    Bank1RobotBetProbability10[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_3", 3);
    Bank1RobotBetProbability10[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_4", 5);
    Bank1RobotBetProbability10[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_5", 10);
    Bank1RobotBetProbability10[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_6", 10);
    Bank1RobotBetProbability10[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_7", 10);
    Bank1RobotBetProbability10[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_8", 10);
    Bank1RobotBetProbability10[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_9", 10);
    Bank1RobotBetProbability10[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_10", 10);
    Bank1RobotBetProbability10[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M10BET_11", 10);

    Bank1RobotBetProbability10[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_0", 0);
    Bank1RobotBetProbability10[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_1", 1);
    Bank1RobotBetProbability10[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_2", 2);
    Bank1RobotBetProbability10[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_3", 3);
    Bank1RobotBetProbability10[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_4", 5);
    Bank1RobotBetProbability10[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_5", 10);
    Bank1RobotBetProbability10[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_6", 10);
    Bank1RobotBetProbability10[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_7", 10);
    Bank1RobotBetProbability10[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_8", 10);
    Bank1RobotBetProbability10[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_9", 10);
    Bank1RobotBetProbability10[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_10", 10);
    Bank1RobotBetProbability10[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M10BET_11", 10);

    Bank1RobotBetProbability15[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_0", 0);
    Bank1RobotBetProbability15[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_1", 1);
    Bank1RobotBetProbability15[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_2", 2);
    Bank1RobotBetProbability15[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_3", 3);
    Bank1RobotBetProbability15[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_4", 5);
    Bank1RobotBetProbability15[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_5", 10);
    Bank1RobotBetProbability15[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_6", 10);
    Bank1RobotBetProbability15[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_7", 10);
    Bank1RobotBetProbability15[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_8", 10);
    Bank1RobotBetProbability15[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_9", 10);
    Bank1RobotBetProbability15[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_10", 10);
    Bank1RobotBetProbability15[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M15BET_11", 10);



    Bank1RobotBetProbability15[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_0", 0);
    Bank1RobotBetProbability15[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_1", 1);
    Bank1RobotBetProbability15[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_2", 2);
    Bank1RobotBetProbability15[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_3", 3);
    Bank1RobotBetProbability15[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_4", 5);
    Bank1RobotBetProbability15[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_5", 10);
    Bank1RobotBetProbability15[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_6", 10);
    Bank1RobotBetProbability15[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_7", 10);
    Bank1RobotBetProbability15[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_8", 10);
    Bank1RobotBetProbability15[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_9", 10);
    Bank1RobotBetProbability15[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_10", 10);
    Bank1RobotBetProbability15[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M15BET_11", 10);

    Bank1RobotBetProbability20[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_0", 0);
    Bank1RobotBetProbability20[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_1", 1);
    Bank1RobotBetProbability20[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_2", 2);
    Bank1RobotBetProbability20[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_3", 3);
    Bank1RobotBetProbability20[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_4", 5);
    Bank1RobotBetProbability20[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_5", 10);
    Bank1RobotBetProbability20[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_6", 10);
    Bank1RobotBetProbability20[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_7", 10);
    Bank1RobotBetProbability20[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_8", 10);
    Bank1RobotBetProbability20[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_9", 10);
    Bank1RobotBetProbability20[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_10", 10);
    Bank1RobotBetProbability20[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_DA.M20BET_11", 10);

    Bank1RobotBetProbability20[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_0", 0);
    Bank1RobotBetProbability20[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_1", 1);
    Bank1RobotBetProbability20[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_2", 2);
    Bank1RobotBetProbability20[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_3", 3);
    Bank1RobotBetProbability20[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_4", 5);
    Bank1RobotBetProbability20[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_5", 10);
    Bank1RobotBetProbability20[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_6", 10);
    Bank1RobotBetProbability20[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_7", 10);
    Bank1RobotBetProbability20[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_8", 10);
    Bank1RobotBetProbability20[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_9", 10);
    Bank1RobotBetProbability20[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_10", 10);
    Bank1RobotBetProbability20[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_1_XIAO.M20BET_11", 10);

    //加注阶段 一倍庄  机器人抢庄概率分布表,牌大0，牌小1
    Bank2RobotBetProbability5[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_0", 0);
    Bank2RobotBetProbability5[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_1", 1);
    Bank2RobotBetProbability5[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_2", 2);
    Bank2RobotBetProbability5[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_3", 3);
    Bank2RobotBetProbability5[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_4", 5);
    Bank2RobotBetProbability5[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_5", 10);
    Bank2RobotBetProbability5[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_6", 10);
    Bank2RobotBetProbability5[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_7", 10);
    Bank2RobotBetProbability5[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_8", 10);
    Bank2RobotBetProbability5[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_9", 10);
    Bank2RobotBetProbability5[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_10", 10);
    Bank2RobotBetProbability5[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M5BET_11", 10);

    Bank2RobotBetProbability5[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_0", 0);
    Bank2RobotBetProbability5[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_1", 1);
    Bank2RobotBetProbability5[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_2", 2);
    Bank2RobotBetProbability5[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_3", 3);
    Bank2RobotBetProbability5[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_4", 5);
    Bank2RobotBetProbability5[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_5", 10);
    Bank2RobotBetProbability5[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_6", 10);
    Bank2RobotBetProbability5[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_7", 10);
    Bank2RobotBetProbability5[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_8", 10);
    Bank2RobotBetProbability5[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_9", 10);
    Bank2RobotBetProbability5[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_10", 10);
    Bank2RobotBetProbability5[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M5BET_11", 10);

    Bank2RobotBetProbability10[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_0", 0);
    Bank2RobotBetProbability10[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_1", 1);
    Bank2RobotBetProbability10[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_2", 2);
    Bank2RobotBetProbability10[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_3", 3);
    Bank2RobotBetProbability10[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_4", 5);
    Bank2RobotBetProbability10[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_5", 10);
    Bank2RobotBetProbability10[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_6", 10);
    Bank2RobotBetProbability10[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_7", 10);
    Bank2RobotBetProbability10[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_8", 10);
    Bank2RobotBetProbability10[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_9", 10);
    Bank2RobotBetProbability10[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_10", 10);
    Bank2RobotBetProbability10[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M10BET_11", 10);

    Bank2RobotBetProbability10[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_0", 0);
    Bank2RobotBetProbability10[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_1", 1);
    Bank2RobotBetProbability10[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_2", 2);
    Bank2RobotBetProbability10[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_3", 3);
    Bank2RobotBetProbability10[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_4", 5);
    Bank2RobotBetProbability10[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_5", 10);
    Bank2RobotBetProbability10[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_6", 10);
    Bank2RobotBetProbability10[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_7", 10);
    Bank2RobotBetProbability10[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_8", 10);
    Bank2RobotBetProbability10[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_9", 10);
    Bank2RobotBetProbability10[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_10", 10);
    Bank2RobotBetProbability10[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M10BET_11", 10);

    Bank2RobotBetProbability15[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_0", 0);
    Bank2RobotBetProbability15[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_1", 1);
    Bank2RobotBetProbability15[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_2", 2);
    Bank2RobotBetProbability15[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_3", 3);
    Bank2RobotBetProbability15[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_4", 5);
    Bank2RobotBetProbability15[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_5", 10);
    Bank2RobotBetProbability15[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_6", 10);
    Bank2RobotBetProbability15[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_7", 10);
    Bank2RobotBetProbability15[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_8", 10);
    Bank2RobotBetProbability15[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_9", 10);
    Bank2RobotBetProbability15[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_10", 10);
    Bank2RobotBetProbability15[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M15BET_11", 10);

    Bank2RobotBetProbability15[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_0", 0);
    Bank2RobotBetProbability15[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_1", 1);
    Bank2RobotBetProbability15[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_2", 2);
    Bank2RobotBetProbability15[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_3", 3);
    Bank2RobotBetProbability15[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_4", 5);
    Bank2RobotBetProbability15[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_5", 10);
    Bank2RobotBetProbability15[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_6", 10);
    Bank2RobotBetProbability15[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_7", 10);
    Bank2RobotBetProbability15[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_8", 10);
    Bank2RobotBetProbability15[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_9", 10);
    Bank2RobotBetProbability15[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_10", 10);
    Bank2RobotBetProbability15[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M15BET_11", 10);

    Bank2RobotBetProbability20[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_0", 0);
    Bank2RobotBetProbability20[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_1", 1);
    Bank2RobotBetProbability20[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_2", 2);
    Bank2RobotBetProbability20[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_3", 3);
    Bank2RobotBetProbability20[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_4", 5);
    Bank2RobotBetProbability20[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_5", 10);
    Bank2RobotBetProbability20[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_6", 10);
    Bank2RobotBetProbability20[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_7", 10);
    Bank2RobotBetProbability20[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_8", 10);
    Bank2RobotBetProbability20[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_9", 10);
    Bank2RobotBetProbability20[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_10", 10);
    Bank2RobotBetProbability20[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_DA.M20BET_11", 10);

    Bank2RobotBetProbability20[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_0", 0);
    Bank2RobotBetProbability20[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_1", 1);
    Bank2RobotBetProbability20[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_2", 2);
    Bank2RobotBetProbability20[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_3", 3);
    Bank2RobotBetProbability20[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_4", 5);
    Bank2RobotBetProbability20[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_5", 10);
    Bank2RobotBetProbability20[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_6", 10);
    Bank2RobotBetProbability20[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_7", 10);
    Bank2RobotBetProbability20[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_8", 10);
    Bank2RobotBetProbability20[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_9", 10);
    Bank2RobotBetProbability20[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_10", 10);
    Bank2RobotBetProbability20[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_2_XIAO.M20BET_11", 10);

    Bank4RobotBetProbability5[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_0", 0);
    Bank4RobotBetProbability5[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_1", 1);
    Bank4RobotBetProbability5[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_2", 2);
    Bank4RobotBetProbability5[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_3", 3);
    Bank4RobotBetProbability5[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_4", 5);
    Bank4RobotBetProbability5[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_5", 10);
    Bank4RobotBetProbability5[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_6", 10);
    Bank4RobotBetProbability5[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_7", 10);
    Bank4RobotBetProbability5[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_8", 10);
    Bank4RobotBetProbability5[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_9", 10);
    Bank4RobotBetProbability5[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_10", 10);
    Bank4RobotBetProbability5[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M5BET_11", 10);

    Bank4RobotBetProbability5[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_0", 0);
    Bank4RobotBetProbability5[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_1", 1);
    Bank4RobotBetProbability5[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_2", 2);
    Bank4RobotBetProbability5[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_3", 3);
    Bank4RobotBetProbability5[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_4", 5);
    Bank4RobotBetProbability5[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_5", 10);
    Bank4RobotBetProbability5[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_6", 10);
    Bank4RobotBetProbability5[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_7", 10);
    Bank4RobotBetProbability5[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_8", 10);
    Bank4RobotBetProbability5[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_9", 10);
    Bank4RobotBetProbability5[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_10", 10);
    Bank4RobotBetProbability5[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M5BET_11", 10);

    Bank4RobotBetProbability10[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_0", 0);
    Bank4RobotBetProbability10[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_1", 1);
    Bank4RobotBetProbability10[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_2", 2);
    Bank4RobotBetProbability10[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_3", 3);
    Bank4RobotBetProbability10[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_4", 5);
    Bank4RobotBetProbability10[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_5", 10);
    Bank4RobotBetProbability10[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_6", 10);
    Bank4RobotBetProbability10[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_7", 10);
    Bank4RobotBetProbability10[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_8", 10);
    Bank4RobotBetProbability10[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_9", 10);
    Bank4RobotBetProbability10[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_10", 10);
    Bank4RobotBetProbability10[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M10BET_11", 10);

    Bank4RobotBetProbability10[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_0", 0);
    Bank4RobotBetProbability10[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_1", 1);
    Bank4RobotBetProbability10[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_2", 2);
    Bank4RobotBetProbability10[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_3", 3);
    Bank4RobotBetProbability10[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_4", 5);
    Bank4RobotBetProbability10[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_5", 10);
    Bank4RobotBetProbability10[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_6", 10);
    Bank4RobotBetProbability10[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_7", 10);
    Bank4RobotBetProbability10[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_8", 10);
    Bank4RobotBetProbability10[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_9", 10);
    Bank4RobotBetProbability10[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_10", 10);
    Bank4RobotBetProbability10[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M10BET_11", 10);


    Bank4RobotBetProbability15[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_0", 0);
    Bank4RobotBetProbability15[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_1", 1);
    Bank4RobotBetProbability15[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_2", 2);
    Bank4RobotBetProbability15[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_3", 3);
    Bank4RobotBetProbability15[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_4", 5);
    Bank4RobotBetProbability15[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_5", 10);
    Bank4RobotBetProbability15[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_6", 10);
    Bank4RobotBetProbability15[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_7", 10);
    Bank4RobotBetProbability15[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_8", 10);
    Bank4RobotBetProbability15[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_9", 10);
    Bank4RobotBetProbability15[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_10", 10);
    Bank4RobotBetProbability15[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M15BET_11", 10);

    Bank4RobotBetProbability15[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_0", 0);
    Bank4RobotBetProbability15[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_1", 1);
    Bank4RobotBetProbability15[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_2", 2);
    Bank4RobotBetProbability15[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_3", 3);
    Bank4RobotBetProbability15[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_4", 5);
    Bank4RobotBetProbability15[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_5", 10);
    Bank4RobotBetProbability15[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_6", 10);
    Bank4RobotBetProbability15[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_7", 10);
    Bank4RobotBetProbability15[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_8", 10);
    Bank4RobotBetProbability15[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_9", 10);
    Bank4RobotBetProbability15[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_10", 10);
    Bank4RobotBetProbability15[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M15BET_11", 10);

    Bank4RobotBetProbability20[1][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_0", 0);
    Bank4RobotBetProbability20[1][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_1", 1);
    Bank4RobotBetProbability20[1][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_2", 2);
    Bank4RobotBetProbability20[1][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_3", 3);
    Bank4RobotBetProbability20[1][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_4", 5);
    Bank4RobotBetProbability20[1][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_5", 10);
    Bank4RobotBetProbability20[1][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_6", 10);
    Bank4RobotBetProbability20[1][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_7", 10);
    Bank4RobotBetProbability20[1][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_8", 10);
    Bank4RobotBetProbability20[1][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_9", 10);
    Bank4RobotBetProbability20[1][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_10", 10);
    Bank4RobotBetProbability20[1][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_DA.M20BET_11", 10);

    Bank4RobotBetProbability20[0][0] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_0", 0);
    Bank4RobotBetProbability20[0][1] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_1", 1);
    Bank4RobotBetProbability20[0][2] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_2", 2);
    Bank4RobotBetProbability20[0][3] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_3", 3);
    Bank4RobotBetProbability20[0][4] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_4", 5);
    Bank4RobotBetProbability20[0][5] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_5", 10);
    Bank4RobotBetProbability20[0][6] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_6", 10);
    Bank4RobotBetProbability20[0][7] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_7", 10);
    Bank4RobotBetProbability20[0][8] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_8", 10);
    Bank4RobotBetProbability20[0][9] = pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_9", 10);
    Bank4RobotBetProbability20[0][10]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_10", 10);
    Bank4RobotBetProbability20[0][11]= pt.get<int64_t>("ANDROID_NEWCONFIG_XIAZHU_4_XIAO.M20BET_11", 10);


}
//读取配置
//void CAndroidUserItemSink::ReadConfigInformation()
//{
//    //设置文件名
//    const TCHAR * p = nullptr;
//    TCHAR szPath[MAX_PATH] = TEXT("");
//    TCHAR szConfigFileName[MAX_PATH] = TEXT("");
//    TCHAR OutBuf[255] = TEXT("");
//    CPathMagic::GetCurrentDirectory(szPath, sizeof(szPath));
//    myprintf(szConfigFileName, sizeof(szConfigFileName), _T("%sconf/kpqznn_config.ini"), szPath);

//    TCHAR INI_SECTION_CONFIG[30] = TEXT("");
//    myprintf(INI_SECTION_CONFIG, sizeof(INI_SECTION_CONFIG), _T("ANDROID_NEWCONFIG_%d"), m_pTableFrame->GetGameRoomInfo()->roomId);

//    TCHAR KeyName[64] = TEXT("");
//    memset(KeyName, 0, sizeof(KeyName));
//    {
//        for(int i=0;i<4;i++)
//        {
//            ZeroMemory(KeyName,sizeof(KeyName));
//            wsprintf(KeyName, TEXT("QIANGZ_%d"), i );

//            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"35,35,15,15", OutBuf, sizeof(OutBuf), szConfigFileName);

//            ASSERT(readSize > 0);
//            p = OutBuf;
//            if(p==nullptr)
//                return;
//            float temp = 0;
//            temp = atoi(p);
//            m_qzbeishu[i][0] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_qzbeishu[i][1] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_qzbeishu[i][2] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_qzbeishu[i][3] = temp;
//        }
//    }

//    memset(KeyName, 0, sizeof(KeyName));
//    {
//        for(int i=0;i<4;i++)
//        {
//            ZeroMemory(KeyName,sizeof(KeyName));
//            wsprintf(KeyName, TEXT("JIAB_%d"), i );

//            int readSize = GetPrivateProfileString(INI_SECTION_CONFIG, KeyName, _T"35,35,15,15", OutBuf, sizeof(OutBuf), szConfigFileName);

//            ASSERT(readSize > 0);
//            p = OutBuf;
//            if(p==nullptr)
//                return;
//            float temp = 0;
//            temp = atoi(p);
//            m_jiabeibeishu[i][0] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_jiabeibeishu[i][1] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_jiabeibeishu[i][2] = temp;
//            p = mystchr(p, ',') + 1;
//            temp = atoi(p);
//            m_jiabeibeishu[i][3] = temp;
//        }
//    }
//    int test = 0;
//    test =1;
//}


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
