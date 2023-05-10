#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

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

#include "proto/ErBaGang.Message.pb.h"

#include "AndroidUserItemSink.h"

//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS                       2                                   //最少时间
#define TIME_DELAY_TIME					3                                   //延时时间

//游戏时间
#define TIME_START_GAME					3                                   //开始时间
#define TIME_USER_ADD_SCORE				7                                   //下注时间

//游戏时间
#define IDI_START_GAME					(100)								//开始定时器
#define IDI_USER_ADD_SCORE				(101)								//下注定时器
#define IDI_GameEnd                     (102)								//结束定时器

//构造函数
CAndroidUserItemSink::CAndroidUserItemSink()
{
	//游戏变量
    m_jionTime = 0;
    m_MaxWinTime = 0;
    m_isShensuanzi= false;
    m_hensuanziMaxArea = 1;
    m_kaishijiangeInit = 2000;
    m_jiangeTimeInit = 3000;
    m_JettonScore = 0;
    m_lianxuxiazhuCount= 0;
    m_end3s = 25;
    m_currentChips[0]=100;
    m_currentChips[1]=500;
    m_currentChips[2]=1000;
    m_currentChips[3]=5000;
    m_currentChips[4]=10000;
    /*m_currentChips[5]=50000;
    m_currentChips[6]=100000;*/
	return;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

/*
//接口查询
void * CAndroidUserItemSink::QueryInterface(REFGUID Guid, int64_t dwQueryVer)
{
	QUERYINTERFACE(IAndroidUserItemSink,Guid,dwQueryVer);
	QUERYINTERFACE_IUNKNOWNEX(IAndroidUserItemSink,Guid,dwQueryVer);
	return NULL;
}
*/

//初始接口
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    LOG(INFO) << "CAndroidUserItemSink::Initialization pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
   // openLog("Initialization。。。");
    bool bSuccess = false;
    do
    {
        //  check the table and user item now.
        if ((!pTableFrame) || (!pUserItem)) {
            LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
            break;
        }

        //查询接口
        // m_pIAndroidUserItem=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,IAndroidUserItem);
        // if (m_pIAndroidUserItem==NULL) return false;
        m_pITableFrame   =  pTableFrame;
        m_pIAndroidUserItem = pUserItem;

        // try to update the specdial room config data item.
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        if(m_pGameRoomInfo)
        {
            m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
        }

        // check if user is android user item.
        if (!pUserItem->IsAndroidUser()) {
            LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserId();
        }

        //查询接口
        ReadConfigInformation();
        bSuccess = true;
    }   while (0);
//Cleanup:
    return (bSuccess);
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{
    m_pITableFrame   =  pTableFrame;
    m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
    if(m_pGameRoomInfo)
    {
     m_strRoomName = "GameServer_" + m_pGameRoomInfo->roomName;
    }
    ReadConfigInformation();
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem> &pUserItem)
{
    m_pIAndroidUserItem = pUserItem;
    if(!pUserItem->IsAndroidUser())
    {
        LOG(INFO) << "pUserItem is not android!, userid:" << pUserItem->GetUserId();
    }
}

//重置接口
bool CAndroidUserItemSink::RepositionSink()
{
    LOG(INFO) << "CAndroidUserItemSink::RepositionSink.";

    return true;
}
void CAndroidUserItemSink::OnTimerJetton()
{
    int64_t wGold = m_pIAndroidUserItem->GetUserScore();
      //金币不足50
      if(wGold<50)
      {
          m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
          return ;
      }
      //下注结束
      //openLog("nowTime=%lld m_timerStart=%lld cha=%lld",time(0),m_timerStart,time(0) - m_timerStart);

      if(m_pIAndroidUserItem->GetUserStatus()==sOffline){
          m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
          return ;
      }

      struct timeval tv;
      gettimeofday(&tv,NULL);
     time_t  NowTime = tv.tv_sec*1000 + tv.tv_usec/1000;
     if(NowTime - m_timerStart > m_endtimer*1000){
         m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
         return ;
        }
     if(NowTime - m_timerStart < (m_endtimer-3)*1000)
     {
         int kaishijiange = rand()%2000+m_kaishijiangeInit;
           if(m_lasttimer== 0 && NowTime - m_timerStart <kaishijiange)
           {
               return ;
           }
           if(m_lasttimer== 0 && NowTime - m_timerStart >=kaishijiange )
           {
               m_lasttimer = NowTime;
               OnSubAddScore();
               return ;
           }

           if(rand()%3==1)
           {
               m_lasttimer = NowTime;
               OnSubAddScore();
               return;
           }
     }
     else if(NowTime - m_timerStart < 300)
     {
         m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
         return ;
     }
     else
     {
         if(rand()%100<= m_end3s)
         {
             m_lasttimer = NowTime;
             OnSubAddScore();
             return ;
         }
     }
}
void CAndroidUserItemSink::OnTimerGameEnd()
{
    m_sendArea.clear();
    //判断是否需要离开
     ExitGame();
    count = 0;
}


//用户加注
bool CAndroidUserItemSink::OnSubAddScore()
{
     int64_t wAddGold = m_startsScore-50;//可下注金额
     //随机可下注金额
     int prob = RandAddProbability();
     int64_t canGold = wAddGold ;//*wAddGold/100;
     //随机下注筹码
     double JettonScore = 0;
     if (m_lianxuxiazhuCount == 0L)
     {
         JettonScore = RandChouMa(canGold,prob);
         m_JettonScore = JettonScore;
         m_lianxuxiazhuCount = rand() % 3 + 2;
     }
     else
     {
         JettonScore = m_JettonScore;
         m_lianxuxiazhuCount--;
     }

     if(JettonScore == 0||JettonScore>canGold)
     {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
        return true;
     }
     //随机下注区域
     int bJettonArea=RandArea();

     bool isfind = false;
     for(int i=0;i<m_sendArea.size();i++)
     {
         if(m_sendArea[i]==bJettonArea)
         {
             isfind=true;
               break;
         }
     }
     if(!isfind)
         m_sendArea.push_back(bJettonArea);

     if( bJettonArea<1 && bJettonArea>8)
         bJettonArea = rand() % 7+1;

//     if(JettonScore>m_pITableFrame->GetGameRoomInfo()->jettons[5] && JettonScore<1)
//     {
//         if (rand() % 100 < 50)
//          {
//               JettonScore = m_pITableFrame->GetGameRoomInfo()->jettons[0];
//               m_JettonScore = JettonScore;
//          }
//          else
//          {
//              JettonScore = m_pITableFrame->GetGameRoomInfo()->jettons[1];
//              m_JettonScore = JettonScore;
//          }
//     }
    m_RoundJettonScore+=JettonScore;
    ErBaGang::CMD_C_PlaceJet mPlaceJet;
    mPlaceJet.set_cbjettonarea(bJettonArea);
    mPlaceJet.set_ljettonscore(JettonScore);

    //openLog("userid =%d  wAddGold = %lld prob =%d canGold =%lld area =%d jettonScore =%f",\
            m_pIAndroidUserItem->GetUserID(),wAddGold,prob,canGold,bJettonArea,JettonScore);
    string content = mPlaceJet.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),ErBaGang::SUB_C_USER_JETTON,(uint8_t*) content.data(), content.size());
    m_RoundJettonCount++;
    if(m_pIAndroidUserItem->GetUserScore()>10000)
    {
        if(m_RoundJettonScore>m_pIAndroidUserItem->GetUserScore()*m_AndroidJettonProb)
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }else if(m_pIAndroidUserItem->GetUserScore()>5000)
    {
        if(m_RoundJettonCount==3)
            m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }
    else
    {
        m_pITableFrame->GetLoopThread()->getLoop()->cancel(m_TimerJetton);
    }
    //LOG(INFO) << " >>> OnSubAddScore SUB_C_PLACE_JETTON";
}

//随机下注区域
int CAndroidUserItemSink::RandArea()
{
    int TotalWeight = 0;
    for(int i=0;i<3;i++)
        TotalWeight +=m_Areaprobability[i];

    int randWeight = rand()%TotalWeight;
    int tempweightsize = 0;
    for (int i = 0; i < 3; i++)
    {
        tempweightsize += m_Areaprobability[i];
        if (randWeight <= tempweightsize)
        {
            if(i==0)
                return 1;//long
            else if(i==1)
                 return 2;//hu
            else if(i==2)
                 return 3;//he
        }
    }
   // openLog("test");
}

//随机下注筹码
double CAndroidUserItemSink::RandChouMa(int64_t CanxiazuGold,int8_t GoldId)
{
    if(!m_currentChips[GoldId])
        GoldId=0;
    uint64_t socre=0;
    uint64_t whileCount=20;
    while(!socre&&whileCount--)
    {
        if(CanxiazuGold>m_currentChips[GoldId])
        {
            CanxiazuGold=RandAddProbability();
        }else
            socre=m_currentChips[GoldId];
    }
    return socre;

}
//随机可下注金额比例
int CAndroidUserItemSink::RandAddProbability()
{
    uint32_t TotalWeight = 0;
    for(int i = 0; i < 5; ++i)
        TotalWeight += m_probabilityWeight[i];

    if(TotalWeight==0) TotalWeight=1;
    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    int goldId=0;
    for (int i = 0; i < 5; ++i)
    {
        tempweightsize += m_probabilityWeight[i];
        if (randWeight <= tempweightsize)
        {
            goldId=i;
            break;
        }
    }
    if(m_pIAndroidUserItem->GetUserScore()<10000)
    {
        goldId=0;
    }
    return goldId;
}

// convert the special command to game scene.
uint8_t SubCmdIDToStatus(uint8_t wSubCmdID)
{
//    uint8_t cbGameStatus = Common::SUB_S_GAME_START;
//    switch (wSubCmdID)
//    {
//    case Common::SUB_S_GAME_START:
//        cbGameStatus = Common::SUB_S_GAME_START;
//        break;
//    case Common::CMD_S_START_PLACE_JETTON:
//        cbGameStatus = Common::CMD_S_START_PLACE_JETTON;
//        break;
//    case Common::SUB_S_GAME_END:
//        cbGameStatus = Common::SUB_S_GAME_END;
//        break;
//    }
//Cleanup:
    return 0;
}

//游戏消息
//bool CAndroidUserItemSink::OnEventGameMessage(WORD wSubCmdID, void * pData, WORD wDataSize)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size)
{
    int64_t dwUserID = m_pIAndroidUserItem->GetUserId();
   // LOG(INFO) << "CAndroidUserItemSink::OnGameMessage wSubCmdID:" << (int)wSubCmdID << ", dwUserID:" << dwUserID;
   // openLog("OnGameMessage...");
    switch (subId)
	{
        case ErBaGang::SUB_S_GameEnd:
        {
            return (OnEventSceneMessage(subId, pData, size));
        }
        case ErBaGang::SUB_S_GameJetton:
        {
       // openLog("OnGameMessage %d..",wSubCmdID);
          //  uint8_t cbGameStatus = SubCmdIDToStatus(wSubCmdID);
            return (OnEventSceneMessage(subId, pData, size));
        }


       case ErBaGang::SUB_S_JETTON_SUCCESS:		//xiazhu
        {

            ErBaGang::CMD_S_PlaceJetSuccess JetSuccess;
            if (!JetSuccess.ParseFromArray(pData,size))
                return false;
            if(JetSuccess.dwuserid() == dwUserID)
                m_startsScore -= JetSuccess.ljettonscore();
                return true;
        }

        case ErBaGang::SUB_S_SCENE_START:
        {
            ErBaGang::CMD_S_Scene_GameStart GameStart;
            if (!GameStart.ParseFromArray(pData,size))
                return false;

            const ::google::protobuf::RepeatedField<int64_t>& chips = GameStart.userchips();
            for(int i=0;i<5;i++)
            {
                m_currentChips[i]=chips[i];
            }
            return true;
        }
        case ErBaGang::SUB_S_SCENE_Jetton:
        {
            ErBaGang::CMD_S_Scene_GameJetton GameJetton;
            if (!GameJetton.ParseFromArray(pData,size))
                return false;

            const ::google::protobuf::RepeatedField<int64_t>& chips = GameJetton.userchips();
            for(int i=0;i<5;i++)
            {
                m_currentChips[i]=chips[i];
            }
            return true;
        }
        case ErBaGang::SUB_S_SCENE_END:
        {
            ErBaGang::CMD_S_Scene_GameEnd GameEnd;
            if (!GameEnd.ParseFromArray(pData,size))
                return false;

            const ::google::protobuf::RepeatedField<int64_t>& chips = GameEnd.userchips();
            for(int i=0;i<5;i++)
            {
                m_currentChips[i]=chips[i];
            }
            return true;
        }
	}

	return true;
}

/*
//游戏消息
bool CAndroidUserItemSink::OnEventFrameMessage(WORD wSubCmdID, void * pData, WORD wDataSize)
{
	return true;
}
*/

//场景消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, const uint8_t * pData, uint32_t size)
{


   // LOG(INFO)<<"<<<m_pIAndroidUserItem->SetTimer(IDI_USER_ADD_SCORE, 100,150)";
   // LOG(INFO) << "CAndroidUserItemSink::OnEventSceneMessage cbGameStatus:" << (int)cbGameStatus << ", wDataSize:" << wDataSize;
   //openLog("OnEventSceneMessage...");
	switch (cbGameStatus)
	{

    case ErBaGang::SUB_S_GameJetton:		//游戏状态
    {
        //效验数据

        ErBaGang::CMD_S_GameJetton pJETTON;
        if (!pJETTON.ParseFromArray(pData,size)) return false;

       // LOG(INFO) << "CAndroidUserItemSink::OnEventSceneMessage SUB_S_SCENE_START:StartJetton" ;



        m_startsScore = m_pIAndroidUserItem->GetUserScore();

        m_isShensuanzi= false;
        m_hensuanziMaxArea = 1;
        m_RoundJettonCount=0;
        m_RoundJettonScore=0;

        //设置定时器
        struct timeval tv;
        gettimeofday(&tv,NULL);
        m_timerStart = tv.tv_sec*1000 + tv.tv_usec/1000;
        m_endtimer = m_Jetton - pJETTON.cbtimeleave();
        m_lasttimer = 0;
        float fd=(rand()%20)/10;
        if (m_endtimer>0)
          m_TimerJetton=m_pITableFrame->GetLoopThread()->getLoop()->runEvery(fd,bind(&CAndroidUserItemSink::OnTimerJetton, this));

        return true;
    }
    case ErBaGang::SUB_S_GameEnd:		//游戏状态
    {
        //效验数据
        //openLog("OnEventSceneMessage SUB_S_GAME_END..");
        m_RoundJettonCount=0;
        m_RoundJettonScore=0;
        ErBaGang::CMD_S_GameEnd pGameEnd;
        if (!pGameEnd.ParseFromArray(pData,size)) return false;

        m_pITableFrame->GetLoopThread()->getLoop()->runAfter(0.1, bind(&CAndroidUserItemSink::OnTimerGameEnd,this));
        return true;
    }
	}
    assert(false);

	return false;
}

bool CAndroidUserItemSink::CheckExitGame()
{
    //机器人结算后携带金币小于55自动退场
//    return m_pIAndroidUserItem->GetUserScore()>=55?false:true;
    int64_t userScore = m_pIAndroidUserItem->GetUserScore();
//    LOG(INFO) << "User Score : " << (int)userScore << " ,High Line : " << (int)m_strategy->exitHighScore <<", Low Line : "<< (int)m_strategy->exitLowScore;
    return userScore >= m_strategy->exitHighScore || userScore <= m_strategy->exitLowScore;
}
bool CAndroidUserItemSink::ExitGame()
{
    //判断是否需要离开
    struct timeval tv;
     gettimeofday(&tv,NULL);
     if(m_jionTime== 0)
     {
         m_jionTime = tv.tv_sec;
         return true;
     }

     //int randTime = rand()%100;
     int PlayTime = tv.tv_sec/60 - m_jionTime/60;
     if (/*(PlayTime == 5 && randTime< m_TimeMinuteprobability[0])  ||
         (PlayTime == 7 && randTime< m_TimeMinuteprobability[1])  ||
         (PlayTime == 9 && randTime< m_TimeMinuteprobability[2])  ||
         (PlayTime == 11 && randTime< m_TimeMinuteprobability[3]) ||
         (PlayTime == 13 && randTime< m_TimeMinuteprobability[4]) ||
         (PlayTime == 15 && randTime< m_TimeMinuteprobability[5]) ||*/
         m_AndroidExitTimes<PlayTime||
         (m_MaxWinTime>=10) ||
         CheckExitGame())//(m_pIAndroidUserItem->GetUserScore()<55000))
     {
         openLog("11111111 %d exituser=%d",PlayTime,m_pIAndroidUserItem->GetUserId());
         //m_pITableFrame->OnUserLeft(m_pIAndroidUserItem);
         //m_pITableFrame->ClearTableUser(m_pIAndroidUserItem->GetChairID());
         m_pIAndroidUserItem->setOffline();
         m_pIAndroidUserItem->setTrustee(true);
         m_jionTime = 0;
         m_MaxWinTime= 0;
         return true;
     }


   return true;
}

//读取配置
bool CAndroidUserItemSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/ebg_config.ini"))
    {
        LOG_INFO << "conf/ebg_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/ebg_config.ini", pt);

    //区域下注权重
    m_Areaprobability[0] =pt.get<uint32_t>("ANDROID_NEWCONFIG.ShunMenAreaprobability", 60);
    m_Areaprobability[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.TianMenAreaprobability", 60);
    m_Areaprobability[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.DiMenAreaprobability", 60);

    //可下注百分比权重
    m_probabilityWeight[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.1robability", 20);
    m_probabilityWeight[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.2robability", 30);
    m_probabilityWeight[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.3robability", 40);
    m_probabilityWeight[3] = pt.get<uint32_t>("ANDROID_NEWCONFIG.4robability", 30);
    m_probabilityWeight[4] = pt.get<uint32_t>("ANDROID_NEWCONFIG.5robability", 10);
    //m_probabilityWeight[5] = pt.get<uint32_t>("ANDROID_NEWCONFIG.6robability", 10);

    //退出概率
    m_TimeMinuteprobability[0] = pt.get<uint32_t>("ANDROID_NEWCONFIG.5Minute", 50);
    m_TimeMinuteprobability[1] = pt.get<uint32_t>("ANDROID_NEWCONFIG.9Minute", 60);
    m_TimeMinuteprobability[2] = pt.get<uint32_t>("ANDROID_NEWCONFIG.15Minute", 70);
    m_TimeMinuteprobability[3] = pt.get<uint32_t>("ANDROID_NEWCONFIG.19Minute", 80);
    m_TimeMinuteprobability[4] = pt.get<uint32_t>("ANDROID_NEWCONFIG.23Minute", 90);
    m_TimeMinuteprobability[5] = pt.get<uint32_t>("ANDROID_NEWCONFIG.30Minute", 100);

    m_Jetton = pt.get<uint32_t>("GAME_CONFIG.TM_GAME_PLACEJET", 15);
    m_kaishijiangeInit = pt.get<uint32_t>("ANDROID_NEWCONFIG.kaishijiangeInit", 2500);
    m_jiangeTimeInit           = pt.get<uint32_t>("GAME_CONFIG.jiangeTimeInit", 5500);
    m_end3s = pt.get<uint32_t>("ANDROID_NEWCONFIG.END3", 20);
    m_AndroidJettonProb         = pt.get<double>("ANDROID_NEWCONFIG.JettonProb", 0.3);

    //计算退出时间
    uint32_t TotalWeight = 0;
    for(int i = 0; i < 6; ++i)
        TotalWeight += m_TimeMinuteprobability[i];
    uint32_t times[6]={5,9,15,19,23,30};
    uint32_t randWeight = rand() % TotalWeight;
    uint32_t tempweightsize = 0;
    for (int i = 0; i < 6; ++i)
    {
        tempweightsize += m_TimeMinuteprobability[i];
        if (randWeight <= tempweightsize)
        {
            m_AndroidExitTimes=times[i];
            break;
        }
    }

    return true;
}

void  CAndroidUserItemSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//longhu//longhu_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
     return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}


//组件创建函数
// DECLARE_CREATE_MODULE(AndroidUserItemSink);
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





//////////////////////////////////////////////////////////////////////////
