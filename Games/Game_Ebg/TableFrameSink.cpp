#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

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

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

//#include "proto/Game.Common.pb.h"
//#include "proto/GameServer.Message.pb.h"
#include "proto/ErBaGang.Message.pb.h"

#include "cardlogic.h"
#include "betarea.h"
#include "bankemanager.h"
// #include "IAicsErbaGun.h"
#include "EbgAlgorithm.h"

#include "TableFrameSink.h"
#include "json/json.h"

#define RESULT_WIN      (1)
#define RESULT_LOST     (-1)
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0



//initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        string dir = "./log";
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = dir;
        FLAGS_logbufsecs = 0;

#ifdef __HJSDS__
    google::InitGoogleLogging("erbagang");
#else
        dir += "/erbagang";
        google::InitGoogleLogging("erbagang");
#endif//__HJSDS__

        FLAGS_log_dir = dir;
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(),S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    }

    virtual ~CGLogInit() {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit;	// 声明全局变局,初始化库.


//调用示例:
//	LOG(INFO) << "ValidTableFrameConfig called.";
//	LOG(ERROR) << "SendAllRoomMessage NO IMPL yet!";



class RomMgr
{
public:
    RomMgr()
    {
            bisInit = false;

            if(access("./log/erbagang", 0) == -1)
               mkdir("./log/erbagang",0777);
    }

    virtual ~RomMgr()
    {
            google::ShutdownGoogleLogging();
    }

public:
    static RomMgr& Singleton()
    {
        static RomMgr* sp = 0;
        if (!sp) {
             sp  = new RomMgr();
        }
    //Cleanup:
        return (*sp);
    }

private:
     bool bisInit;
};


//库存加上
//-lglog
TableFrameSink::TableFrameSink()//:m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "BaiRenNiuNiuTimerEventLoopThread")
{

    LoadRecored.clear();
	LoadNewRecored.clear();
    mgameTimeStart=7.0;
    mgameTimeJetton=15.0;
    mgameTimeEnd=10;
    ceshifasong=0;
    ReadCount=0;
    dPaoMaDengScore=500;
    IsIncludeAndroid=0;
    ResultRecored.clear();
    for(int i=0;i<9;i++)
    {
        ResultRecored.push_back(RandomInt(0,7));
    }
    vecPlayerList.clear();
    iWriteUserLength=0;
    m_ConfigPath = "conf/ebg_config.ini";
    mbIsOpen=false;
    mPItableFrame=nullptr;
    mfGameTimeLeftTime=(int32_t)time(NULL);
    miGameSceneState=GAME_SCENESTATE_STARTGAME;
    mfGameRoundTimes=0;
    mpGameBetArea=new BetArea();
    mpBankerManager= new BankeManager();
    m_bankerInfo = new ::ErBaGang::BankerInfo();
    m_pPlayerInfo.clear();
    miThisTurnMultical=20;
    m_bankerWinLost=0;
    for(int i=0;i<GAME_MAXAERA;i++)
    {
        playerMuiltcle[i]=0;
        ThisTurnPlaceMulticle[i]=0;
    }
    pPlayerInfoList.clear();
    srand(time(NULL));
    //m_TimerLoopThread.startLoop();
    m_strRoundId = "";
    m_applyBankerMaxCount=0;
    m_bControl=false;
    m_dControlkill=0;//控制杀放的概率值
    m_lLimitPoints=0;//红线阈值
    stockWeak = 0;
    for(int i=0;i<EBG_MAX_NUM;i++)
    {
        m_currentchips[i][0]=100;
        m_currentchips[i][1]=200;
        m_currentchips[i][2]=500;
        m_currentchips[i][3]=1000;
        m_currentchips[i][4]=5000;
    }
    useIntelligentChips=1;
	
	m_bGameEnd = false;
	m_fUpdateGameMsgTime = 0;
	m_fResetPlayerTime = 5.0f;
	m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_strTableId = "";		//桌号
	memset(m_winOpenCount, 0, sizeof(m_winOpenCount));
	memset(m_lLimitScore, 0, sizeof(m_lLimitScore));
	m_ipCode = 0;
}
TableFrameSink::~TableFrameSink()
{
    delete(mpGameBetArea);
    delete(mpBankerManager);
    delete(mpCardLogic);
}
void TableFrameSink::OnGameStart()
{

}
void TableFrameSink::OnEventGameEnd(  int64_t chairid, uint8_t nEndTag)
{
    LOG_INFO<<"On Event On Game End ";
}
bool TableFrameSink::OnEventGameScene(uint32_t chairid, bool bisLookon)
{
    LOG_INFO<<"On Event On Event Game Scene ";
}

bool TableFrameSink::OnUserEnter(int64_t userid, bool islookon)
{    
    shared_ptr<CServerUserItem> UserItem=nullptr;
    UserItem   =mPItableFrame->GetUserItemByUserId(userid);
    if(!UserItem)
    {
        return false;
    }
//    if(UserItem->GetChairId()<0||UserItem->GetChairId()>=100)
//    {
//        LOG(ERROR)<<"**********This is a WrongchairID"<<"   "<<(int)UserItem->GetChairId();
//        return false;
//    }
    int chairId=UserItem->GetChairId();
    if (m_pPlayerInfo.end()==m_pPlayerInfo.find(UserItem->GetChairId()))
    {
        shared_ptr<strPlalyers> PlayerInfo(new(strPlalyers));
        PlayerInfo->wCharid=UserItem->GetChairId();
        PlayerInfo->wUserid=userid;

        m_pPlayerInfo[UserItem->GetChairId()]=PlayerInfo;
        pPlayerInfoList.push_back(PlayerInfo);
    }
     UserItem->SetUserStatus(sPlaying);
    //PlayerRichSorting();
    if(!mbIsOpen)
    {
		string ip = mPItableFrame->GetGameRoomInfo()->serverIP;
		m_ipCode = 0;
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
				m_ipCode += stoi(ipVec[i]);
			}
		}
		m_strTableId = to_string(m_ipCode) + "-" + to_string(mPItableFrame->GetTableId() + 1);	//桌号

		erbaganAlgorithm.SetRoomMsg(mPItableFrame->GetGameRoomInfo()->roomId, mPItableFrame->GetTableId());

        m_startTime=chrono::system_clock::now();
        mpBankerManager->UpdateBankerData();//一开始设置系统庄家
        mpBankerManager->UpdateBankerInfo(m_bankerInfo);
        miShenSuanZiID=UserItem->GetChairId();
        miShenSuanZiUserID=UserItem->GetUserId();
        curBankerChairID=-1;
        mfGameRoundTimes=0;      
        mbIsOpen=true;
        m_strRoundId= mPItableFrame->GetNewRoundId();
        m_dwReadyTime = (uint32_t)time(NULL);
       m_replay.gameinfoid = m_strRoundId;
	   m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        GamefirstInGame();
        mfGameTimeLeftTime=(int32_t)time(NULL);
		m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, CALLBACK_0(TableFrameSink::updateGameMsg, this, false));
        //pPlayerInfoList.push_back(PlayerInfo);
    }
    for(int i=0;i<5;i++)
    {

        if(UserItem->GetUserScore()>= m_userScoreLevel[0]&&
                UserItem->GetUserScore()< m_userScoreLevel[1])
        {
            m_currentchips[chairId][i]=m_userChips[0][i];
        }
        else if(UserItem->GetUserScore()>= m_userScoreLevel[1]&&
                UserItem->GetUserScore()< m_userScoreLevel[2])
        {
            m_currentchips[chairId][i]=m_userChips[1][i];
        }
        else if(UserItem->GetUserScore()>= m_userScoreLevel[2]&&
                UserItem->GetUserScore()< m_userScoreLevel[3])
        {
            m_currentchips[chairId][i]=m_userChips[2][i];
        }
        else
        {
            m_currentchips[chairId][i]=m_userChips[3][i];
        }
    }
    switch(miGameSceneState)
    {
        case ErBaGang::SUB_S_GameStart:
        {
            SendGameSceneStart(userid,false);
            break;
        }
        case ErBaGang::SUB_S_GameJetton:
        {
            SendGameSceneJetton(userid,false);
            break;
        }
        case ErBaGang::SUB_S_GameEnd:
        {
            SendGameSceneEnd(userid,false);
            break;
        }
        default:
            break;
    }

    //mPItableFrame->BroadcastUserInfoToOther(UserItem);
    //mPItableFrame->SendAllOtherUserInfoToUser(UserItem);
    //mPItableFrame->BroadcastUserStatus(UserItem, true);

    return true;
}
bool TableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    return true;
}
bool TableFrameSink::OnUserLeft( int64_t userid, bool islookon)
{
    shared_ptr<CServerUserItem> UserItem=mPItableFrame->GetUserItemByUserId(userid);
    if(!UserItem) return false;
//     if(UserItem->GetChairId()<0||UserItem->GetChairId()>=100)
//     {
//         LOG(ERROR)<<"**********This is a WrongchairID"<<"   "<<(int)UserItem->GetChairId();
//     }
    bool bClear=false;
    UserItem->setOffline();
    if(!UserItem->IsAndroidUser() &&
            (m_pPlayerInfo[UserItem->GetChairId()]->dCurrentPlayerJetton<=0.1 ||miGameSceneState==ErBaGang::SUB_S_GameEnd)
            &&curBankerChairID!=UserItem->GetChairId())//庄家不能退出房间
    {
        {
            //WRITE_LOCK(m_mutex);
            m_pPlayerInfo.erase(UserItem->GetChairId());
        }
        openLog("OnUserLeft ClearTableUser");
        mPItableFrame->ClearTableUser(UserItem->GetChairId());
        bClear =true;
        //上庄类表中的玩家退出房间
        if(mpBankerManager->CancelBanker(UserItem))
        {
            UpdateBankerInfo2Client();
        }
        //UserItem->SetUserStatus(sGetout);
        //mPItableFrame->BroadcastUserStatus(UserItem, true);
    }else
    {
        UserItem->SetTrustship(true);
    }
    //UserItem->SetUserStatus(sOffline);
    //mPItableFrame->BroadcastUserStatus(UserItem, true);
    openLog("OnUserLeft...............");
    return bClear;

}

bool TableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}
bool TableFrameSink::CanLeftTable(int64_t userId)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = mPItableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
        return true;
    }else
    {
        uint32_t chairId = pIServerUserItem->GetChairId();
        shared_ptr<strPlalyers> &userInfo = m_pPlayerInfo[chairId];


        int64_t totalxz = 0;
        uint8_t status = mPItableFrame->GetGameStatus();
        if(ErBaGang::SUB_S_GameJetton == miGameSceneState)
        {
            totalxz = userInfo->dAreaJetton[0] + userInfo->dAreaJetton[1] + userInfo->dAreaJetton[2];
        }

        if(totalxz > 0)
            canLeft = false;
    }

    return canLeft;
}

void TableFrameSink::GamefirstInGame()
{
   miGameSceneState=ErBaGang::SUB_S_GameStart;
   //mPItableFrame->SetGameTimer(ErBaGang::SUB_S_GameEnd,12000 ,0);
   m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(mgameTimeStart, boost::bind(&TableFrameSink::GameTimerJetton, this));
}

void TableFrameSink::GameTimerStart()
{
     //连续5局未下注踢出房间
     //CheckKickOutUser();
     m_startTime=chrono::system_clock::now();
     //BulletinBoard();
     ResetTableDate();///clear Table

     ceshifasong=0;
     clearTableUser();
	 m_bGameEnd = false;
     mfGameTimeLeftTime=time(NULL);
     m_strRoundId= mPItableFrame->GetNewRoundId();
     m_dwReadyTime = (uint32_t)time(NULL);
     m_replay.clear();
     m_replay.gameinfoid = m_strRoundId;
	 m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
     uint64_t preBankerChairID=mpBankerManager->GetBankerChairID();
     uint64_t preBankerID=mpBankerManager->GetCurrentBanker();
	 m_strTableId = to_string(m_ipCode) + "-" + to_string(mPItableFrame->GetTableId() + 1);	//桌号

     //no apply banker now
     uint8_t cause=mpBankerManager->CheckBanker();
     if(cause)//更新庄家
     {
        //通知上个庄家下庄
        string str;
        if(cause==1)
            str="已连庄到最大次数，您已下庄";
        if(cause==2)
            str="金币不足，您已下庄";
        if(cause==3)
            str="您已下庄";

        //可能玩家上局退出去，刚好位置被下个进入的玩家占据
        if(mpBankerManager->GetCurrentBanker()) //
        {
            shared_ptr<IServerUserItem> pIServerUserItem;
            pIServerUserItem = mPItableFrame->GetUserItemByUserId(mpBankerManager->GetCurrentBanker());
            if(pIServerUserItem)
                SendNotifyToChair(preBankerChairID,str);
        }

        mpBankerManager->TryToBeBanker();
        curBankerChairID=mpBankerManager->GetBankerChairID();
        if(curBankerChairID!=-1)
        {
            str="您已上庄";
            SendNotifyToChair(curBankerChairID,str);
        }
        mpBankerManager->UpdateBankerData();
        mpBankerManager->UpdateBankerInfo(m_bankerInfo);
        //UpdateBankerInfo2Client();
     }
     NormalGameStart();

     miGameSceneState= ErBaGang::SUB_S_GameStart;
     mPItableFrame->SetGameStatus(ErBaGang::SUB_S_GameStart);
     //mPItableFrame->SetGameTimer(ErBaGang::SUB_S_GameEnd,15000,0);
     m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(mgameTimeStart, boost::bind(&TableFrameSink::GameTimerJetton, this));
}

void TableFrameSink::GameTimerJetton()
{

    ceshifasong=0;

    mfGameTimeLeftTime=time(NULL);
    //BulletinBoard();
    //ResetTableDate();///clear Table
    NormalGameJetton();

    miGameSceneState= ErBaGang::SUB_S_GameJetton;
    mPItableFrame->SetGameStatus(ErBaGang::SUB_S_GameJetton);
    //mPItableFrame->SetGameTimer(ErBaGang::SUB_S_GameEnd,15000,0);
    m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(mgameTimeJettonBroadcast,boost::bind(&TableFrameSink::GameTimerJettonBroadcast, this));
    m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(mgameTimeJetton, boost::bind(&TableFrameSink::GameTimerEnd, this));
}
void TableFrameSink::GameTimerEnd()
{
    int64_t shijian=time(NULL);
    if(ReadCount>=1)
    {
        ReadInI();
        ReadCount=0;
    }
    shijian=time(NULL)-shijian;
    openLog("读取配置文件所损耗的时间------%d",shijian);
    mfGameTimeLeftTime=time(NULL);
    miGameSceneState= ErBaGang::SUB_S_GameEnd;
	m_bGameEnd = true;
    NormalCalculate();
    // 跑马灯显示
    BulletinBoard();
    bool IsContinue = mPItableFrame->ConcludeGame(miGameSceneState);
    if(IsContinue)
    {
        mPItableFrame->SetGameStatus(ErBaGang::SUB_S_GameEnd);
        //mPItableFrame->SetGameTimer(ErBaGang::SUB_S_GameStart,21000,0);
		m_TimerIdStart = m_TimerLoopThread->getLoop()->runAfter(mgameTimeEnd, boost::bind(&TableFrameSink::GameTimerStart, this));
		m_TimerIdResetPlayerList = m_TimerLoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(TableFrameSink::RefreshPlayerList, this, true));

    }
    FillContainer(); //write score
    clearTableUser();

    ReadCount++;

}
void TableFrameSink::clearTableUser()
{
//    int64_t min_score = MIN_PLAYING_ANDROID_USER_SCORE;
//    if(mPItableFrame->GetGameRoomInfo())
//        min_score = std::max(mPItableFrame->GetGameRoomInfo()->enterMinScore, min_score);

    vector<int> clearUserIdVec;
    {
        //WRITE_LOCK(m_mutex);
        for(auto &it :m_pPlayerInfo)
        {
            shared_ptr<CServerUserItem> pUserItem=mPItableFrame->GetTableUserItem(it.first);
            if(!pUserItem)
            {
                assert(false);
                continue;
            }

            if(mPItableFrame->GetGameRoomInfo()->serverStatus !=SERVER_RUNNING||
                    (pUserItem->GetUserStatus()==sOffline ||
                    (pUserItem->IsAndroidUser() &&
                     pUserItem->GetUserScore() < MIN_PLAYING_ANDROID_USER_SCORE)))  //kick player offline
            {
                pUserItem->setOffline();
                clearUserIdVec.push_back(pUserItem->GetChairId());
                if(it.second->bApplyBanker)
                    mpBankerManager->CancelBanker(pUserItem);

                if(pUserItem->GetChairId()==curBankerChairID)
                    mpBankerManager->SetContinueBanker(false);
            }
        }
    }

    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        mPItableFrame->ClearTableUser(clearUserIdVec[i]);
        m_pPlayerInfo.erase(clearUserIdVec[i]);
    }
}

void TableFrameSink::GameTimerJettonBroadcast()
{
    if(miGameSceneState != ErBaGang::SUB_S_GameJetton)
    {
        return;
    }
    memset(tmpJttonList,0,sizeof(tmpJttonList))  ;
    memset(iarr_tmparea_jetton,0,sizeof(iarr_tmparea_jetton));
    {
        swap(m_arr_jetton,tmpJttonList); //there swap pointer
        swap(m_iarr_tmparea_jetton,iarr_tmparea_jetton);
    }

    ErBaGang::CMD_S_Jetton_Broadcast jetton_broadcast;
    jetton_broadcast.Clear();

    bool havejetton=false;
    for (int x=0; x<MAX_JETTON_AREA; x++)
    {
        if (iarr_tmparea_jetton[x] > 0)
        {
            havejetton = true;
            break;
        }
    }
    if (havejetton)//check jetton
    {
        for(auto &it : m_pPlayerInfo)
        {
            shared_ptr<CServerUserItem> player=this->mPItableFrame->GetTableUserItem(it.first);
            if(!player||player->IsAndroidUser()) continue;
            jetton_broadcast.Clear();
            for (int x=0; x<MAX_JETTON_AREA; x++)
            {
               jetton_broadcast.add_alljettonscore(mpGameBetArea->GetCurrenAreaJetton(x));
               jetton_broadcast.add_tmpjettonscore(iarr_tmparea_jetton[x]- tmpJttonList[it.first].iAreaJetton[x]);
            }
            std::string data = jetton_broadcast.SerializeAsString();
            mPItableFrame->SendTableData(it.first, ErBaGang::SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
            //openLog("ErBaGang::SUB_S_JETTON_BROADCAST chairID=%d",it.first);
        }
    }
    if(miGameSceneState == ErBaGang::SUB_S_GameJetton)
    {
        m_TimerJettonBroadcast=m_TimerLoopThread->getLoop()->runAfter(mgameTimeJettonBroadcast,boost::bind(&TableFrameSink::GameTimerJettonBroadcast, this));
    }
}

bool TableFrameSink::OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize)
{
    if(chairid<0||chairid>=EBG_MAX_NUM)
    {
        LOG(ERROR)<<"**********This is a WrongchairID"<<"   "<<(int)chairid;

        return false;
    }
    switch(subid)
    {
        case ErBaGang::SUB_C_USER_JETTON:
        {

           // LOG(ERROR)<<"**********This is a jetton"<<"   "<<(int)chairid;
			ErBaGang::CMD_C_PlaceJet placeJet;
			placeJet.ParseFromArray(data, datasize);
			int cbJetArea = placeJet.cbjettonarea();
			int lJetScore = (int)placeJet.ljettonscore();
			cbJetArea = cbJetArea - 1;
            GamePlayerJetton( chairid, cbJetArea, lJetScore);
            break;
        }     
        case ErBaGang::SUB_C_USER_ASKLIST:
        {
            SendPlayerList( chairid,  subid, (char *)data,  datasize);//pai hang bang
            break;
        }
        case ErBaGang::SUB_C_HANDLE_BANKER:
        {
            ErBaGang::CMD_C_Handle_Banker handlBanker;
            handlBanker.ParseFromArray(data,datasize);
            if(handlBanker.mask()==1)
                GamePlayerApplyBanker(chairid);
            else
                GamePlayerCancelBanker(chairid);
            break;
        }
        case ErBaGang::SUB_C_USER_REPEAT_JETTON:
        {
            Rejetton( chairid,  subid, (char *)data,  datasize);
            break;
        }
        case ErBaGang::SUB_C_QUERY_PLAYERLIST:
        {
            GamePlayerQueryPlayerList(chairid,  subid, (char*)data,  datasize);//wan jia lie biao
            break;
        }
    }
    return true;
}
void TableFrameSink::FillContainer()
{
    vector<tagScoreInfo> infoVec;
    shared_ptr<strPlalyers> playerInfo;
    int mbsshuiwin=0;
    tagScoreInfo score;
    int64_t totalBetScore = 0;
    uint8_t cards[8];
    auto card_value=[](int card)->int{
        if (card==10){
            card=0x37;
        }else{
            card|=0x20;
        }
        return card;
    };
    // 1 2 3 0(BANKER)
    int index=-1;
    for(int i=0;i<4;i++ ){
        cards[++index]=card_value(mpCardLogic->vecCardGroup[(i+1)%4].card[0]);
        cards[++index]=card_value(mpCardLogic->vecCardGroup[(i+1)%4].card[1]);
    }

    for(auto &it :m_pPlayerInfo){
        shared_ptr<CServerUserItem> User=mPItableFrame->GetTableUserItem(it.first);
        if(!User)
            continue;
        playerInfo=it.second;


        score.clear();


        totalBetScore = playerInfo->dCurrentPlayerJetton;
        if (totalBetScore> 0 )
        {
            score.cellScore.push_back(playerInfo->dAreaJetton[0]);
            score.cellScore.push_back(playerInfo->dAreaJetton[1]);
            score.cellScore.push_back(playerInfo->dAreaJetton[2]);

            score.chairId = it.first;//User->GetChairId();
            score.betScore = totalBetScore;
            score.betScore=playerInfo->dCurrentPlayerJetton;
            score.rWinScore = score.betScore;
            score.addScore=playerInfo->ShuiHouWin;
            score.startTime=m_startTime;
            score.cardValue = GlobalFunc::converToHex(cards, 8);
            score.revenue =playerInfo->usershuishou;

            infoVec.push_back(score);
        }
        // 统计连续未下注的次数
        if (totalBetScore<=0)
        {
            if (curBankerChairID!=-1 && it.first != curBankerChairID)
            {
                m_pPlayerInfo[it.first]->NotBetJettonCount++;
            }
            else
            {
                 m_pPlayerInfo[it.first]->NotBetJettonCount++;
            }
        }
        else
        {
            m_pPlayerInfo[it.first]->NotBetJettonCount=0;
        }


    }

     mPItableFrame->WriteUserScore(&infoVec[0],infoVec.size(),m_strRoundId);//mfGameRoundTimes
     mPItableFrame->SaveReplay(m_replay);
}
void TableFrameSink::writeRecored()
{

   int index[50] ;
   int64_t x_pos[50],
   y_pos[50];
   for(int i = 0; i < 50; i ++ )
   {
   index[i] = i;
   x_pos[i] = rand()%1000 * 0.01 ;
   y_pos[i] = rand()%2000 * 0.01;
   }
   //写出txt
   FILE * fid = fopen("txt_out.txt","w");
   if(fid == NULL)
   {
        printf("写出文件失败！\n");
   return;
   }
   for(int i = 0; i < 50; i ++ )
   {
      fprintf(fid,"%03d\t%4.6lf\t%4.6lf\n",index[i],x_pos[i],y_pos[i]);
   }
   fclose(fid);
}
void TableFrameSink::readRecored()
{
    FILE * fid = fopen("txt_out.txt","r");
    if(fid == NULL)
    {
        printf("打开%s失败","txt_out.txt");
        return;
    }
    vector<int> index;
    vector<double> x_pos;
    vector<double> y_pos;
    int mode = 1;
    printf("mode为1，按字符读入并输出；mode为2，按行读入输出；mode为3，知道数据格式，按行读入并输出\n");
    scanf("%d",&mode);
    if(mode == 1)
    {
        //按字符读入并直接输出
        char ch;       //读取的字符，判断准则为ch不等于结束符EOF（end of file）
        while(EOF!=(ch= fgetc(fid)))
            printf("%c", ch);
    }
    else if(mode == 2)
    {
        char line[1024];
        memset(line,0,1024);
        while(!feof(fid))
        {
            fgets(line,1024,fid);
            printf("%s\n", line); //输出
        }
    }
    else if(mode == 3)
    {		//知道数据格式，按行读入并存储输出
        int index_tmp;
        double x_tmp, y_tmp;
        while(!feof(fid))
        {
            fscanf(fid,"%d%lf%lf\n",&index_tmp, &x_tmp, &y_tmp);
            index.push_back(index_tmp);
            x_pos.push_back(x_tmp);
            y_pos.push_back(y_tmp);
        }
        for(int i = 0; i < index.size(); i++)
            printf("%04d\t%4.8lf\t%4.8lf\n",index[i], x_pos[i], y_pos[i]);
    }
    fclose(fid);

}

void TableFrameSink::recoredResult()
{
    if (LoadRecored.size()>10)
        LoadRecored.erase(LoadRecored.begin());

	m_thisResult.shunmen = 0;
	m_thisResult.tianmen = 0;
	m_thisResult.dimen = 0;

    recored it; //record each door winlost last game
    if(ThisTurnPlaceMulticle[0]>0)
    {
        it.shunmen=1;
		m_thisResult.shunmen = 1;
    }
    else
    {
        it.shunmen=0;
    }
    if(ThisTurnPlaceMulticle[1]>0)
    {
        it.tianmen=1;
		m_thisResult.tianmen = 1;
    }
    else
    {
        it.tianmen=0;
    }
    if(ThisTurnPlaceMulticle[2]>0)
    {
        it.dimen=1;
		m_thisResult.dimen = 1;
    }
    else
    {
        it.dimen=0;
    }
    LoadRecored.push_back(it);
}
void TableFrameSink::ReadInI()
{

    if(!boost::filesystem::exists(m_ConfigPath)){
        LOG_ERROR<<"config file exists"<<m_ConfigPath;
        playerMuiltcle[0]=2;
        playerMuiltcle[1]=2;
        playerMuiltcle[2]=2;
        AreaMaxJetton[0]=10000;
        AreaMaxJetton[1]=10000;
        AreaMaxJetton[2]=10000;
        IsIncludeAndroid=0;
        dPaoMaDengScore=500;
        m_bControl=true;
        return;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini(m_ConfigPath,pt);

    stockWeak = pt.get<double>("GAME_CONFIG.STOCK_WEAK",1.0);

    mgameTimeStart=pt.get<double>("GAME_STATE_TIME.TM_GAME_START",7);
    mgameTimeJetton=pt.get<double>("GAME_STATE_TIME.TM_GAME_JETTON",15);
    mgameTimeEnd=pt.get<double>("GAME_STATE_TIME.TM_GAME_END",10);
    mgameTimeJettonBroadcast =pt.get<float>("GAME_STATE_TIME.TM_GAME_JETTON_BROADCAST",1);

    playerMuiltcle[0]=pt.get<int>("GAME_MULTIPLIER.AREA_SHUNMEN",2);
    playerMuiltcle[1]=pt.get<int>("GAME_MULTIPLIER.AREA_TIANMEN",2);
    playerMuiltcle[2]=pt.get<int>("GAME_MULTIPLIER.AREA_DIMEN",2);

    AreaMaxJetton[0]=pt.get<double>("GAME_AREA_MAX_JETTON_" + to_string(mPItableFrame->GetGameRoomInfo()->roomId) + ".Shun",10000);
    AreaMaxJetton[1]=pt.get<double>("GAME_AREA_MAX_JETTON_" + to_string(mPItableFrame->GetGameRoomInfo()->roomId) + ".Tian",10000);
    AreaMaxJetton[2]=pt.get<double>("GAME_AREA_MAX_JETTON_" + to_string(mPItableFrame->GetGameRoomInfo()->roomId) + ".Di",10000);

    IsIncludeAndroid=pt.get<double>("GAME_CONFIG.USER_ANDROID_DATA_TEST_AISC",0);
    dPaoMaDengScore=pt.get<double>("GAME_CONFIG.PAOMADENG_CONDITION_SET",500);
    m_bControl=pt.get<float>("GAME_CONFIG.CONTROL",1);
    mpBankerManager->SetMinApplyBankerScore(pt.get<double>("BANKER_CONFIG.MIN_APPLY_BANKER_SCORE",100000000));
    mpBankerManager->SetMinBankerScore(pt.get<double>("BANKER_CONFIG.MIN_BANKER_SCORE",10000000));
    m_applyBankerMaxCount=pt.get<int>("BANKER_CONFIG.MAX_APPLY_COUNT",5);

    m_dControlkill=pt.get<double>("GAME_CONFIG.Controlkill",0.45);
    //m_lLimitPoints=pt.get<int64_t>("GAME_CONFIG.LimitPoints",500000);
	m_lLimitPoints = pt.get<int64_t>("GAME_CONFIG.LimitPoints" + to_string(mPItableFrame->GetGameRoomInfo()->roomId % 100), 500000);
    m_fUpdateGameMsgTime = pt.get<float>("GAME_CONFIG.TM_UpdateGameMsgTime", 1.0);
	m_fResetPlayerTime = pt.get<float>("GAME_CONFIG.TM_RESET_PLAYER_LIST", 5.0);

    vector<int64_t> scorelevel;
    vector<int64_t> chips;
	string strCHIP_CONFIGURATION = "";
	strCHIP_CONFIGURATION = "CHIP_CONFIGURATION_" + to_string(mPItableFrame->GetGameRoomInfo()->roomId);

	m_lLimitScore[0] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMin", 100);
	m_lLimitScore[1] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMax", 2000000);

    vector<int64_t> allchips;
    allchips.clear();
    scorelevel.clear();
    chips.clear();
    useIntelligentChips=pt.get<int64_t>(strCHIP_CONFIGURATION + ".useintelligentchips",1);
    scorelevel = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear1","1,2,5,10,50"));
    for(int i=0;i<4;i++)
    {
        m_userScoreLevel[i]=scorelevel[i];
    }
    for(int i=0;i<5;i++)
    {
        m_userChips[0][i]=chips[i];
        allchips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear2","1,2,5,10,50"));
    for(int i=0;i<5;i++)
    {
        m_userChips[1][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear3","1,2,5,10,50"));
    for(int i=0;i<5;i++)
    {
        m_userChips[2][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear4","1,2,5,10,50"));
    for(int i=0;i<5;i++)
    {
        m_userChips[3][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000"));
    if(!useIntelligentChips)
    {
        for(int i=0;i<5;i++)
        {
            m_userChips[0][i]=chips[i];
            m_userChips[1][i]=chips[i];
            m_userChips[2][i]=chips[i];
            m_userChips[3][i]=chips[i];
        }
    }
}

void TableFrameSink::ReadCardConfig()
{
    return ;
    m_listTestCardArray.clear();
    if (!boost::filesystem::exists("./conf/cards_ebg.json"))
        return ;
    boost::property_tree::ptree pt;
    try{
        boost::property_tree::read_json("./conf/cards_ebg.json",pt);
    }catch(...){
        LOG(INFO)<<"cards.json firmat error !!!!!!!!!!!!!!!";
        return ;
    }

    boost::property_tree::ptree  pCard_weave=pt.get_child("card_weave");
    boost::property_tree::ptree cards;
    for(auto& weave :pCard_weave){
        cards=weave.second.get_child("liang");
        for(auto& it:cards){
            m_listTestCardArray.push_back(stod(it.second.data()));
        }
        cards=weave.second.get_child("an");
        for(auto& it:cards){
            m_listTestCardArray.push_back(stod(it.second.data()));
        }
    }
    string str="./conf/cards";
    str+= time((time_t*)NULL);
    str+="_ebg.json";
    boost::filesystem::rename("./conf/cards_ebg.json",str);

}
bool TableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    // Algorithmc::Instence()->InitAlgor(mPItableFrame);
    if(pTableFrame==nullptr)
    {
        return false;
    }
    srand(unsigned(time(0)));
    mPItableFrame=pTableFrame;
    m_TimerLoopThread = mPItableFrame->GetLoopThread();
    if(mPItableFrame==nullptr)
    {
        return false;
    }
    m_replay.cellscore =  mPItableFrame->GetGameRoomInfo()->floorScore;
    m_replay.roomname = mPItableFrame->GetGameRoomInfo()->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    mpCardLogic=new CardLogic();
    ReadInI();
    srand((int)time(NULL));
    return true;
}

void TableFrameSink::RepositionSink()
{

}
void TableFrameSink::NormalGameStart()
{
    bool checkFinish=false;
    while(!checkFinish){
        checkFinish=true;
        for(auto& it : m_pPlayerInfo){
            shared_ptr<CServerUserItem>User = this->mPItableFrame->GetTableUserItem(it.first);
            if(!User){
                m_pPlayerInfo.erase(it.first);
                checkFinish=false;
                break;
            }
        }
    }
    pPlayerInfoList.clear();
    shared_ptr<strPlalyers> playerInfo;
	shared_ptr<IServerUserItem> pUserItem;
    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
        LOG(INFO)<<"user="<<(int)playerInfo->wUserid<<"     playereWin="<<playerInfo->ShuiHouWin;
		pUserItem = mPItableFrame->GetTableUserItem(playerInfo->wCharid);
		if (!pUserItem)
			continue;
		// 否则把重押分数记录
		for (int j = 0; j < 3; j++)
		{
			playerInfo->dLastJetton[j] = playerInfo->dAreaJetton[j];
		}
		playerInfo->clear();
		pPlayerInfoList.push_back(playerInfo);
    }
    PlayerRichSorting();

    mpCardLogic->InitVector();//random card and send one card to each door

    if (m_listTestCardArray.size()==0)
        ReadCardConfig();

    if (m_listTestCardArray.size()>7)
    {
        for(int i = 0 ; i < 4 ;++i)
        {
            mpCardLogic->vecCardGroup[i].setCard(*(m_listTestCardArray.begin()),0);
            m_listTestCardArray.pop_front();
        }
    }

    ErBaGang::CMD_S_GameStart gameStart;
    gameStart.set_cbplacetime(mgameTimeStart);
    gameStart.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);
    for(int j=0;j<GAME_MAXAERA+1;j++)
    {
        for(int k=0;k<2;k++)
        {
            gameStart.add_cardgroup(mpCardLogic->vecCardGroup[j].card[k]);
            LOG(INFO)<<"NormalGameStart car ="<<mpCardLogic->vecCardGroup[j].card[k];

        }
        if (j<3){
            gameStart.add_lalljettonscore(mpGameBetArea->mifourAreajetton[j]);
            gameStart.add_mutical(playerMuiltcle[j]);
        }
    }
    for(int j=0;j<2;j++)
    {
        gameStart.add_shuaizi(mpCardLogic->shuaizi[j]);
    }
    LOG(INFO)<<"shaizi1===="<<mpCardLogic->shuaizi[0]<<"    shaizi2===="<<mpCardLogic->shuaizi[1];
    int listNum=0;
    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end()&&listNum<10;it++)
    {
        shared_ptr<CServerUserItem> pUser=this->mPItableFrame->GetTableUserItem((*it)->wCharid);
        if (!pUser)
            continue;
        ErBaGang::PlayerListItem *item = gameStart.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_luserscore(pUser->GetUserScore());
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(listNum+1); // has sort in "PlayerRichSorting" function
        item->set_ltwentywinscore((*it)->GetTwentyJetton());
        item->set_ltwentywincount((*it)->GetTwentyWin());
        listNum++;
    }
    gameStart.set_onlinenum(mPItableFrame->GetPlayerCount(true));
    gameStart.set_roundid(m_strRoundId);

    ErBaGang::BankerInfo* bankerinfo=gameStart.mutable_bankerinfo();
    bankerinfo->CopyFrom(*m_bankerInfo);

	//限红
	for (int i = 0;i < 2;i++)
	{
		gameStart.add_llimitscore(m_lLimitScore[i]);
	}
	gameStart.set_tableid(m_strTableId);

    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
        shared_ptr<CServerUserItem>User = this->mPItableFrame->GetTableUserItem(it.first);
        gameStart.clear_selfjettonscore();
        for(int j=0;j<GAME_MAXAERA;j++)
        {
            gameStart.add_selfjettonscore(playerInfo->dAreaJetton[j]);
        }

        gameStart.set_luserscore(User->GetUserScore());

        std::string endData = gameStart.SerializeAsString();
        mPItableFrame->SendTableData(User->GetChairId(),  ErBaGang::SUB_S_GameStart, (uint8_t *) endData.c_str(), endData.length());
        //openLog("ErBaGang::SUB_S_GameStart chairID=%d",User->GetChairId());
    }


}
void TableFrameSink::NormalGameJetton()
{

    pPlayerInfoList.clear();
    shared_ptr<strPlalyers> playerInfo;
    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
//        LOG(INFO)<<"user="<<(int)playerInfo->wUserid<<"     playereWin="<<playerInfo->ShuiHouWin;
        pPlayerInfoList.push_back(playerInfo);
    }
    PlayerRichSorting(); //maybe after gamestart someone exit room

    LOG(INFO)<<"shaizi1===="<<mpCardLogic->shuaizi[0]<<"    shaizi2===="<<mpCardLogic->shuaizi[1];

    ErBaGang::CMD_S_GameJetton gameJetton;
    for(int j=0;j<2;j++)
    {
        gameJetton.add_shuaizi(mpCardLogic->shuaizi[j]);
    }
    gameJetton.set_cbplacetime(mgameTimeJetton);
    gameJetton.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);
    for(int j=0;j<GAME_MAXAERA+1;j++)
    { 
        for(int k=0;k<2;k++)
        {
            gameJetton.add_cardgroup(mpCardLogic->vecCardGroup[j].card[k]);
            LOG(INFO)<<"NormalgameJetton car ="<<mpCardLogic->vecCardGroup[j].card[k];

        }
        if (j<3){
            gameJetton.add_alljettonscore(mpGameBetArea->mifourAreajetton[j]);
            gameJetton.add_mutical(playerMuiltcle[j]);
        }
    }
    int listNum=0;
    for(vector<shared_ptr<strPlalyers>>::iterator it1=pPlayerInfoList.begin();it1!=pPlayerInfoList.end()&&listNum<10;it1++)//give top 10
    {
        shared_ptr<CServerUserItem> pUser=mPItableFrame->GetTableUserItem((*it1)->wCharid);
        if(!pUser) continue;

        ErBaGang::PlayerListItem *item = gameJetton.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_luserscore(pUser->GetUserScore());
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(listNum+1);
        item->set_ltwentywinscore((*it1)->GetTwentyJetton());
        item->set_ltwentywincount((*it1)->GetTwentyWin());
        listNum++;
    }
    gameJetton.set_onlinenum(mPItableFrame->GetPlayerCount(true));
    gameJetton.set_roundid(m_strRoundId);

    ErBaGang::BankerInfo* bankerinfo=gameJetton.mutable_bankerinfo();
    bankerinfo->CopyFrom(*m_bankerInfo);

    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
        shared_ptr<CServerUserItem>User = this->mPItableFrame->GetTableUserItem(it.first);
        if(!User)
            continue;

        gameJetton.clear_selfjettonscore();
        for(int j=0;j<GAME_MAXAERA;j++)
        {
            gameJetton.add_selfjettonscore(playerInfo->dAreaJetton[j]);
        }
        gameJetton.set_luserscore(User->GetUserScore());
        std::string endData = gameJetton.SerializeAsString();
        mPItableFrame->SendTableData(User->GetChairId(),  ErBaGang::SUB_S_GameJetton, (uint8_t *) endData.c_str(), endData.length());
        //openLog("ErBaGang::SUB_S_GameJetton chairID=%d",User->GetChairId());
    }
}
void TableFrameSink::BulletinBoard()
{
    double winscore=0.0;
    int idex=0;
    shared_ptr<strPlalyers> playerInfo;
    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
        shared_ptr<CServerUserItem> playe=mPItableFrame->GetTableUserItem(it.first);

        // if(playerInfo->ShuiHouWin>winscore)
        // {
        //     winscore=playerInfo->ShuiHouWin;
        //     idex=it.first;
        // }
        if(playerInfo->ShuiHouWin >= mPItableFrame->GetGameRoomInfo()->broadcastScore)
        {
            int sgType = SMT_GLOBAL|SMT_SCROLL;
            mPItableFrame->SendGameMessage(it.first,"",sgType,playerInfo->ShuiHouWin);
        }
    }

    // string Huode;
    // if(miThisTurnResultIndex==1)
    // {
    //     //strcat(Huode, "天门");
    //     Huode=("天门");
    // }
    // else if(miThisTurnResultIndex==2)
    // {
    //     //strcat(Huode, "地门");
    //     Huode=("地门");
    // }
    // else if(miThisTurnResultIndex==3)
    // {
    //     //strcat(Huode, "順门");
    //     Huode=("順门");
    // }

    // if(winscore>mPItableFrame->GetGameRoomInfo()->broadcastScore)
    // {
    //     int sgType = SMT_GLOBAL|SMT_SCROLL;
    //     mPItableFrame->SendGameMessage(idex,"",sgType,winscore);
    // }
}
void TableFrameSink::ResetTableDate()
{
    miThisTurnMultical=0;
    miThisTurnResultIndex=0;
    miThisTurnStopIndex=0;
    miThisTurnBestPlayerUserID=0;
    miThisTurnBestPlayerChairID=0;
    miThisTurnBestPlayerWinScore=0.0;


    shared_ptr<strPlalyers> playerInfo;
    for(auto& it : m_pPlayerInfo){
        playerInfo=it.second;
        if(playerInfo->dCurrentPlayerJetton<1)
        {
            playerInfo->clear();
            continue;
        }
        for(int j=0;j<GAME_MAXAERA;j++)
        {
            playerInfo->dLastJetton[j]=playerInfo->dAreaJetton[j];
        }

        playerInfo->clear();
    }
    mpGameBetArea->ThisTurnClear();
}
void TableFrameSink::ArgorithResult()
{

    int64_t shijian=time(NULL);
    tagStorageInfo storageInfo;
    mPItableFrame->GetStorageScore(storageInfo);
    int  isHaveBlackPlayer=0;
    for(int i=0;i<EBG_MAX_NUM;i++)
    {
        shared_ptr<CServerUserItem> user=mPItableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=m_pPlayerInfo[i]->dAreaJetton[0]
        +m_pPlayerInfo[i]->dAreaJetton[1]+m_pPlayerInfo[i]->dAreaJetton[2];
        if(allJetton==0) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(mPItableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                if(it->second>0)
                {
                    isHaveBlackPlayer=it->second;//
                }

                openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),mPItableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }


    }
    //难度干涉值
    //m_difficulty
    m_difficulty =mPItableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        erbaganAlgorithm.SetMustKill(-1);
        for(int i=0;i<3;i++)
         erbaganAlgorithm.SetBetting(i,mpGameBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
        erbaganAlgorithm.GetOpenCard(iopenResult,3);
    }
    else
    {
        if(isHaveBlackPlayer)
        {
            float probilityAll[3]={0.0f};
            //user probilty count
            for(int i=0;i<EBG_MAX_NUM;i++)
            {
                shared_ptr<CServerUserItem> user=mPItableFrame->GetTableUserItem(i);
                if(!user||user->IsAndroidUser())
                {
                    continue;
                }
                int64_t allJetton=m_pPlayerInfo[user->GetChairId()]->dAreaJetton[0]
                +m_pPlayerInfo[user->GetChairId()]->dAreaJetton[1]+m_pPlayerInfo[user->GetChairId()]->dAreaJetton[2];

                if(allJetton==0) continue;

                if(user->GetBlacklistInfo().status == 1 )
                {
                    int shuzhi=0;
                    auto it=user->GetBlacklistInfo().listRoom.find(to_string(mPItableFrame->GetGameRoomInfo()->gameId));
                    if(it != user->GetBlacklistInfo().listRoom.end())
                    {
                        shuzhi=it->second;
                    }
                    else
                    {
                        shuzhi=0;
                    }
                    float gailv=(float)shuzhi/1000;
                   for(int j=0;j<MAX_JETTON_AREA;j++)
                   {
                       if(mpGameBetArea->AndroidmifourAreajetton[j]>0)
                       {
                           probilityAll[j]+=(gailv*(float)m_pPlayerInfo[user->GetChairId()]->dAreaJetton[j])/mpGameBetArea->AndroidmifourAreajetton[j];
                       }

                   }
                }
            }
            openLog (" 开始黑名单控制" );
            erbaganAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<3;i++)
             erbaganAlgorithm.SetBetting(i,mpGameBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            erbaganAlgorithm.BlackGetOpenCard(iopenResult,3,probilityAll);
        }
        else
        {
            erbaganAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<3;i++)
             erbaganAlgorithm.SetBetting(i,mpGameBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            erbaganAlgorithm.GetOpenCard(iopenResult,3);
        }

    }




    double res=erbaganAlgorithm.CurrentProfit();
    if(res>0)
    {
        res = res*(1000.0-mPItableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    mPItableFrame->UpdateStorageScore(res);//结果出来了就可以刷新库存了,当然是不算税的
    mPItableFrame->GetStorageScore(storageInfo);
    if(iopenResult[0]==0)
        openLog("本次顺门输");
    else
        openLog("本次顺门赢");
    if(iopenResult[1]==0)
        openLog("本次天门输");
    else
        openLog("本次天门赢");
    if(iopenResult[2]==0)
        openLog("本次地门输");
    else
        openLog("本次地门赢");
    openLog("系统库存上限------%d",storageInfo.lUplimit);
    openLog("系统库存下限------%d",storageInfo.lLowlimit);
    openLog("系统当前库存------%d",storageInfo.lEndStorage);
    openLog("本次系统赢输------%d",erbaganAlgorithm.CurrentProfit());
    shijian=time(NULL)-shijian;
    openLog("调用算法总共耗时------%d",shijian);
}
void TableFrameSink::NormalCalculate()
{
    // default.
    for (int i=0;i<GAME_MAXAERA;i++)
    {
       int win=rand()%2;
      // openLog("sign:[%x],此门总嬴分:[%0.02lf],牌型:[%d],倍率:[%d],是否中此门:[%d]", out.sign, out.lscorewined, 0, out.multi, out.iswin);
       if(win==1) {
           iopenResult[i]= 1;
       }   else
       {
           iopenResult[i]=-1;
       }
    }
    if (m_bControl)
        ArgorithResult();  //get winlost each door

    for(int i=0;i<GAME_MAXAERA;++i)
    {
        if(iopenResult[i]==0)
        {
           iopenResult[i]=-1;
        }
    }

    openLog("ArgorithResult, gate shun odds:[%d],iswin:[%d] gate tian odds:[%d],iswin:[%d], gate di odds:[%d],iswin:[%d]",
            ThisTurnPlaceMulticle[0],iopenResult[0],
            ThisTurnPlaceMulticle[1],iopenResult[1],
            ThisTurnPlaceMulticle[2],iopenResult[2]);
    LOG(INFO)<<"openresult--------  "<<iopenResult[0]<<"  "<<iopenResult[1]<<"  "<<iopenResult[2];
    mpCardLogic->GetCardGroup(iopenResult);
    if (m_listTestCardArray.size()>3){
        for(int i = 0 ; i < 4 ; ++i)
        {
             mpCardLogic->vecCardGroup[i].setCard(*(m_listTestCardArray.begin()),1);
             m_listTestCardArray.pop_front();
        }
    }

    ThisTurnPlaceMulticle[0]= mpCardLogic->vecCardGroup[1].comparewithother(mpCardLogic->vecCardGroup[0]);
    ThisTurnPlaceMulticle[1]= mpCardLogic->vecCardGroup[2].comparewithother(mpCardLogic->vecCardGroup[0]);
    ThisTurnPlaceMulticle[2]= mpCardLogic->vecCardGroup[3].comparewithother(mpCardLogic->vecCardGroup[0]);
    stringstream ss;
    ss << "0:" << mpCardLogic->vecCardGroup[0].getcardtype() <<"|";
    ss << "1:" << mpCardLogic->vecCardGroup[1].getcardtype() <<"|";
    ss << "2:" << mpCardLogic->vecCardGroup[2].getcardtype() <<"|";
    ss << "3:" << mpCardLogic->vecCardGroup[3].getcardtype();
    m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,ss.str(),-1,opShowCard,-1,-1);

    openLog("ThisTurnPlaceMulticle %d,%d,%d ---------- ",ThisTurnPlaceMulticle[0],ThisTurnPlaceMulticle[1],ThisTurnPlaceMulticle[2]);
    LOG(INFO)<<"ThisTurnPlaceMulticle[0]="<<ThisTurnPlaceMulticle[0];
    LOG(INFO)<<"ThisTurnPlaceMulticle[1]="<<ThisTurnPlaceMulticle[1];
    LOG(INFO)<<"ThisTurnPlaceMulticle[2]="<<ThisTurnPlaceMulticle[2];


    //openLog("shunmen peilv=%d  tianmen peilv=%d  dimen peilv=%d",ThisTurnPlaceMulticle[0],ThisTurnPlaceMulticle[1],ThisTurnPlaceMulticle[2]);
    for(int i=0;i<4;++i)
    {
        for(int j=0;j<2;++j)
        {
            LOG(INFO)<<"playercar"<<i<<"  "<<mpCardLogic->vecCardGroup[i].card[j];
        }
        //openLog("menshu%d  car1=%d  car2=%d",i,mpCardLogic->vecCardGroup[i].card[0],mpCardLogic->vecCardGroup[i].card[1]);
    }

    int64_t winscore=0;
    int    bestChairID=0;
    shared_ptr<strPlalyers> playerInfo;
    m_bankerWinLost=0;
    for(auto& it:m_pPlayerInfo)
    {
        playerInfo=it.second;
        shared_ptr<CServerUserItem>player= mPItableFrame->GetTableUserItem(it.first);
        if (!player || it.first==curBankerChairID) //庄家计总的
            continue;
        for(int j=0;j<GAME_MAXAERA;j++)
        {
            playerInfo->SetPlayerMuticl(j, ThisTurnPlaceMulticle[j]); //set player winlost each door
        }
        //she zhi wanjia bei lv
        playerInfo->Culculate(mPItableFrame,m_bankerWinLost,m_replay,player->IsAndroidUser());
        if(winscore<=playerInfo->ReturnMoney) //record big winer
        {
            winscore=playerInfo->ReturnMoney;
            bestChairID=it.first;
        }
		int64_t allBetScore = playerInfo->dAreaJetton[0] + playerInfo->dAreaJetton[1] + playerInfo->dAreaJetton[2];
		if (allBetScore > 0)
		{
			//对局记录详情
			SetGameRecordDetail(player->GetUserId(), it.first, player->GetUserScore(), playerInfo);
		}
    }
    //计算庄家输赢
    if(curBankerChairID!=-1)
    {
        shared_ptr<CServerUserItem> player= mPItableFrame->GetTableUserItem(curBankerChairID);
        if(player&&m_pPlayerInfo[curBankerChairID])
        {
            if(m_bankerWinLost > 0) //庄家赢
            {
                if(player->GetUserScore()<m_bankerWinLost)
                {
                    m_bankerWinLost=player->GetUserScore();
                    m_pPlayerInfo[curBankerChairID]->usershuishou=mPItableFrame->CalculateRevenue(m_bankerWinLost);
                    m_bankerWinLost-=m_pPlayerInfo[curBankerChairID]->usershuishou;
                    m_pPlayerInfo[curBankerChairID]->ReturnMoney=m_bankerWinLost;

                }
            }else
            {
                if(player->GetUserScore()<-m_bankerWinLost)
                {
                    m_bankerWinLost=-player->GetUserScore();
                    m_pPlayerInfo[curBankerChairID]->ReturnMoney=m_bankerWinLost;

                }
            }
            m_pPlayerInfo[curBankerChairID]->ShuiHouWin=m_bankerWinLost;
			int64_t allBetScore = playerInfo->dAreaJetton[0] + playerInfo->dAreaJetton[1] + playerInfo->dAreaJetton[2];
			if (allBetScore > 0)
			{
				//对局记录详情
				SetGameRecordDetail(player->GetUserId(), curBankerChairID, player->GetUserScore(), m_pPlayerInfo[curBankerChairID]);
			}
        }
        else
        {
            //庄家都没有严重错误
            openLog("庄家都没有严重错误");
            assert(false);
        }
    }

    if(vecPlayerList.size()<20) //record last twenty big winner
    {
         shared_ptr<CServerUserItem>player= mPItableFrame->GetTableUserItem(bestChairID);
         if(player&&m_pPlayerInfo[bestChairID]->ReturnMoney>0)
         {
             shared_ptr<BestPlayerList> pla(new BestPlayerList);
             pla->NikenName=player->GetNickName();
             pla->bestID=player->GetChairId();
             pla->WinScore=m_pPlayerInfo[bestChairID]->ReturnMoney;
             time_t t = time(NULL);

             char ch[64] = {0};

             strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));     //年-月-日 时-分-秒
             pla->onTime=ch;
             //cout<<p->tm_year+1900<<"  "<<p->tm_mon+1<<"  "<<p->tm_mday<<"  "<<p->tm_hour<<
             vecPlayerList.push_back(pla);
         }
    }
    else
    {
        shared_ptr<CServerUserItem>player= mPItableFrame->GetTableUserItem(bestChairID);
        if(player&&m_pPlayerInfo[bestChairID]->ReturnMoney>0)
        {
            shared_ptr<BestPlayerList> pla(new BestPlayerList);
            pla->NikenName=player->GetNickName();
            pla->bestID=player->GetChairId();
            pla->WinScore=m_pPlayerInfo[bestChairID]->ReturnMoney;
            time_t t = time(NULL);

            char ch[64] = {0};

            strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t));     //年-月-日 时-分-秒
            pla->onTime=ch;
            //cout<<p->tm_year+1900<<"  "<<p->tm_mon+1<<"  "<<p->tm_mday<<"  "<<p->tm_hour<<
            vecPlayerList.erase(vecPlayerList.begin());
            vecPlayerList.push_back(pla);
        }
    }
    /////////////////////////

    int64_t winscorearea[3]={0};
    for(auto&it:m_pPlayerInfo){
        playerInfo=it.second;
        shared_ptr<CServerUserItem>User = mPItableFrame->GetTableUserItem(it.first);
        if(!User)
            continue;
        for(int j=0;j<3;j++)
        {
            winscorearea[j]+=playerInfo->dwinscore[j];
        }
    }
//    for(int j=0;j<3;j++)
//    {
//        LOG(INFO)<<"------------winscorearea[j]"<<winscorearea[j];
//    }
    recoredResult();
    ErBaGang::CMD_S_GameEnd gameEnd;
    for(int j=0;j<3;j++)
    {
        gameEnd.add_dallwinscore(winscorearea[j]);
    }
    gameEnd.set_cbplacetime(mgameTimeEnd);
    gameEnd.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);
    for(int j=0;j<GAME_MAXAERA+1;j++)
    {
        for(int k=0;k<2;k++)
        {
            gameEnd.add_cardgroup(mpCardLogic->vecCardGroup[j].card[k]);
            LOG(INFO)<<"NormalCalculate car ="<<mpCardLogic->vecCardGroup[j].card[k];
        }
        if (j<3){
            gameEnd.add_lalljettonscore(mpGameBetArea->mifourAreajetton[j]);
        }
    }
    for(auto &recored :LoadRecored){
        gameEnd.add_tianplace(recored.tianmen);
        gameEnd.add_diplace(recored.dimen);
        gameEnd.add_shunplace(recored.shunmen);
    }
    int listNum=0;
    for(vector<shared_ptr<strPlalyers>>::iterator it1=pPlayerInfoList.begin();it1!=pPlayerInfoList.end()&&listNum<10;it1++)
    {
        shared_ptr<CServerUserItem> pUser=mPItableFrame->GetTableUserItem((*it1)->wCharid);
        if(!pUser) continue;

        ErBaGang::PlayerListItem *item = gameEnd.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_szlocation(pUser->GetLocation());
        item->set_luserscore(pUser->GetUserScore()+(*it1)->ShuiHouWin);
        //LOG(INFO)<<"-------------------USER SCOR "<<pUser->GetUserScore()<<"--------"<<pUser->GetUserId();
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(listNum+1);
        item->set_ltwentywinscore((*it1)->GetTwentyJetton());
        item->set_ltwentywincount((*it1)->GetTwentyWin());
        item->set_lallwinscore((*it1)->ShuiHouWin);
        //item->set_gender(pUser->GetGender());
        for(int j=0;j<3;j++)
        {
            item->add_lwinscore((*it1)->dwinscore[j]);
        }
        listNum++;
    }
    for(int i=0;i<4;i++)
    {
        int mtype = mpCardLogic->vecCardGroup[i].getcardtype();
        //openLog("i=%d type=%d",i,mtype);
        gameEnd.add_cartype(mtype);
    }
    for(int i=1;i<4;i++)
    {
       int mwinindex = mpCardLogic->vecCardGroup[i].comparewithother(mpCardLogic->vecCardGroup[0]);
        gameEnd.add_winindex(mwinindex);
    }
    gameEnd.set_lusermaxscore(0);
    ErBaGang::PlayerListItem *bestuser =gameEnd.mutable_bestuser();
    shared_ptr<CServerUserItem> bestplayer=mPItableFrame->GetTableUserItem(bestChairID);
    if (bestplayer){
        gameEnd.set_mostwinscore(m_pPlayerInfo[bestChairID]->ShuiHouWin);
        int64_t bestplayerchairID = bestplayer->GetChairId();
        miThisTurnBestPlayerUserID=bestplayer->GetUserId();
        miThisTurnBestPlayerChairID=bestChairID;
        miThisTurnBestPlayerWinScore=m_pPlayerInfo[bestChairID]->ShuiHouWin;
        gameEnd.set_bestuserid(bestplayer->GetUserId());
        gameEnd.set_bestusernikename(bestplayer->GetNickName());
        bestuser->set_dwuserid(bestplayer->GetUserId());
        bestuser->set_headerid(bestplayer-> GetHeaderId());
        bestuser->set_nickname(bestplayer->GetNickName());
        bestuser->set_luserscore(bestplayer->GetUserScore()+m_pPlayerInfo[bestChairID]->ShuiHouWin);
        bestuser->set_isdivinemath(0);
        bestuser->set_ltwentywinscore(m_pPlayerInfo[bestplayerchairID]->GetTwentyJetton());
        bestuser->set_ltwentywincount(m_pPlayerInfo[bestplayerchairID]->GetTwentyWin());
        for(int j=0;j<3;j++)
        {
            bestuser->add_lwinscore(m_pPlayerInfo[bestplayerchairID]->dwinscore[j]);
            //LOG(INFO)<<"------------pPlayerList[it->GetChairId()].dwinscore[j]"<<pPlayerList[it->GetChairId()].dwinscore[j];
        }
    }
    gameEnd.set_onlinenum(mPItableFrame->GetPlayerCount(true));
    gameEnd.set_roundid(m_strRoundId);
    gameEnd.set_bankerwinlost(m_bankerWinLost);
    for(auto&it:m_pPlayerInfo){
        playerInfo=it.second;
        shared_ptr<CServerUserItem>User = this->mPItableFrame->GetTableUserItem(it.first);

        if(nullptr == User )
            continue;
        int64_t chairID = User->GetChairId();
//        if(chairID>=EBG_MAX_NUM ||chairID!=i)
//            continue;

        gameEnd.clear_selfjettonscore();
        for(int j=0;j<GAME_MAXAERA;j++)
        {
            gameEnd.add_selfjettonscore(playerInfo->dAreaJetton[j]);
        }
        gameEnd.set_userwinscore(playerInfo->ShuiHouWin);

        bestuser->set_szlocation(User->GetLocation());
        if(User->GetUserId() == miShenSuanZiUserID)
        {
            bestuser->set_isdivinemath(1);
        }
        bestuser->set_index(0);

        ErBaGang::PlayerListItem *selfItem =gameEnd.mutable_self();

        selfItem->set_dwuserid(User->GetUserId());
        selfItem->set_headerid(User-> GetHeaderId());
        selfItem->set_nickname(User->GetNickName());
        selfItem->set_luserscore(User->GetUserScore()+playerInfo->ShuiHouWin);
        selfItem->set_isdivinemath(0);
        selfItem->set_szlocation(User->GetLocation());
        if(User->GetUserId() == miShenSuanZiUserID)
        {
            selfItem->set_isdivinemath(1);
        }
        selfItem->set_index(listNum+1);

        selfItem->set_ltwentywinscore(playerInfo->GetTwentyJetton());
        selfItem->set_ltwentywincount(playerInfo->GetTwentyWin());
        selfItem->set_lallwinscore(playerInfo->ShuiHouWin);
        for(int j=0;j<3;j++)
        {
            selfItem->add_lwinscore(playerInfo->dwinscore[j]);
            //LOG(INFO)<<"------------pPlayerList[it->GetChairId()].dwinscore[j]"<<pPlayerList[it->GetChairId()].dwinscore[j];
        }
        std::string endData = gameEnd.SerializeAsString();
        mPItableFrame->SendTableData(chairID,  ErBaGang::SUB_S_GameEnd, (uint8_t *) endData.c_str(), endData.length());
        //openLog("ErBaGang::SUB_S_GameEnd chairID=%d",chairID);
    }


    if(ResultRecored.size()<9)//tianchong lu dan
    {
        ResultRecored.push_back(miThisTurnResultIndex);
    }
    else
    {
        ResultRecored.erase(ResultRecored.begin());
        ResultRecored.push_back(miThisTurnResultIndex);
    }

}
void TableFrameSink::PlayerRichSorting()
{

    if(pPlayerInfoList.size()>1)
    {
        sort(pPlayerInfoList.begin(),pPlayerInfoList.end(),Comparingconditions); //sort with win count
        sort(++(pPlayerInfoList.begin()),pPlayerInfoList.end(),Comparingconditions1); //sort with jetton socre
    }
    miShenSuanZiUserID=pPlayerInfoList[0]->wUserid; //the big winner in the last twenty games
    miShenSuanZiID=pPlayerInfoList[0]->wCharid;
}

void TableFrameSink::SendPlayerList(int64_t chairid, int64_t subid,const char*  data,int64_t datasize)
{

    if(miGameSceneState==ErBaGang::SUB_S_GameEnd)
    {
        if(vecPlayerList.size()>1)
        {
             ErBaGang::CMD_S_UserWinList PlayerList;
            for(int i=vecPlayerList.size()-2;i>-1;i--)
            {
                shared_ptr<CServerUserItem> player=mPItableFrame->GetTableUserItem(vecPlayerList[i]->bestID);
                if(!player)
                {
                    continue;
                }
                ErBaGang::PlayerInfo* playerinfo=PlayerList.add_player();
                playerinfo->set_winscore(vecPlayerList[i]->WinScore);
                playerinfo->set_wintime(vecPlayerList[i]->onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_szlocation(player->GetLocation());
                LOG(INFO)<<"set_UserId"<<(int)player->GetUserId() ;
                LOG(INFO)<<"set_headerid"<<(int)player->GetHeaderId() ;
                LOG(INFO)<<"set_nickname"<<player->GetNickName();
            }
            string date=PlayerList.SerializeAsString();
            mPItableFrame->SendTableData(chairid, ErBaGang::SUB_S_PLAYERLIST, (uint8_t *)date.c_str(), date.size());
            //openLog("ErBaGang::SUB_S_PLAYERLIST chairID=%d",chairid);
        }
    }
    else
    {
        if(vecPlayerList.size()>0)
        {
             ErBaGang::CMD_S_UserWinList PlayerList;
            for(int i=vecPlayerList.size()-1;i>-1;i--)
            {
                shared_ptr<CServerUserItem> player=mPItableFrame->GetTableUserItem(vecPlayerList[i]->bestID);
                if(!player)
                {
                    continue;
                }
                ErBaGang::PlayerInfo* playerinfo=PlayerList.add_player();
                playerinfo->set_winscore(vecPlayerList[i]->WinScore);
                playerinfo->set_wintime(vecPlayerList[i]->onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_szlocation(player->GetLocation());
                LOG(INFO)<<"set_UserId"<<(int)player->GetUserId() ;
                LOG(INFO)<<"set_headerid"<<(int)player->GetHeaderId() ;
                LOG(INFO)<<"set_nickname"<<player->GetNickName();
            }
            string date=PlayerList.SerializeAsString();
            mPItableFrame->SendTableData(chairid, ErBaGang::SUB_S_PLAYERLIST, (uint8_t *)date.c_str(), date.size());
            //openLog("ErBaGang::SUB_S_PLAYERLIST chairID=%d",chairid);
        }
    }

    LOG(INFO)<<"pai hang bang send" ;
}
bool TableFrameSink::GamePlayerJetton(int64_t chairid, uint8_t cbJettonArea, int64_t lJettonScore, bool bReJetton)
{

        shared_ptr<CServerUserItem> pJettonUser = mPItableFrame->GetTableUserItem(chairid);
        if (!pJettonUser) return false;    // try to return content.
        //if (pSelf->GetUserStatus() != sPlaying) return false;
        /*ErBaGang::CMD_C_PlaceJet placeJet;
        placeJet.ParseFromArray(data,datasize);
        int cbJetArea = placeJet.cbjettonarea();
        int lJetScore = (int)placeJet.ljettonscore();
        cbJetArea=cbJetArea-1;*/

        bool bPlaceJetScuccess = false;
        bool nomoney=false;
        int  errorAsy=0;
        do
        {
            if (ErBaGang::SUB_S_GameJetton != miGameSceneState)
            {
                errorAsy=-1;
                break;
            }
            if(pJettonUser)
            {

                errorAsy=PlayerJettonPanDuan(cbJettonArea, lJettonScore,chairid,pJettonUser);
                if(errorAsy!=0)
                {
                    break;
                }

            }
            bPlaceJetScuccess = true;
        }while(0);

        if (bPlaceJetScuccess)
        {
            if(!pJettonUser->IsAndroidUser())
                m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,pJettonUser->GetChairId(),cbJettonArea+1);
            ErBaGang::CMD_S_PlaceJetSuccess placeJetSucc;
            auto push=[&](int64_t iChairId){
                shared_ptr<CServerUserItem> pSelf=mPItableFrame->GetTableUserItem(iChairId);
                if(!pSelf||pSelf->IsAndroidUser()||iChairId!=pSelf->GetChairId()) return;
                placeJetSucc.Clear();
                int64_t userid = pJettonUser->GetUserId();
                placeJetSucc.set_dwuserid(userid);
                placeJetSucc.set_cbjettonarea(cbJettonArea+1);
                placeJetSucc.set_ljettonscore(lJettonScore);
                placeJetSucc.set_bisandroid(pJettonUser->IsAndroidUser());
                //if (pSelf->IsAndroidUser())
                //{LOG(INFO)<<"android jetton";}

                placeJetSucc.set_luserscore(pJettonUser->GetUserScore()-m_pPlayerInfo[chairid]->dCurrentPlayerJetton);
                 for(int k=0;k<MAX_JETTON_AREA;++k)
                {
                    placeJetSucc.add_alljettonscore(mpGameBetArea->mifourAreajetton[k]);
                    placeJetSucc.add_selfjettonscore(m_pPlayerInfo[iChairId]->dAreaJetton[k]);
                }

                std::string data = placeJetSucc.SerializeAsString();
                mPItableFrame->SendTableData(iChairId, ErBaGang::SUB_S_JETTON_SUCCESS, (uint8_t *)data.c_str(), data.size());
                //openLog("ErBaGang::SUB_S_JETTON_SUCCESS chairID=%d",iChairId);
            };

            ceshifasong++;
            if (! isInRichList(chairid) &&m_pPlayerInfo.size()>8)
            {
                push(chairid);
                AddPlayerJetton(chairid,cbJettonArea, lJettonScore);
            }else //if in rich list push to all soon
            {
                for(auto& it : m_pPlayerInfo){
                    push(it.first);
                }
            }
        }
        else
        {
            ErBaGang::CMD_S_PlaceJettonFail placeJetFail;
            int64_t userid = pJettonUser->GetUserId();
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(cbJettonArea+1);
            placeJetFail.set_lplacescore(lJettonScore);
            placeJetFail.set_cbandroid(pJettonUser->IsAndroidUser());
            switch (errorAsy)
            {
                case SCORE_FILURE: placeJetFail.set_returncode("下注筹码不正确"); break;
                case EXCEED_AREA: placeJetFail.set_returncode("下注区域错误"); break;
                case EXCEED_AREA_SCORE: placeJetFail.set_returncode("下注已达区域上限"); break;
                case SHORT_SCORE: placeJetFail.set_returncode("筹码不足"); break;
                case USER_FAILURE:placeJetFail.set_returncode("无效玩家"); break;
                default:
                    break;
            }
            char buf[1024]={0};
            int bytesize = placeJetFail.ByteSize();
            placeJetFail.SerializeToArray(buf,bytesize);
            mPItableFrame->SendTableData(pJettonUser->GetChairId(), ErBaGang::SUB_S_JETTON_FAIL,(uint8_t *) buf, bytesize);
            //openLog("ErBaGang::SUB_S_JETTON_FAIL chairID=%d",pSelf->GetChairId());
            LOG(INFO)<<"-------------------Wanjia--------xiazhu--------------FAIL"<<errorAsy;
        }
        return true;
}

void TableFrameSink::GamePlayerApplyBanker(int64_t chairid)
{
    shared_ptr<CServerUserItem> pApplyIServerUserItem = mPItableFrame->GetTableUserItem(chairid);
    if (!pApplyIServerUserItem) return;
    if (pApplyIServerUserItem->GetUserStatus() == sLookon) return;
    string str;
    if(m_pPlayerInfo[chairid]&&m_pPlayerInfo[chairid]->bApplyBanker)
    {
        str="您已经在申请上庄队列中";
        SendNotifyToChair(chairid,str);
        return;
    }
    if(curBankerChairID==chairid)
    {
        str="申请上庄失败，您已上庄";
        SendNotifyToChair(chairid,str);
        return;
    }
    if(mpBankerManager->GetApplySize()>=m_applyBankerMaxCount)
    {
        str="上庄列表已满";
        SendNotifyToChair(chairid,str);
        return;
    }
    if(mpBankerManager->ApplyBanker(pApplyIServerUserItem))
    {
        str="申请上庄成功";
        SendNotifyToChair(chairid,str);
        UpdateBankerInfo2Client();
        return;
    }else
    {
        str="上庄失败，金币不足";
        SendNotifyToChair(chairid,str);
        return;
    }
}
void TableFrameSink::GamePlayerCancelBanker(int64_t chairid)
{
    shared_ptr<CServerUserItem> pICancelIServerUserItem = mPItableFrame->GetTableUserItem(chairid);
    if(pICancelIServerUserItem&&m_pPlayerInfo[chairid])
    {
        string  str;
        if(curBankerChairID==chairid)
        {
            str="申请下庄成功，本局结束您将下庄";
            SendNotifyToChair(chairid,str);
            mpBankerManager->SetContinueBanker(false);
            return;
        }

        if(!m_pPlayerInfo[chairid]->bApplyBanker)
        {
            str="取消下庄失败,您不在上庄队列";
            SendNotifyToChair(chairid,str);
            return ;
        }
        m_pPlayerInfo[chairid]->bApplyBanker=false;
        mpBankerManager->CancelBanker(pICancelIServerUserItem);

        str="取消下庄成功";
        SendNotifyToChair(chairid,str);
        UpdateBankerInfo2Client();
    }
}

void TableFrameSink::SendNotifyToChair(int64_t chairid, string& message)
{
    ErBaGang::CMD_S_Server_Notity notify;
    notify.set_notifymessage(message);
    std::string lstData = notify.SerializeAsString();
    mPItableFrame->SendTableData(chairid, ErBaGang::SUB_S_SERVER_NOTIFY,(uint8_t *)lstData.c_str(),lstData.size());
}

void TableFrameSink::UpdateBankerInfo2Client(int64_t chairid)
{
    mpBankerManager->UpdateBankerInfo(m_bankerInfo);
    ErBaGang::CMD_S_UPDATE_BANKER updateBanker;
    ErBaGang::BankerInfo*bankerInfo= updateBanker.mutable_bankerinfo();
    bankerInfo->CopyFrom(*m_bankerInfo);
    string lstData=updateBanker.SerializeAsString();
    mPItableFrame->SendTableData(chairid,ErBaGang::SUB_S_UPDATE_BANKER_INFO,(uint8_t *)lstData.c_str(),lstData.size());
}


void TableFrameSink::AddPlayerJetton(int64_t chairid, int areaid, int score)
{
    assert(chairid<EBG_MAX_NUM&&areaid<GAME_MAXAERA);

    m_arr_jetton[chairid].bJetton =true;
    m_arr_jetton[chairid].iAreaJetton[areaid] +=score;
    m_iarr_tmparea_jetton[areaid]+=score;
}

void TableFrameSink::Rejetton(int64_t chairid, int64_t subid,const char*  data,int64_t datasize)
{
    shared_ptr<CServerUserItem>Usr=mPItableFrame->GetTableUserItem(chairid);
    if(!Usr)
    {
		LOG(WARNING) << "----查找玩家不在----";
        return;
    }
	LOG(WARNING) << "-----------Rejetton续押-------------";
	// 游戏状态判断
	if (ErBaGang::SUB_S_GameJetton != miGameSceneState)
	{
		LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << miGameSceneState << " " << ErBaGang::SUB_S_GameJetton;
		return;
	}
    int64_t alljetton = 0;
    int64_t areajetton[GAME_MAXAERA] = { 0 };
	int64_t userid = Usr->GetUserId();

	ErBaGang::CMD_C_ReJetton reJetton;
	reJetton.ParseFromArray(data, datasize);
    int reSize = reJetton.ljettonscore_size();
	for (int i = 0;i < reSize;i++)
    {
        m_pPlayerInfo[chairid]->dLastJetton[i] = reJetton.ljettonscore(i);
		if (m_pPlayerInfo[chairid]->dLastJetton[i] < 0)
		{
			LOG(ERROR) << "---续押押分有效性检查出错--->" << i << " " << m_pPlayerInfo[chairid]->dLastJetton[i];
		}
        LOG(WARNING) << "---------续押最大押分dLastJetton =================="<<m_pPlayerInfo[chairid]->dLastJetton[i];
	}
	for (int i = 0; i < GAME_MAXAERA; i++)
	{
		alljetton += m_pPlayerInfo[chairid]->dLastJetton[i];
		areajetton[i] = m_pPlayerInfo[chairid]->dLastJetton[i];
		LOG(WARNING) << "--dLastJetton-" << areajetton[i];
		if (areajetton[i] < 0)
		{
			alljetton = 0;
			break;
		}
	}

	// 续押失败
	if (alljetton == 0 || Usr->GetUserScore() < alljetton)
	{
		ErBaGang::CMD_S_PlaceJettonFail placeJetFail;

		/*placeJetFail.set_dwuserid(userid);
		placeJetFail.set_cbjettonarea(3);
		placeJetFail.set_lplacescore(0);
		placeJetFail.set_cberrorcode(4);
		placeJetFail.set_cbandroid(Usr->IsAndroidUser());*/
		placeJetFail.set_dwuserid(userid);
		placeJetFail.set_cbjettonarea(0);
		placeJetFail.set_lplacescore(alljetton);
		placeJetFail.set_cbandroid(Usr->IsAndroidUser());
		/*switch (errorAsy)
		{
		case SCORE_FILURE: placeJetFail.set_returncode("下注筹码不正确"); break;
		case EXCEED_AREA: placeJetFail.set_returncode("下注区域错误"); break;
		case EXCEED_AREA_SCORE: placeJetFail.set_returncode("下注已达区域上限"); break;
		case SHORT_SCORE: placeJetFail.set_returncode("筹码不足"); break;
		case USER_FAILURE:placeJetFail.set_returncode("无效玩家"); break;
		default:
			break;
		}*/
		placeJetFail.set_returncode("筹码不足");

		std::string sendData = placeJetFail.SerializeAsString();
		mPItableFrame->SendTableData(Usr->GetChairId(), ErBaGang::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
		LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
		return;
	}

	for (int i = 0; i < GAME_MAXAERA; i++)
	{
		// 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
		if (AreaMaxJetton[i] < m_pPlayerInfo[chairid]->dAreaJetton[i] + m_pPlayerInfo[chairid]->dLastJetton[i])
		{
			ErBaGang::CMD_S_PlaceJettonFail placeJetFail;
			/*placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i);
			placeJetFail.set_lplacescore(m_pPlayerInfo[chairid]->dLastJetton[i]);
			placeJetFail.set_cberrorcode(5);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());*/
			placeJetFail.set_dwuserid(userid);
			placeJetFail.set_cbjettonarea(i + 1);
			placeJetFail.set_lplacescore(m_pPlayerInfo[chairid]->dLastJetton[i]);
			placeJetFail.set_cbandroid(Usr->IsAndroidUser());
			/*switch (errorAsy)
			{
			case SCORE_FILURE: placeJetFail.set_returncode("下注筹码不正确"); break;
			case EXCEED_AREA: placeJetFail.set_returncode("下注区域错误"); break;
			case EXCEED_AREA_SCORE: placeJetFail.set_returncode("下注已达区域上限"); break;
			case SHORT_SCORE: placeJetFail.set_returncode("筹码不足"); break;
			case USER_FAILURE:placeJetFail.set_returncode("无效玩家"); break;
			default:
				break;
			}*/
			placeJetFail.set_returncode("下注已达区域上限");

			std::string data = placeJetFail.SerializeAsString();
			//if (m_bTest==0)
			mPItableFrame->SendTableData(Usr->GetChairId(), ErBaGang::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
			LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << AreaMaxJetton[i] << " < " << m_pPlayerInfo[chairid]->dAreaJetton[i] << "+" << m_pPlayerInfo[chairid]->dLastJetton[i];
			return;
		}
	}
	// LOG(ERROR) << "---------重押成功--------";
	for (int i = 0; i < GAME_MAXAERA; i++)
	{
		if (areajetton[i] <= 0)
		{
			continue;
		}

		while (areajetton[i] > 0)
		{
			int jSize = 5;
			for (int j = jSize - 1;j >= 0;j--)
			{
				if (areajetton[i] >= m_currentchips[chairid][j])
				{
                    int64_t tmpVar = m_currentchips[chairid][j];
					GamePlayerJetton(chairid, i, tmpVar, true);
					areajetton[i] -= tmpVar;
					break;
				}
			}
            if(areajetton[i]<m_currentchips[chairid][0])
            {
                 LOG(WARNING) << "---------续押有问题areajetton=================="<<areajetton[i]<<" 最小筹码是"<<m_currentchips[chairid][0];
                break;
            }
		}
	}
}
void TableFrameSink::GamePlayerQueryPlayerList(int64_t chairId, uint8_t subid, const char* data, int64_t datasize,bool IsEnd)
{
    shared_ptr<CServerUserItem> pIIServerUserItem = mPItableFrame->GetTableUserItem(chairId);
    if (!pIIServerUserItem) return;

    pPlayerInfoList.clear();
    for(auto& it:m_pPlayerInfo)
        pPlayerInfoList.push_back(it.second);
    //get the new list
    PlayerRichSorting();

    ErBaGang::CMD_C_PlayerList querylist;
    querylist.ParseFromArray(data, datasize);
    int64_t limitcount = querylist.nlimitcount();
    int64_t beginindex = querylist.nbeginindex();
    int64_t resultcount = 0;
    int64_t endindex = 0;
    ErBaGang::CMD_S_PlayerList playerlist;
    int64_t wMaxPlayer = mPItableFrame->GetMaxPlayerCount();
    if (!limitcount) limitcount = wMaxPlayer;
    int index=0;

    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end();it++)
    {
        shared_ptr<CServerUserItem> pUser=this->mPItableFrame->GetTableUserItem((*it)->wCharid);
        if(!pUser) continue;
        ErBaGang::PlayerListItem* item = playerlist.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_szlocation(pUser->GetLocation());
        if(IsEnd)
        {
            item->set_luserscore(pUser->GetUserScore()-(*it)->dCurrentPlayerJetton);
        }
        else
        {
            item->set_luserscore(pUser->GetUserScore());
        }
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(index);
        item->set_ltwentywinscore((*it)->GetTwentyJetton());
        item->set_ltwentywincount((*it)->GetTwentyWin());

        endindex=index+1;
        index++;
        resultcount++;
        if (resultcount >= limitcount)
        {
            break;
        }
    }
    playerlist.set_nendindex(endindex);
    playerlist.set_nresultcount(resultcount);
    std::string lstData = playerlist.SerializeAsString();
    mPItableFrame-> SendTableData(pIIServerUserItem->GetChairId(), ErBaGang::SUB_S_QUERY_PLAYLIST,(uint8_t *)lstData.c_str(),lstData.size());

}
void TableFrameSink::SendGameSceneStart(int64_t staID,bool lookon)
{
    shared_ptr<CServerUserItem> player=mPItableFrame->GetUserItemByUserId(staID);
    if(!player) return;
    ErBaGang::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime(mgameTimeStart);
    scenceStart.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);

    for(int i=0;i<5;i++)
    {
        scenceStart.add_userchips(m_currentchips[player->GetChairId()][i]);
    }
    //LOG(INFO)<<"shijain---------------------------"<<(int32_t)time(NULL)-mfGameTimeLeftTime;
    int64_t dAlljetton=0.0;
    for(int i=0;i<GAME_MAXAERA;i++)
    {
        ErBaGang::CMD_AeraInfo *areainfo=scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(mpGameBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(playerMuiltcle[i]);
        dAlljetton+=m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i];
    }
    for(int i=ResultRecored.size()-1;i>-1;i--)
    {
        scenceStart.add_cartype(ResultRecored[i]);
    }
    for(int i=0;i<GAME_MAXAERA+1;i++)
    {
        for(int j=0;j<2;j++)
        {
            scenceStart.add_cardgroup(mpCardLogic->vecCardGroup[i].card[j]);
        }
    }
    for(int i=0;i<2;i++)
    {
        scenceStart.add_shuaizi(mpCardLogic->shuaizi[i]);
    }
    int listNum=0;

    //LOG(INFO)<<"************SendGameSceneStart pPlayerInfoList.size="<<(int)pPlayerInfoList.size();

    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end()&&listNum<10;it++)
    {
        shared_ptr<CServerUserItem> pUser=mPItableFrame->GetTableUserItem((*it)->wCharid);
        if(!pUser ||(*it)->wCharid!=pUser->GetChairId()) continue;

        ErBaGang::PlayerListItem *item = scenceStart.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_luserscore(pUser->GetUserScore());

        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_szlocation(pUser->GetLocation());
        item->set_index(listNum+1);
        item->set_ltwentywinscore((*it)->GetTwentyJetton());
        item->set_ltwentywincount((*it)->GetTwentyWin());


        listNum++;
    }
    LOG(INFO)<<"wanjia list "<<listNum;
    scenceStart.set_maxbet(AreaMaxJetton[0]);
    scenceStart.set_timetype("bet");
    scenceStart.set_onlinenum(mPItableFrame->GetPlayerCount(true));
    ErBaGang::BankerInfo* bankerinfo=scenceStart.mutable_bankerinfo();
    bankerinfo->CopyFrom(*m_bankerInfo);

    ErBaGang::PlayerListItem *self =scenceStart.mutable_self();
    self->set_dwuserid(player->GetUserId());
    self->set_headerid(player-> GetHeaderId());
    self->set_nickname(player->GetNickName());
    self->set_luserscore(player->GetUserScore()-dAlljetton);
    self->set_isdivinemath(0);
    self->set_szlocation(player->GetLocation());
    if(player->GetUserId() == miShenSuanZiUserID)
    {
        self->set_isdivinemath(1);
    }
    self->set_index(listNum+1);
    self->set_ltwentywinscore(m_pPlayerInfo[player->GetChairId()]->GetTwentyJetton());
    self->set_ltwentywincount(m_pPlayerInfo[player->GetChairId()]->GetTwentyWin());
    for(auto& recored :LoadRecored)
    {
        scenceStart.add_shunplace(recored.shunmen);
        scenceStart.add_tianplace(recored.tianmen);
        scenceStart.add_diplace(recored.dimen);
    }
    scenceStart.set_roundid(m_strRoundId);

	//限红
	for (int i = 0;i < 2;i++)
	{
		scenceStart.add_llimitscore(m_lLimitScore[i]);
	}
	scenceStart.set_tableid(m_strTableId);

    std::string data = scenceStart.SerializeAsString();
    mPItableFrame->SendTableData(player->GetChairId(), ErBaGang::SUB_S_SCENE_START, (uint8_t *)data.c_str(), data.size());
    //openLog("ErBaGang::SUB_S_SCENE_START chairID=%d",player->GetChairId());
}

void TableFrameSink::SendGameSceneJetton(int64_t staID,bool lookon)
{
    shared_ptr<CServerUserItem> player=mPItableFrame->GetUserItemByUserId(staID);
    if(!player) return;
    ErBaGang::CMD_S_Scene_GameJetton scenceStart;

    for(int i=0;i<5;i++)
    {
        scenceStart.add_userchips(m_currentchips[player->GetChairId()][i]);
    }
    scenceStart.set_cbplacetime(mgameTimeJetton);
    scenceStart.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);
    int64_t dAlljetton=0.0;
    for(int i=0;i<GAME_MAXAERA;i++)
    {
        ErBaGang::CMD_AeraInfo *areainfo=scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(mpGameBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(playerMuiltcle[i]);
        dAlljetton+=m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i];
        //LOG(INFO)<<"selfjettonscore = "<<m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i];
    }
    for(int i=ResultRecored.size()-1;i>-1;i--)
    {
        scenceStart.add_cartype(ResultRecored[i]);
    }
    for(int i=0;i<GAME_MAXAERA+1;i++)
    {
        for(int j=0;j<2;j++)
        {
            scenceStart.add_cardgroup(mpCardLogic->vecCardGroup[i].card[j]);
        }
    }
    for(int i=0;i<2;i++)
    {
        scenceStart.add_shuaizi(mpCardLogic->shuaizi[i]);
    }
    int listNum=0;
    int xzScore=0;
    //LOG(INFO)<<"************SendGameSceneStart pPlayerInfoList.size="<<(int)pPlayerInfoList.size();
    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end()&&listNum<10;it++)
    {
        shared_ptr<CServerUserItem> pUser=this->mPItableFrame->GetTableUserItem((*it)->wCharid);
        if(!pUser) continue;

        ErBaGang::PlayerListItem *item = scenceStart.add_players();

        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_luserscore(pUser->GetUserScore()-(*it)->dCurrentPlayerJetton);
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
            for(int i =0; i<GAME_MAXAERA;++i){
               scenceStart.add_shensuanzijettonflag((*it)->dAreaJetton[i]>0 ? 1 : 0);
            }

        }
        item->set_szlocation(pUser->GetLocation());
        item->set_index(listNum+1);
        item->set_ltwentywinscore((*it)->GetTwentyJetton());
        item->set_ltwentywincount((*it)->GetTwentyWin());

        listNum++;

    }
    LOG(INFO)<<"wanjia list "<<listNum;
    scenceStart.set_maxbet(AreaMaxJetton[0]);
    scenceStart.set_timetype("bet");
    scenceStart.set_onlinenum(mPItableFrame->GetPlayerCount(true));

    ErBaGang::PlayerListItem *self =scenceStart.mutable_self();
    self->set_dwuserid(player->GetUserId());
    self->set_headerid(player-> GetHeaderId());
    self->set_nickname(player->GetNickName());
    self->set_luserscore(player->GetUserScore()-m_pPlayerInfo[player->GetChairId()]->dCurrentPlayerJetton);
    self->set_isdivinemath(0);
    self->set_szlocation(player->GetLocation());
    if(player->GetUserId() == miShenSuanZiUserID)
    {
        self->set_isdivinemath(1);
    }
    self->set_index(listNum+1);
    self->set_ltwentywinscore(m_pPlayerInfo[player->GetChairId()]->GetTwentyJetton());
    self->set_ltwentywincount(m_pPlayerInfo[player->GetChairId()]->GetTwentyWin());
    scenceStart.set_roundid(m_strRoundId);

    ErBaGang::BankerInfo* bankerinfo=scenceStart.mutable_bankerinfo();
    bankerinfo->CopyFrom(*m_bankerInfo);

    for(auto& recored :LoadRecored)
    {
        scenceStart.add_shunplace(recored.shunmen);
        scenceStart.add_tianplace(recored.tianmen);
        scenceStart.add_diplace(recored.dimen);
    }

	//限红
	for (int i = 0;i < 2;i++)
	{
		scenceStart.add_llimitscore(m_lLimitScore[i]);
	}
	scenceStart.set_tableid(m_strTableId);

    std::string data = scenceStart.SerializeAsString();
    mPItableFrame->SendTableData(player->GetChairId(), ErBaGang::SUB_S_SCENE_Jetton, (uint8_t *)data.c_str(), data.size());
    //openLog("ErBaGang::SUB_S_SCENE_Jetton chairID=%d",player->GetChairId());
}
void TableFrameSink::SendGameSceneEnd(int64_t staID,bool lookon)
{

    shared_ptr<CServerUserItem> player=mPItableFrame->GetUserItemByUserId(staID);
    if(!player) return;
    ErBaGang::CMD_S_Scene_GameEnd scenceEnd;

    for(int i=0;i<5;i++)
    {
        scenceEnd.add_userchips(m_currentchips[player->GetChairId()][i]);
    }
    scenceEnd.set_cbplacetime(mgameTimeEnd);
    scenceEnd.set_cbtimeleave((int32_t)time(NULL)-mfGameTimeLeftTime);
    LOG(INFO)<<"shijain---------------------------"<<(int32_t)time(NULL)-mfGameTimeLeftTime;
    for(int i=0;i<GAME_MAXAERA;i++)
    {
        ErBaGang::CMD_AeraInfo *areainfo=scenceEnd.add_aerainfo();
        areainfo->set_selfjettonscore(m_pPlayerInfo[player->GetChairId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(mpGameBetArea->mifourAreajetton[i]);
        areainfo->set_mutical(playerMuiltcle[i]);
    }
    for(int i=ResultRecored.size()-1;i>-1;i--)
    {
        scenceEnd.add_cartype(ResultRecored[i]);
    }

    for(int i=0;i<GAME_MAXAERA+1;i++)
    {
        for(int j=0;j<2;j++)
        {
            scenceEnd.add_cardgroup(mpCardLogic->vecCardGroup[i].card[j]);
        }
    }
    for(int i=0;i<2;i++)
    {
        scenceEnd.add_shuaizi(mpCardLogic->shuaizi[i]);
    }
    int listNum=0;
    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end()&&listNum<10;it++)
    {
        shared_ptr<CServerUserItem> pUser=this->mPItableFrame->GetTableUserItem((*it)->wCharid);
        if(!pUser) continue;

        ErBaGang::PlayerListItem *item = scenceEnd.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser-> GetHeaderId());
        item->set_nickname(pUser->GetNickName());
        item->set_luserscore(pUser->GetUserScore());
        item->set_isdivinemath(0);
        if(pUser->GetUserId() == miShenSuanZiUserID)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(listNum+1);
        item->set_ltwentywinscore((*it)->GetTwentyJetton());
        item->set_ltwentywincount((*it)->GetTwentyWin());

        for(int j=0;j<3;j++)
        {
            item->add_lwinscore((*it)->dwinscore[j]);
        }
        listNum++;
    }
    scenceEnd.set_maxbet(AreaMaxJetton[0]);
    scenceEnd.set_timetype("End");
    scenceEnd.set_onlinenum(mPItableFrame->GetPlayerCount(true));
    ErBaGang::PlayerListItem *self =scenceEnd.mutable_self();
    self->set_dwuserid(player->GetUserId());
    self->set_headerid(player-> GetHeaderId());
    self->set_nickname(player->GetNickName());
    self->set_luserscore(player->GetUserScore());
    self->set_isdivinemath(0);
    if(player->GetUserId() == miShenSuanZiUserID)
    {
        self->set_isdivinemath(1);
    }
    self->set_index(listNum+1);
    self->set_ltwentywinscore(m_pPlayerInfo[player->GetChairId()]->GetTwentyJetton());
    self->set_ltwentywincount(m_pPlayerInfo[player->GetChairId()]->GetTwentyWin());

    for(int j=0;j<3;j++)
    {
        self->add_lwinscore(m_pPlayerInfo[player->GetChairId()]->dwinscore[j]);
    }
    for(auto& recored :LoadRecored)
    {
        scenceEnd.add_shunplace(recored.shunmen);
        scenceEnd.add_tianplace(recored.tianmen);
        scenceEnd.add_diplace(recored.dimen);
    }
    scenceEnd.set_roundid(m_strRoundId);

    ErBaGang::BankerInfo* bankerinfo=scenceEnd.mutable_bankerinfo();
    bankerinfo->CopyFrom(*m_bankerInfo);

	//限红
	for (int i = 0;i < 2;i++)
	{
		scenceEnd.add_llimitscore(m_lLimitScore[i]);
	}
	scenceEnd.set_tableid(m_strTableId);

    std::string data = scenceEnd.SerializeAsString();
    mPItableFrame->SendTableData(player->GetChairId(), ErBaGang::SUB_S_SCENE_END, (uint8_t *)data.c_str(), data.size());
    //openLog("ErBaGang::SUB_S_SCENE_END chairID=%d",player->GetChairId());
}
//check is in the top 10
bool TableFrameSink::isInRichList(int64_t chairid)
{
    int listNum=0;
    for(vector<shared_ptr<strPlalyers>>::iterator it=pPlayerInfoList.begin();it!=pPlayerInfoList.end()&&listNum<10;it++)
    {
        if (chairid == (*it)->wCharid)
            return true;
        listNum++;
    }
    return false;
}
int TableFrameSink::PlayerJettonPanDuan(int index,int64_t score,int64_t chairid,shared_ptr<CServerUserItem>& pSelf)
{
    if(index<0||index>=GAME_MAXAERA||score<=0)
    {
        return EXCEED_AREA;
    }
    bool bValid =false;
    for(int i=0;i<5;i++)
    {
        if(m_currentchips[chairid][i]==score)
        {
            bValid = true;
            break;
        }
    }
//    for (auto& i : mPItableFrame->GetGameRoomInfo()->jettons){
//        if (i == score)
//        {
//            bValid= true;
//            break;
//        }
//    }
    if(!bValid)
    {
        LOG(INFO)<<"ErBaGang::SUB_S_JETTON_FAIL lJetScore="<<score;
        return SCORE_FILURE;
    }
    //shared_ptr<CServerUserItem> play=mPItableFrame->GetTableUserItem(chairid);
    if(!pSelf)
    {
         return USER_FAILURE;
    }
//    if (m_pPlayerInfo[chairid]->dAreaJetton[index]+score >AreaMaxJetton[index]){
//        return EXCEED_AREA_SCORE;
//    }

//    if(!pSelf->IsAndroidUser())
//    {
//        LOG(INFO)<<"jetton chairid"<<chairid<<"score ="<<score;
//    }
    int64_t allScore=0.0;
    for(int i=0;i<GAME_MAXAERA;i++)
    {
        allScore+= m_pPlayerInfo[chairid]->dAreaJetton[i];
    }
    if (m_pPlayerInfo[chairid]->dAreaJetton[index]+score>AreaMaxJetton[index]/*mPItableFrame->GetGameRoomInfo()->maxJettonScore*/)
        return EXCEED_AREA_SCORE;

    if(pSelf->GetUserScore()-allScore-score<0)
    {
        LOG(INFO)<<"user score not enough";
        return SHORT_SCORE;
    }
    m_pPlayerInfo[chairid]->dAreaJetton[index]+=score;
    m_pPlayerInfo[chairid]->dCurrentPlayerJetton=m_pPlayerInfo[chairid]->dAreaJetton[0]+m_pPlayerInfo[chairid]->dAreaJetton[1]+m_pPlayerInfo[chairid]->dAreaJetton[2] ;//+m_pPlayerInfo[chairid]->dAreaJetton[3];
    mpGameBetArea->SetJettonScore(index,score,mpBankerManager->GetBankerScore(),pSelf->IsAndroidUser());
    if( allScore == 0 && !pSelf->IsAndroidUser())
    {
        m_replay.addPlayer(pSelf->GetUserId(),pSelf->GetAccount(),pSelf->GetUserScore(),chairid);
    }
    return 0;
}
void  TableFrameSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//erbagang//erbagang%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,mPItableFrame->GetGameRoomInfo()->roomId, mPItableFrame->GetTableId());
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
     return;
    }
   va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] [TABLEID:%d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,mPItableFrame->GetTableId());
   vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}
void TableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    for(auto &it :m_pPlayerInfo){
        shared_ptr<CServerUserItem> pUserItem=mPItableFrame->GetTableUserItem(it.first);
        if(!pUserItem)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (m_pPlayerInfo[it.first]->NotBetJettonCount>=5)
        {
            if(it.second->bApplyBanker)
                mpBankerManager->CancelBanker(pUserItem);
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(pUserItem->GetChairId());            
            mPItableFrame->BroadcastUserStatus(pUserItem, true);
            mPItableFrame->ClearTableUser(pUserItem->GetChairId(),true,true);
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<it.first<<" userId="<<it.second->wUserid;          
        } 
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_pPlayerInfo.erase(clearUserIdVec[i]);
    }
}

//设置当局游戏详情
void TableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<strPlalyers> userInfo)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
	int32_t iMulticle = 1;
	uint8_t cardData[2] = { 0 };
	ErBaGang::CMD_S_GameRecordDetail details;
	details.set_gameid(mPItableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->ShuiHouWin); //总输赢积分
	//系统开牌
	ErBaGang::SysCards* card = details.mutable_syscard();
	for (int i = 0;i < 2;i++)
	{
		cardData[i] = mpCardLogic->vecCardGroup[0].card[i];
	}
	card->set_cards(&(cardData)[0], 2);
	card->set_ty(mpCardLogic->vecCardGroup[0].getcardtype());
	card->set_multi(iMulticle);

	//if (!mPItableFrame->IsExistUser(chairId))
	{
		//[0:顺;1:天;2:地] 
		for (int betIdx = 0;betIdx < GAME_MAXAERA; betIdx++)
		{
			// 结算时记录
            int64_t winScore = userInfo->dRealwinscore[betIdx];
			int64_t betScore = userInfo->dAreaJetton[betIdx];
			int32_t	nMuilt = iMulticle;// 倍率表m_nMuilt
			uint8_t cardDatas[2] = { 0 };
			//if (betScore > 0) 
			{
				ErBaGang::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//手牌数据
                ErBaGang::SysCards* handcard = detail->mutable_handcard();
				for (int i = 0;i < 2;i++)
				{
					cardDatas[i] = mpCardLogic->vecCardGroup[betIdx + 1].card[i];
				}
				handcard->set_cards(&(cardDatas)[0], 2);
				handcard->set_ty(mpCardLogic->vecCardGroup[betIdx + 1].getcardtype());
				handcard->set_multi(iMulticle);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
				//本区域是否赢[0:否;1:赢]
				detail->set_dareaiswin(ThisTurnPlaceMulticle[betIdx]);
			}
		}
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}

void TableFrameSink::updateGameMsg(bool isInit)
{
	if (!isInit)
		m_TimerLoopThread->getLoop()->cancel(m_TimerIdUpdateGameMsg);

	Json::FastWriter writer;
	Json::Value gameMsg;

	gameMsg["tableid"] = mPItableFrame->GetTableId(); //桌子ID
	gameMsg["gamestatus"] = miGameSceneState; //游戏状态
	gameMsg["time"] = getGameCountTime(miGameSceneState); //倒计时
	//gameMsg["goodroutetype"] = m_nGoodRouteType; //好路类型
	//gameMsg["routetypename"] = m_strGoodRouteName; //好路名称

	string gameinfo = writer.write(gameMsg);

	Json::Value details;
	Json::Value countMsg;
	Json::Value routeMsg;
	Json::Value routeMsgItem;

	//details["routetype"] = m_nGoodRouteType;//好路类型
	//countMsg["t"] = m_winOpenCount[0];//和数量
	//countMsg["b"] = m_winOpenCount[1];//龙数量
	//countMsg["p"] = m_winOpenCount[2];//虎数量
	//countMsg["tt"] = m_winOpenCount[3];//和对数量
	//countMsg["bt"] = m_winOpenCount[4];//庄对数量
	//countMsg["pt"] = m_winOpenCount[5];//闲对数量
	countMsg["player"] = mPItableFrame->GetPlayerCount(true);//桌子人数

	details["count"] = countMsg;//各开奖次数
	//details["routetypename"] = m_strGoodRouteName; //好路名称
	details["limitmin"] = m_lLimitScore[0];//限红低
	details["limitmax"] = m_lLimitScore[1];//限红高

	//开奖记录
	routeMsg = Json::arrayValue;
	routeMsgItem = Json::arrayValue;
	/*if (m_openRecord.size() > 0)
	{
		for (int i = 0;i < m_openRecord.size();i++)
		{
			routeMsg.append(m_openRecord[i]);
		}
	}*/
	for (auto &recored : LoadNewRecored) {
		routeMsgItem.clear();
		routeMsgItem.append(recored.shunmen);
		routeMsgItem.append(recored.tianmen);
		routeMsgItem.append(recored.dimen);
		routeMsg.append(routeMsgItem);
	}
	details["route"] = routeMsg;

	string detailsinfo = writer.write(details);
	/*GlobalFunc::replaceChar(detailsinfo, '\\n');
	GlobalFunc::replaceChar(detailsinfo, '\\');*/
	//LOG(INFO) << "updateGameMsg gameinfo " << gameinfo << " detailsinfo " << detailsinfo;
	// 写数据库
	mPItableFrame->CommonFunc(eCommFuncId::fn_id_1, gameinfo, detailsinfo);
	if (!isInit)
		m_TimerIdUpdateGameMsg = m_TimerLoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, boost::bind(&TableFrameSink::updateGameMsg, this, false));
}

//获取游戏状态对应的剩余时间
int32_t TableFrameSink::getGameCountTime(int gameStatus)
{
	int countTime = 0;
	int32_t leaveTime = 0;
	int32_t allTime = 0;
	//chrono::system_clock::time_point gameStateStartTime;
	int32_t gameStateStartTime = 0;
	if (gameStatus == ErBaGang::SUB_S_GameStart) // 发牌时间时间
	{
		allTime = mgameTimeStart;
		gameStateStartTime = mfGameTimeLeftTime;
	}
	else if (gameStatus == ErBaGang::SUB_S_GameJetton) //下注时间
	{
		allTime = mgameTimeJetton;
		gameStateStartTime = mfGameTimeLeftTime;
	}
	else //结束时间
	{
		allTime = mgameTimeEnd;
		gameStateStartTime = mfGameTimeLeftTime;
	}
	//chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - gameStateStartTime);
	//leaveTime = allTime - durationTime.count();
	leaveTime = allTime - ((int32_t)time(NULL) - gameStateStartTime);
	countTime = (leaveTime > 0 ? leaveTime : 0);

	return countTime;
}

void TableFrameSink::RefreshPlayerList(bool isGameEnd)
{
	//m_pPlayerInfoList_20.clear();
	//shared_ptr<IServerUserItem> pServerUserItem;
	//for (int i = 0;i < m_pPlayerInfoList.size();i++)
	//{
	//	if (!m_pPlayerInfoList[i])
	//		continue;
	//	pServerUserItem = mPItableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
	//	if (!pServerUserItem)
	//		continue;
	//	m_pPlayerInfoList[i]->GetTwentyWinScore();
	//	m_pPlayerInfoList[i]->RefreshGameRusult(isGameEnd);
	//}

	//for (int i = 0;i < m_pPlayerInfoList.size();i++)
	//{
	//	if (!m_pPlayerInfoList[i])
	//		continue;
	//	pServerUserItem = mPItableFrame->GetTableUserItem(m_pPlayerInfoList[i]->chairId);
	//	if (!pServerUserItem)
	//		continue;
	//	m_pPlayerInfoList_20.push_back((*m_pPlayerInfoList[i]));
	//	/*LOG(ERROR) << "old二十局赢分:" << (m_pPlayerInfoList[i]->GetTwentyWinScore());
	//	LOG(ERROR) << "二十局赢分:" << (m_pPlayerInfoList_20[i].GetTwentyWinScore());*/
	//}
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);

	if (LoadNewRecored.size() >= 14)
		LoadNewRecored.erase(LoadNewRecored.begin());

	LoadNewRecored.push_back(m_thisResult);

	//m_openRecord.push_back(m_nWinIndex);

	/*m_nGoodRouteType = 0;
	m_strGoodRouteName = "";
	m_nGoodRouteType = mpCardLogic.getGoodRouteType(m_openRecord);
	m_strGoodRouteName = mpCardLogic.getGoodRouteName(m_nGoodRouteType);*/

	//m_bRefreshPlayerList = true;
	//PlayerRichSorting();
	//游戏记录
	//updateResultList(m_nWinIndex);
	//下注项累计开奖的次数
	//m_winOpenCount[m_nWinIndex - 1]++;
	
	/*string stsRsult = "";
	for (int i = 0;i < GAME_MAXAERA;i++)
	{
		stsRsult += to_string(i) + "_" + to_string(m_winOpenCount[i]) + " ";
	}
	openLog("开奖结果累计 stsRsult = %s", stsRsult.c_str());*/
	//更新开奖结果
	//memcpy(m_notOpenCount_last, m_notOpenCount, sizeof(m_notOpenCount_last));

	/*if (isGameEnd && m_bGameEnd)
	{
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<strPlalyers> userInfoItem;
		for (auto &it : m_pPlayerInfo)
		{
			userInfoItem = it.second;
			pUserItem = mPItableFrame->GetTableUserItem(userInfoItem->wCharid);
			if (!pUserItem)
				continue;
			SendOpenRecord(userInfoItem->wCharid);
		}
	}*/
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<TableFrameSink> pTableFrameSink(new TableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
    pSink.reset();
}
