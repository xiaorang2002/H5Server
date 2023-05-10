#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

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
#include "proto/hhmf.Message.pb.h"

#include "betarea.h" 
// #include "Algorithmc.h" 
#include "HhmfAlgorithm.h"
#include "TableFrameSink.h"

#include "json/json.h"

//宏定义
#ifndef _UNICODE
#define myprintf    _snprintf
#else
#define myprintf    swprintf
#endif

#define RESULT_WIN (1)
#define RESULT_LOST (-1)

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
        google::InitGoogleLogging("Jsys");
#else
        dir += "/hhmf";
        google::InitGoogleLogging("hhmf");
#endif //__HJSDS__

        FLAGS_log_dir = dir;
        // set std output special log content value.
        google::SetStderrLogging(google::GLOG_INFO);
        mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }

    virtual ~CGLogInit()
    {
        // google::ShutdownGoogleLogging();
    }
};

CGLogInit g_loginit; // 声明全局变局,初始化库.

int64_t TableFrameSink::m_llStorageControl = 0;
int64_t TableFrameSink::m_llStorageLowerLimit = 0;
int64_t TableFrameSink::m_llStorageHighLimit = 0;
int64_t TableFrameSink::m_llStorageAverLine = 0;
int64_t TableFrameSink::m_llGap = 0;

uint8_t TableFrameSink::m_cbTableCard[54*8] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
};
//库存加上
TableFrameSink::TableFrameSink()
{
    m_lBlackBetLimit = 0;
    m_lControlAndLoseLimit = 0;
    m_bBetStatus = 0;
    testIsOpen =false;
    stockWeak = 1.0;
    m_bIsClearUser = false; 
    m_nReadCount = 0;
    m_MarqueeScore = 1000;
    m_bIsIncludeAndroid = 0;
	m_vOpenResultListCount.clear();
	for (int i = 0;i < BET_ITEM_COUNT;i++)
	{
        vector<OpenResultInfo> vResultList;
		m_vOpenResultListCount[i] = vResultList;
	}
    m_fUpdateGameMsgTime=0;
    m_vOpenResultListCount_last.clear();
    for (auto &it : m_vOpenResultListCount)
    {
        m_vOpenResultListCount_last[it.first] = it.second;
    }
    m_vResultRecored.clear();
	// 初始化20个随机结果
    m_CardVecData.clear();
    m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
    random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

    memset(m_iHhmfwNum, 0, sizeof(m_iHhmfwNum));
    for (int i = 0; i < OPEN_RESULT_LIST_COUNT; i++)
    {
        strResultInfo result;
        uint8 card = m_CardVecData[m_random.betweenInt(0,(int)m_CardVecData.size()-1).randInt_mt(true)];
        result.setTouList(card);

//        int32_t cardColor=logic.GetCardColor(card);
//        m_iHhmfwNum[cardColor]++;

        shared_ptr<OpenResultInfo> iResultIndex(new OpenResultInfo());
        iResultIndex->init(result);

        m_vResultRecored.push_back(result);
        updateResultList(result);
	}


    m_lastRetId.reset();

    m_vPlayerList.clear(); 
    m_pPlayerInfoList.clear(); 
	m_pPlayerInfoList_20.clear();
    m_bIsOpen = false; 
    m_pITableFrame = nullptr;
    m_EapseTime = (int32_t)time(nullptr);
    mfGameTimeLeftTime=(int32_t)time(NULL);
    m_iGameState = GAME_STATUS_INIT;
    m_strRoundId = "";
    
    m_pBetArea.reset(new BetArea());
    
    m_UserInfo.clear();
    m_iRetMul = 20;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        m_lOpenResultIndexCount[i] = 0;
    }

    // m_LoopThread.startLoop();

    m_bIsContinue = true;
    m_fTMJettonBroadCard = 0.5f;
    m_testInex=-1;
    m_testResultInex = -1;
	m_testInex_sd = -1;
	m_testResultInex_sd = -1;
    m_lOpenResultCount = 0;
    m_bTest = 0;
    m_nTestTimes = 0;
    m_bWritelog = false;
    m_nTestType = 0;
    m_bTestAgain = false;
    m_dControlkill=0;
    m_lLimitPoints=0;
    m_iStopIndex = 0;
    for(int i=0;i<MAX_PLAYER;i++)
    {
        m_currentchips[i][0]=100;
        m_currentchips[i][1]=500;
        m_currentchips[i][2]=1000;
        m_currentchips[i][3]=5000;
        m_currentchips[i][4]=10000;
    }
	m_ShenSuanZiId = 0;
	memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));
    m_fGameStartAniTime = 1.0;
	m_fGameEndAniTime = 1.0;
	m_iResultId_sd = 0;

	m_fResetPlayerTime = 5.0f;
	m_bGameEnd = false;
	m_ctrlGradePercentRate[0] = 600;
	m_ctrlGradePercentRate[1] = 900;
	m_ctrlGradeMul[0] = 50;
	m_ctrlGradeMul[1] = 25;
    m_iRoundCount = 0;
    m_strTableId = "";
    memset(m_lLimitScore, 0, sizeof(m_lLimitScore));
}
TableFrameSink::~TableFrameSink()
{
    m_pITableFrame = nullptr;
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_LoopThread->getLoop()->cancel(m_TimerIdTest);
	m_LoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
}

void TableFrameSink::OnGameStart()
{
    LOG(WARNING) << "On Game Start " << m_pGameRoomInfo->roomName << " " << m_pGameRoomInfo->roomId;
}

void TableFrameSink::OnEventGameEnd(int32_t chairid, uint8_t nEndTag)
{
    LOG_INFO << "On Event On Game End ";
}
bool TableFrameSink::OnEventGameScene(uint32_t chairid, bool bisLookon)
{
    LOG_INFO << "On Event On Event Game Scene ";
}

bool TableFrameSink::OnEventGameConclude(uint32_t chairid, uint8_t nEndTag)
{
    return false;
}

bool TableFrameSink::CanJoinTable(shared_ptr<CServerUserItem> &pUser)
{
    // 分数不够不能进
    // if(pUser->GetUserScore() < m_pGameRoomInfo->floorScore ){
    //     LOG(WARNING)<< "UserScore "<< pUser->GetUserScore() << " floorScore " << m_pGameRoomInfo->floorScore;

    //     return false;
    // }
    return true;
}

bool TableFrameSink::CanLeftTable(int64_t userId)
{
    LOG(WARNING) << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if (!pIServerUserItem)
    {
        LOG(WARNING) << " pIServerUserItem==nullptr Left UserId:" << userId;
        return true;
    }
    else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->wCharid;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            if (userInfo->Isplaying)
                totalxz = userInfo->dCurrentPlayerJetton;
        }

        if(totalxz > 0)
            canLeft = false;

    }
    string strtttt = (canLeft?"可以":"不可以") ;
    LOG(WARNING) << "****玩家"<< strtttt <<"离开桌子***[" << pIServerUserItem->GetChairId() << "] " << pIServerUserItem->GetUserId() <<" "<< pIServerUserItem->GetAccount();

    return canLeft;
}
//获取游戏状态对应的剩余时间
int32_t TableFrameSink::getGameCountTime(int gameStatus)
{
    int countTime = 0;
    int32_t leaveTime = 0;
    int32_t allTime = 0;
    //chrono::system_clock::time_point gameStateStartTime;
    int32_t gameStateStartTime = 0;
    if (gameStatus == hhmf::SUB_S_GameStart) // 发牌时间时间
    {
        allTime = m_betCountDown;
        gameStateStartTime = mfGameTimeLeftTime;
    }
    else
    {
        allTime = m_endCountDown;
        gameStateStartTime = mfGameTimeLeftTime;
    }
    leaveTime = allTime - ((int32_t)time(NULL) - gameStateStartTime);
    countTime = (leaveTime > 0 ? leaveTime : 0);

    return countTime;
}
void TableFrameSink::updateGameMsg(bool isInit)
{
    if (!isInit)
        m_LoopThread->getLoop()->cancel(m_TimerIdUpdateGameMsg);

    Json::FastWriter writer;
    Json::Value gameMsg;

    gameMsg["tableid"] = m_pITableFrame->GetTableId(); //桌子ID
    gameMsg["gamestatus"] = m_iGameState; //游戏状态
    gameMsg["time"] = getGameCountTime(m_iGameState); //倒计时
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
    countMsg["player"] = m_pITableFrame->GetPlayerCount(true);//桌子人数

    details["count"] = countMsg;//各开奖次数
    //details["routetypename"] = m_strGoodRouteName; //好路名称
    details["limitmin"] = int32_t(m_lLimitScore[0]);//限红低
    details["limitmax"] = int32_t(m_lLimitScore[1]);//限红高

    //开奖记录
    routeMsg = Json::arrayValue;
    routeMsgItem = Json::arrayValue;

    for (auto &recored : m_vResultRecored) {
        routeMsgItem.clear();
        routeMsgItem.append(recored.icard);
        routeMsg.append(routeMsgItem);
    }
    details["route"] = routeMsg;

    string detailsinfo = writer.write(details);
    /*GlobalFunc::replaceChar(detailsinfo, '\\n');
    GlobalFunc::replaceChar(detailsinfo, '\\');*/
    //LOG(INFO) << "updateGameMsg gameinfo " << gameinfo << " detailsinfo " << detailsinfo;
    // 写数据库
    m_pITableFrame->CommonFunc(eCommFuncId::fn_id_1, gameinfo, detailsinfo);
    if (!isInit)
        m_TimerIdUpdateGameMsg = m_LoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, boost::bind(&TableFrameSink::updateGameMsg, this, false));
}

bool TableFrameSink::OnUserEnter(int64_t userid, bool islookon)
{ 
    shared_ptr<CServerUserItem> m_pPlayer = m_pITableFrame->GetUserItemByUserId(userid);
    if (!m_pPlayer)
    {
        LOG(ERROR) << "**********Get UserItem By UserId error**********" << userid;
        return false;
    }

    if(m_pPlayer->IsAndroidUser()){
        //LOG(WARNING) << "*********机器人进来了**********" << userid;
    }

    int32_t wCharid = m_pPlayer->GetChairId();

    if (wCharid < 0 || wCharid >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a WrongchairID"  << "   " << wCharid;
        return false;
    }    
    // 
    m_pPlayer->setUserSit();

    //LOG(WARNING) << "********** User Enter **********[" << wCharid << "] " << userid <<" "<< m_pPlayer->GetAccount();

    auto it = m_UserInfo.find(userid);
    if(m_UserInfo.end() == it)
    {
        shared_ptr<UserInfo> userInfo(new UserInfo());
        userInfo->Isplaying = true;
        userInfo->wCharid = wCharid;
        userInfo->wUserid = userid;
		userInfo->headerId = m_pPlayer->GetHeaderId();

        m_UserInfo[userid] = userInfo;
    }

    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
       if(m_pPlayer->GetUserScore()>= m_userScoreLevel[0]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[1])
       {
           m_currentchips[wCharid][i]=m_userChips[0][i];
       }
       else if(m_pPlayer->GetUserScore()>= m_userScoreLevel[1]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[2])
       {
           m_currentchips[wCharid][i]=m_userChips[1][i];
       }
       else if(m_pPlayer->GetUserScore()>= m_userScoreLevel[2]&&
               m_pPlayer->GetUserScore()< m_userScoreLevel[3])
       {
           m_currentchips[wCharid][i]=m_userChips[2][i];
       }
       else
       {
           m_currentchips[wCharid][i]=m_userChips[3][i];
       }
    }

	for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
	{
        if ((*it)->wUserid == userid)
		{
			m_pPlayerInfoList.erase(it);
			m_pPlayerInfoList.push_back(m_UserInfo[userid]);
			break;
		}
	}
    for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
	{
        if ((*it).wUserid == userid)
		{
			m_pPlayerInfoList_20.erase(it);
            m_pPlayerInfoList_20.push_back((*m_UserInfo[userid]));
			break;
		}
	}
	
    // 启动游戏
    if(!m_bIsOpen&&m_iGameState==GAME_STATUS_INIT)
    { 
        // mpBankerManager->UpdateBankerData(); //一开始设置系统庄家
		m_ShenSuanZiId = m_pPlayer->GetUserId();
        m_iShenSuanZiChairId = wCharid;
        m_iCurrBankerId = -1;  
        m_iShenSuanZiUserId = m_pPlayer->GetUserId();
        m_bIsOpen = true;
        m_EapseTime = (int32_t)time(nullptr);

        m_startTime = chrono::system_clock::now();
        m_dwReadyTime = (uint32_t)time(nullptr);
        m_strRoundId = m_pITableFrame->GetNewRoundId();
        m_replay.clear();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
        m_replay.gameinfoid = m_strRoundId;


        string ip = m_pITableFrame->GetGameRoomInfo()->serverIP;
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
        m_strTableId = to_string(m_ipCode) + "-" + to_string(m_pITableFrame->GetTableId() + 1);	//桌号        
        hhmfAlgorithm.SetGameRoomName(m_strTableId);
        mfGameTimeLeftTime=(int32_t)time(NULL);
        // 修改miGameSceneState
        m_iGameState = hhmf::SUB_S_GameStart;

        m_TimerIdUpdateGameMsg = m_LoopThread->getLoop()->runAfter(m_fUpdateGameMsgTime, CALLBACK_0(TableFrameSink::updateGameMsg, this, false));
        // 启动定时器
        GamefirstInGame();

        // if (m_bTest>0)
        // {
        //     m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        //     m_TimerIdTest=m_LoopThread->getLoop()->runEvery(0.04, boost::bind(&TableFrameSink::TestGameMessage, this,wCharid,m_nTestType));
        //     openSLog("本次测试%d开始;",m_nTestTimes);
        //     openSLog("开始库存:High Limit:%d,Lower Limit:%d,,Aver Line:%d",m_llStorageHighLimit,m_llStorageLowerLimit,m_llStorageControl);
        //     m_bTestAgain = true;
        // } 
    }

    // 发送场景消息
    switch (m_iGameState)
    {
    case hhmf::SUB_S_GameStart:
    {
        SendGameSceneStart(userid, false);
        break;
    }
    case hhmf::SUB_S_GameEnd:
    {
        SendGameSceneEnd(userid, false);
        break;
    }
    default:
    {
        LOG(ERROR) << "*********发送场景消息 状态错误**********" << m_iGameState;
    }
        break;
    }

    LOG(WARNING) << "********** User Enter 2 **********" << m_iGameState;

    return true;
}
bool TableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    return true;
}
bool TableFrameSink::OnUserLeft(int64_t userid, bool islookon)
{ 
    shared_ptr<CServerUserItem> user = m_pITableFrame->GetUserItemByUserId(userid);
    if (!user)
        return false;

    bool bClear = false;
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userid];
    uint32_t chairId = userInfo->wCharid;
    if (chairId < 0 || chairId >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a Wrong chair ID" << "   " << (int)chairId;
    }
    if (m_bIsClearUser)
    {
        m_UserInfo.erase(userid);
        m_pITableFrame->ClearTableUser(chairId);
        bClear = true;
    }
    else if (!user->IsAndroidUser() && userInfo->dCurrentPlayerJetton <= 0)
    {
        m_UserInfo.erase(userid);
        m_pITableFrame->ClearTableUser(chairId);
        bClear = true;
    }

    LOG(WARNING) << "**********玩家离开 3**********[" << chairId << "] " << userid;
    if (userid == m_ShenSuanZiId)
	{
		m_ShenSuanZiId = 0;
	}
	if (bClear)
	{
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
			if ((*it)->wUserid == userid)
			{
                m_pPlayerInfoList.erase(it);
				break;
			}
		}
        for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
            if ((*it).wUserid == userid)
			{
				m_pPlayerInfoList_20.erase(it);
				break;
			}
		}
	}
    return bClear;
}

bool TableFrameSink::SetTableFrame(shared_ptr<ITableFrame> &pTableFrame)
{
    assert(nullptr != pTableFrame);
    if (pTableFrame == nullptr)
        return false;

    m_pITableFrame = pTableFrame;
    m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();

    // 获取EventLoopThread
    m_LoopThread = m_pITableFrame->GetLoopThread();
    m_replay.clear(); 
    m_replay.cellscore = m_pGameRoomInfo->floorScore; 
    m_replay.roomname = m_pGameRoomInfo->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
    ReadStorageInfo();
    //初始化游戏数据
    // m_wGameStatus = GAME_STATUS_INIT;

    LOG(WARNING) << "Set Table Frame " << m_pGameRoomInfo->roomName << " " << m_pGameRoomInfo->roomId << " " << m_pGameRoomInfo->tableCount;

    OnGameStart(); // m_pITableFrame->onStartGame();

    // Algorithmc::Instence()->InitAlgor(m_pITableFrame->GetMaxPlayerCount(), m_pGameRoomInfo->tableCount, m_pGameRoomInfo->roomId);

    m_UserInfo.clear();
    // 读配置
    ReadInI();
    srand((int)time(nullptr));
	/*string str = "";
	int textNum = 0;
	for (int i=0;i<200;i++)
	{
		textNum = m_random.betweenInt(0, 2).randInt_mt(true);
		str += to_string(textNum) + "  ";
	}
	LOG(INFO) << "m_random.betweenInt(0, 2):" << str;
	openSLog("m_random.betweenInt(0, 2):：%s", str.c_str());*/
    return true;
}

void TableFrameSink::GamefirstInGame()
{
	getJSodds();
    // 启动定时器
    //float betCountDown = m_betCountDown + m_fGameStartAniTime; //1s,预留前端的开始动画
    m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(m_betCountDown, boost::bind(&TableFrameSink::GameTimerEnd, this));

    m_timetickAll = m_betCountDown;
    m_timetick = 0;
    // 测试
    // m_TimerIdTest = m_LoopThread->getLoop()->runEvery(1, [&](){
    //     // LOG(WARNING) << "Tick---> " << (m_timetickAll - (m_timetick++)) ;
    // });

    LOG(WARNING) << "--- 启动桌子---" << __FUNCTION__ << " " << m_iGameState;
}

void TableFrameSink::GameTimerStart()
{
    LOG(WARNING) << "--- 开始游戏 1---" << __FUNCTION__;
    //连续5局未下注踢出房间
    //CheckKickOutUser();
    // 设置游戏状态
    m_iGameState = hhmf::SUB_S_GameStart;
    m_pITableFrame->SetGameStatus(hhmf::SUB_S_GameStart);
	m_bGameEnd = false;
    m_timetick = 0;
    //float betCountDown = m_betCountDown + m_fGameStartAniTime; //1s,预留前端的开始动画
    m_timetickAll = m_betCountDown;

    mfGameTimeLeftTime=(int32_t)time(NULL);
    m_startTime = chrono::system_clock::now();
    m_dwReadyTime = (uint32_t)time(nullptr);
    m_strRoundId = m_pITableFrame->GetNewRoundId();

    m_replay.clear();
    m_replay.cellscore = m_pGameRoomInfo->floorScore;
    m_replay.roomname = m_pGameRoomInfo->roomName;
    m_replay.gameinfoid = m_strRoundId;

    m_iRoundCount++; //统计局数
    vector<uint64_t> clearUserIdVec;
    vector<uint32_t> clearChairIdVec;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if (!pUserItem)
            continue;
        // 踢出离线玩家
        if (pUserItem->GetUserStatus() == sOffline)
        {
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
            LOG(INFO) << "踢出离线玩家 "<< userInfoItem->wCharid << "  " <<  pUserItem->GetUserId();
            continue;
        }
        if (!userInfoItem->Isplaying || pUserItem->IsAndroidUser())
        {
            continue;
        }
        LOG(WARNING) << "user=" << pUserItem->GetUserId() << " bet1=" << userInfoItem->dAreaJetton[0] << " bet2=" << userInfoItem->dAreaJetton[1] << " bet3=" << userInfoItem->dAreaJetton[2]
                     << " bet4=" << userInfoItem->dAreaJetton[3] << " bet5=" << userInfoItem->dAreaJetton[5] << " bet6=" << userInfoItem->dAreaJetton[5] << " bet7=" << userInfoItem->dAreaJetton[6]
                     << " bet8=" << userInfoItem->dAreaJetton[7];

        LOG(WARNING) << "user=" << pUserItem->GetUserId() << " playereWin=" << userInfoItem->m_winAfterTax;
    }
    uint32_t size = clearUserIdVec.size();
    for (int i = 0; i < size; ++i)
    {
        m_pITableFrame->ClearTableUser(clearChairIdVec[i]);
        m_UserInfo.erase(clearUserIdVec[i]);
		//for (auto it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); ++it)
		for (vector<shared_ptr<UserInfo>>::iterator it = m_pPlayerInfoList.begin(); it != m_pPlayerInfoList.end(); it++)
		{
            if (clearUserIdVec[i] == (*it)->wUserid)
			{
				m_pPlayerInfoList.erase(it);
				break;
			}
		}
        for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
		{
            if (clearUserIdVec[i] == (*it).wUserid)
			{
				m_pPlayerInfoList_20.erase(it);
				break;
			}
		}
    }

    // 记录当前时间
    m_EapseTime = time(nullptr);

    // 跑马灯显示
    // BulletinBoard();
    LOG(ERROR) << "游戏开始1111111111111111111111111111111111111111111111111111111111111";
	RefreshPlayerList();
    //clear Table
    ResetTableDate();
    // 服务器没启动踢出玩家
    if (m_pGameRoomInfo->serverStatus !=SERVER_RUNNING)
    {
        clearUserIdVec.clear();
        clearChairIdVec.clear();
        for (auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
            if(!pUserItem)
                continue;
            pUserItem->setOffline();
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
        }
        uint32_t size = clearUserIdVec.size();
        for (int i = 0; i < size; ++i)
        {
            m_pITableFrame->ClearTableUser(clearChairIdVec[i],true, true, ERROR_ENTERROOM_SERVERSTOP);
            m_UserInfo.erase(clearUserIdVec[i]);
        }

        m_bIsOpen = false;
        m_bIsContinue = true;
        m_LoopThread->getLoop()->cancel(m_TimerIdTest);
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
        LOG(WARNING) << "m_pGameRoomInfo->serverStatus !=SERVER_RUNNING === " << m_pGameRoomInfo->serverStatus << "SERVER_RUNNING="<<SERVER_RUNNING;
        return;
    }
    // 向客户端发送游戏开始命令
    NormalGameStart();
    //开始启动一段时间内的相对自己的其他所有玩家的押注总额,发送给客户端
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    m_TimerOtherBetJetton = m_LoopThread->getLoop()->runEvery(m_fTMJettonBroadCard,boost::bind(&TableFrameSink::OnTimerSendOtherBetJetton, this));
    LOG(WARNING)<< "----------当前房间总人数  " << m_pITableFrame->GetPlayerCount(true);
    // 清除玩家标志
    m_bIsClearUser = false;

    // 服务器状态判断
    if (m_bIsContinue)
    {
        LOG(WARNING) << "--- 启动定时器 开始押分 CallBack GameTimerEnd---";
        m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
        // if (m_bTest>0)
        //     m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(0.05, boost::bind(&TableFrameSink::GameTimerEnd, this));
        // else
            m_bBetStatus = 1;
            m_TimerIdEnd = m_LoopThread->getLoop()->runAfter(m_betCountDown, boost::bind(&TableFrameSink::GameTimerEnd, this));
            m_TimerIdJetton = m_LoopThread->getLoop()->runAfter(0.1, boost::bind(&TableFrameSink::GameTimerJetton, this));
    }
    else
    {
        clearUserIdVec.clear();
        clearChairIdVec.clear();
        for (auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
            if(!pUserItem || !userInfoItem->Isplaying)
                continue;
            pUserItem->setOffline();
            clearUserIdVec.push_back(userInfoItem->wUserid);
            clearChairIdVec.push_back(userInfoItem->wCharid);
        }
        uint32_t size = clearUserIdVec.size();
        for (int i = 0; i < size; ++i)
        {
            m_pITableFrame->ClearTableUser(clearChairIdVec[i], true, true, ERROR_ENTERROOM_SERVERSTOP);
            m_UserInfo.erase(clearUserIdVec[i]);
        }
    }
    m_bIsContinue = false;
}
void TableFrameSink::GameTimerJetton()
{
    //m_bBetStatus = 1;
}
void TableFrameSink::GameTimerEnd()
{
    LOG(WARNING) << "--- 游戏结束---" << __FUNCTION__;
    // 更新状态
    m_iGameState = hhmf::SUB_S_GameEnd;
    m_bBetStatus =0;
    m_pITableFrame->SetGameStatus(hhmf::SUB_S_GameEnd);
    m_timetick = 0;
    //float endCountDown = m_endCountDown + m_fGameEndAniTime; //1s,预留前端播放动画
    m_LoopThread->getLoop()->cancel(m_TimerIdEnd);
    m_LoopThread->getLoop()->cancel(m_TimerOtherBetJetton);
    OnTimerSendOtherBetJetton();

    mfGameTimeLeftTime=(int32_t)time(NULL);
    // muduo::MutexLockGuard lock(m_mutexLock);
    m_EapseTime = time(nullptr);

    // 结算计算
    NormalCalculate();
    m_bIsClearUser = true;
    // 跑马灯显示
    BulletinBoard();
    m_bIsContinue = m_pITableFrame->ConcludeGame(m_iGameState);
    LOG(WARNING) << "--- 游戏结束 m_bIsContinue is True?---" << m_bIsContinue;
    LOG(WARNING) << "--- 启动结算定时器---"; 

    m_TimerIdStart = m_LoopThread->getLoop()->runAfter(m_endCountDown, CALLBACK_0(TableFrameSink::GameTimerStart, this));
    m_TimerIdResetPlayerList = m_LoopThread->getLoop()->runAfter(m_fResetPlayerTime, CALLBACK_0(TableFrameSink::RefreshPlayerList, this,true));

    //动态读取配置文件
    ReadInI();
    
    //统计本局的库存累加值
    double res = 0.0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if(pUserItem->IsAndroidUser())
            continue;
        res -= userInfoItem->m_winAfterTax; 
    }
    LOG(INFO)<<"update Num----------------- "<<res;
    tagStorageInfo ceshi;
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update before----------------- "<<ceshi.lEndStorage;
    LOG(INFO)<<"update Num----------------- "<<res;
    if(res>0)
    {
        res=res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    m_pITableFrame->UpdateStorageScore(res);
    memset(&ceshi, 0, sizeof(ceshi));
    m_pITableFrame->GetStorageScore(ceshi);
    LOG(INFO)<<"update after ----------------- "<<ceshi.lEndStorage;    
    
    // 重新更新库存
    ReadStorageInfo();

	openSLog("库存值:High Limit:%d,Lower Limit:%d,Stock:%d", m_llStorageHighLimit, m_llStorageLowerLimit, m_llStorageControl);
	openSLog("本局游戏结束;\n");
	// if (m_bTest > 0 && m_bTestAgain > 0)
	// {
	// 	if ((m_nTestType == 1 || m_nTestType == 2) && m_llStorageControl <= 5500000 * COIN_RATE) //放分
	// 	{
	// 		m_bTestAgain = false;
	// 		openSLog("=============库存下降为550W后再测试500局开始;");
	// 		m_nTestTimes = 500;
	// 		m_lOpenResultCount = 0;
	// 		for (int i = 0; i < BET_ITEM_COUNT; ++i)
	// 		{
	// 			m_lOpenResultIndexCount[i] = 0;
	// 		}
	// 	}
	// 	else if ((m_nTestType == 3 || m_nTestType == 4) && m_llStorageControl >= 5000000 * COIN_RATE) //收分
	// 	{
	// 		m_bTestAgain = false;
	// 		openSLog("=============库存上升到500W后再测试500局开始;");
	// 		m_nTestTimes = 500;
	// 		m_lOpenResultCount = 0;
	// 		for (int i = 0; i < BET_ITEM_COUNT; ++i)
	// 		{
	// 			m_lOpenResultIndexCount[i] = 0;
	// 		}
	// 	}
	// }
}
void TableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo){
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (userInfoItem->NotBetJettonCount>=5)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(userInfoItem->wUserid);            
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);            
            m_pITableFrame->ClearTableUser(userInfoItem->wCharid,true,true);
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->wCharid<<" userId="<<userInfoItem->wUserid;          
        } 
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_UserInfo.erase(clearUserIdVec[i]);
    }
}

void TableFrameSink::ReadStorageInfo()
{
    tagStorageInfo storageInfo;
    m_pITableFrame->GetStorageScore(storageInfo);

    m_llStorageControl = storageInfo.lEndStorage;
    m_llStorageLowerLimit = storageInfo.lLowlimit;
    m_llStorageHighLimit = storageInfo.lUplimit;
    m_llGap = (m_llStorageHighLimit-m_llStorageLowerLimit) >> 1;
    m_llStorageAverLine = (m_llStorageHighLimit+m_llStorageLowerLimit) >> 1;
    if(m_llGap < 0)
    {
        LOG(ERROR) << "Error Limit...";
    }else{
        LOG(INFO) << "High Limit" << m_llStorageHighLimit << ",Lower Limit:" << m_llStorageLowerLimit << ",Aver Line"<<m_llStorageAverLine
                  << ", Gap" << m_llGap << ", Stock:" << m_llStorageControl;
    }
}
//测试游戏数据
void TableFrameSink::TestGameMessage(uint32_t wChairID,int type)
{
    // 游戏状态判断
    if (hhmf::SUB_S_GameStart != m_iGameState)
    {
       return;
    }
    int betAllScore[11] = {0,100,100,100,100,100,100,4500,5500,100,5000};
    int tmpbetAllScore = betAllScore[m_nTestType]*COIN_RATE;
    //每次下注到一个5倍的区域   
    int32_t bJettonArea = 0;//RandomInt(0, 3);
    if (m_nTestType>=7)
    {
        bJettonArea = RandomInt(10, 11);
    }
    do
    {                
        if (m_nTestType<7)
        {
            if (m_nTestType%2==1)
            {
                bJettonArea = RandomInt(0, 9);
            }
            else
            {
                bJettonArea = RandomInt(10, 11);
            }
        }
        
        //随机可下注金额
        int64_t JettonScore=0,canGold = 0,TotalWeightGold = 0;
        int randn = 0;
        int jsize = m_pGameRoomInfo->jettons.size();
        for (int i = jsize-1; i >= 0; --i)
        {
            if(m_pGameRoomInfo->jettons[i]>tmpbetAllScore) 
                continue;
            randn = i;//RandomInt(0, i);
            canGold = m_pGameRoomInfo->jettons[randn];
            tmpbetAllScore -= canGold;
            break; 
        }
        //随机下注筹码
        JettonScore = canGold;

        LOG(WARNING) << "----随机下注金额---" << JettonScore << " " << bJettonArea ;

        hhmf::CMD_C_PlaceJet mPlaceJet;
        mPlaceJet.set_cbjettonarea(bJettonArea);
        mPlaceJet.set_ljettonscore(JettonScore);

        string content = mPlaceJet.SerializeAsString();
        OnGameMessage(wChairID, hhmf::SUB_C_USER_JETTON, (uint8_t *)content.data(), content.size());
    }
    while(tmpbetAllScore>0);
    
    

    GameTimerEnd();
}

bool TableFrameSink::OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{ 
    if (chairid < 0 || chairid >= MAX_PLAYER)
    {
        LOG(ERROR) << "**********This is a WrongchairID"  << "   " << (int)chairid;
        return false;
    }
    switch (subid)
    {
        case hhmf::SUB_C_USER_JETTON: //玩家押注
        {
            GamePlayerJetton(chairid, subid, data, datasize);
            break;
        }
        case hhmf::SUB_C_USER_ASKLIST:
        {
            SendPlayerList(chairid, subid, data, datasize); //排行榜
            break;
        }
        case hhmf::SUB_C_USER_REPEAT_JETTON:
        {
            Rejetton(chairid, subid, data, datasize); //续押
            break;
        }
        case hhmf::SUB_C_QUERY_PLAYERLIST: //查询玩家列表
        {
            GamePlayerQueryPlayerList(chairid, subid, data, datasize); //wan jia lie biao
            break;
        }
        case hhmf::SUB_C_QUERY_CUR_STATE: //获取当前状态
        {
            QueryCurState(chairid, subid, data, datasize);
            break;
        }
        case hhmf::SUB_C_QUERY_GAMERECORD: //100局获取开奖结果
		{
			QueryGameRecord(chairid, subid, data, datasize);
			break;
		}
    }
    return true;
}

// 查询当前游戏状态
void TableFrameSink::QueryCurState(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{ 
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
    if (!player)
        return;
    hhmf::CMD_C_CurState curStateReq;
    curStateReq.ParseFromArray(data, datasize);

    int32_t userid = curStateReq.dwuserid();

    int32_t elsTimeTest = ((int32_t)time(nullptr) - m_EapseTime);
    LOG(WARNING) << "**********查询当前游戏状态**********" << userid <<" "<<elsTimeTest;

    hhmf::CMD_S_CurState curStateRsp;

    int32_t iEapseTime = m_betCountDown;
    if(hhmf::SUB_S_GameEnd == m_iGameState)
    {
        iEapseTime = m_endCountDown;
    }
    curStateRsp.set_ncurretidx(m_iStopIndex); 
    curStateRsp.set_ncurstate((int)(hhmf::SUB_S_GameStart != m_iGameState)); //当前状态
    curStateRsp.set_cbtimeleave(iEapseTime - elsTimeTest);
    LOG(WARNING) << "****" <<iEapseTime;

    int betCount = 0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || userInfoItem->wCharid != chairid || !userInfoItem->Isplaying)
            continue;
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            betCount += userInfoItem->dAreaJetton[j];
            curStateRsp.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            curStateRsp.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
        } 
    }
    LOG(ERROR) << "查询当前游戏状态 =>UserScore=" << player->GetUserScore() << ",betCount= "<< betCount;
    curStateRsp.set_luserscore(player->GetUserScore() - betCount);
    curStateRsp.set_userwinscore(userInfoItem->RealMoney);

    hhmf::ResultStr * lastresult = curStateRsp.mutable_nlastresult();   
    lastresult->set_ndanshuang(m_lastRetId.iDanshuang);
    lastresult->set_ndaxiao(m_lastRetId.iDaxiao);
    lastresult->set_ncard(m_lastRetId.icard);
    lastresult->set_nhhmf(m_lastRetId.iHhmf);
    lastresult->set_nhonghei(m_lastRetId.iHonghei);

    for(int i=0;i<eMax;i++)
    {   if(m_lastRetId.bReturnMoney[i])
        {
            lastresult->add_nwinmutic(0);
        }
        else if(m_lastRetId.muticlList[i])
        {
            lastresult->add_nwinmutic(m_lastRetId.muticlList[i]);
        }
        else
        {
            lastresult->add_nwinmutic(-1);
        }
    }

    //记录
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {

        hhmf::ResultStr * sResultList = curStateRsp.add_sresultlist();
        sResultList->set_ndanshuang(m_vResultRecored[i].iDanshuang);
        sResultList->set_ndaxiao(m_vResultRecored[i].iDaxiao);
        sResultList->set_ncard(m_vResultRecored[i].icard);
        sResultList->set_nhhmf(m_vResultRecored[i].iHhmf);
        sResultList->set_nhonghei(m_vResultRecored[i].iHonghei);

        for(int j=0;j<eMax;j++)
        {   if(m_vResultRecored[i].bReturnMoney[j])
            {
                sResultList->add_nwinmutic(0);
            }
            else if(m_vResultRecored[i].muticlList[j])
            {
                sResultList->add_nwinmutic(m_vResultRecored[i].muticlList[j]);
            }
            else
            {
                sResultList->add_nwinmutic(-1);
            }
        }
    }
    ///////////////////////////////////
    /// \brief PlayerListItem
    int i = 0;
    int64_t xzScore = 0;
    int64_t nowScore = 0;
    shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &uInfoItem : m_pPlayerInfoList)
    {
        xzScore = 0;
        pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
        if (!pServerUserItem)
            nowScore = 0;
        else
        {
            for (int j = 0; j < BET_ITEM_COUNT; j++)
            {
                xzScore += uInfoItem->dAreaJetton[j];
            }
            nowScore = pServerUserItem->GetUserScore() - xzScore;
        }

        if (uInfoItem->headerId > 12)
        {
            LOG_INFO << " ========== 5 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
        }
        hhmf::PlayerListItem* palyer = curStateRsp.add_players();
        palyer->set_dwuserid(uInfoItem->wUserid);
        palyer->set_headerid(uInfoItem->headerId);
        palyer->set_nviplevel(0);
        palyer->set_nickname(uInfoItem->nickName);

        LOG(ERROR)<<" 传递的玩家列表分数 "<<nowScore;
        palyer->set_luserscore(nowScore);

        palyer->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
        palyer->set_ltwentywincount(uInfoItem->lTwentyWinCount);
        int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
        palyer->set_isdivinemath(shensuanzi);
        palyer->set_index(i + 1);
        if (shensuanzi == 1)
            palyer->set_index(0);

        palyer->set_isbanker(0);
        palyer->set_gender(0);
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(uInfoItem->location);
        palyer->set_headboxid(0);
        palyer->set_luserwinscore(uInfoItem->RealMoney);

        if (++i >= 6)
            break;
    }
    ///
    ////////////////////////////////

    vector<OpenResultInfo> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
	for (int i = 0;i < BET_ITEM_COUNT; ++i)
	{
		vResultList = m_vOpenResultListCount_last[i];
		if (openCount == 0)
			curStateRsp.add_notopencount(0);
		else
		{
            curStateRsp.add_notopencount(vResultList[openCount - 1].notOpenCount);
		}
	}


    std::string sendData = curStateRsp.SerializeAsString();
    m_pITableFrame->SendTableData(chairid, hhmf::SUB_S_QUERY_CUR_STATE, (uint8_t *)sendData.c_str(), sendData.size());
}

// 100局游戏记录
void TableFrameSink::QueryGameRecord(int32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
	shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(chairid);
	if (!player)
		return;
    hhmf::CMD_C_GameRecord gameRecordReq;
	gameRecordReq.ParseFromArray(data, datasize);

	int32_t userid = gameRecordReq.dwuserid();

	sendGameRecord(chairid);
	
}

// 写玩家分
void TableFrameSink::WriteUserScore()
{ 
    vector<tagScoreInfo> infoVec;
    infoVec.clear(); 

    //牌值|黑红梅方王-倍数(01234)|大小七王-倍数(0123)|单双七王-倍数(0123)|红色黑色王-倍数(012)

    float hhmfmul =0.0;
    int32_t daxiaomul = 0;
    int32_t danshuangmul =0;
    int32_t honghei =0;
    if(m_strResultInfo.iHhmf==4)
    {
        hhmfmul = 20;
    }
    else if(m_strResultInfo.iHhmf==0||m_strResultInfo.iHhmf==1)
    {
        hhmfmul = 4;
    }
    else
    {
        hhmfmul = 3.8;
    }
    if(m_strResultInfo.iDaxiao>2)
    {
        daxiaomul = 0;
    }
    else if(m_strResultInfo.iDaxiao==2)
    {
        daxiaomul = 13;
    }
    else
    {
        daxiaomul = 2;
    }
    if(m_strResultInfo.iDanshuang>1)
    {
        danshuangmul = 0;
    }
    else
    {
        danshuangmul = 2;
    }
    if(m_strResultInfo.iHonghei<2)
    {
        honghei = 2;
    }
    else
    {
        honghei = 0;
    }
    char buf[32]={0};
    snprintf(buf,sizeof(buf),"%02X", (unsigned char)m_strResultInfo.icard);
    string card = buf;
    string strRet="";
    if(m_strResultInfo.iHhmf==4)
    {
        strRet = str(boost::format("%s|%d-%.2f|||")
                   % card
        %m_strResultInfo.iHhmf%hhmfmul);
    }
    else if(m_strResultInfo.iDaxiao==2)
    {
        strRet = str(boost::format("%s|%d-%.2f|%d-%d||%d-%d")
                   % card
        %m_strResultInfo.iHhmf%hhmfmul%m_strResultInfo.iDaxiao%daxiaomul
        %m_strResultInfo.iHonghei%honghei);
    }
    else
    {
        strRet = str(boost::format("%s|%d-%.2f|%d-%d|%d-%d|%d-%d")
                   % card
        %m_strResultInfo.iHhmf%hhmfmul%m_strResultInfo.iDaxiao%daxiaomul
        %m_strResultInfo.iDanshuang%danshuangmul%m_strResultInfo.iHonghei%honghei);
    }
    LOG(WARNING) << "写玩家分 =>" << strRet; 
    m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,strRet,-1,opShowCard,-1,-1);

    bool isNeedRecord = false;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

	int64_t allBetScore = 0;
	int64_t userid = 0;
	m_replay.detailsData = "";
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if (pUserItem->IsAndroidUser()) 
            continue; 
        if ( userInfoItem->dCurrentPlayerJetton == 0) 
            continue;

		allBetScore = 0;
		userid = pUserItem->GetUserId();
        tagScoreInfo scoreInfo; 
        for(int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++){  
            // 结算时记录
            int64_t addScore = userInfoItem->dAreaWin[betIdx];
            int64_t betScore = userInfoItem->dAreaJetton[betIdx]; 
            // 押分情况
            scoreInfo.cellScore.push_back(betScore);
            //牌型
            if(betScore > 0){
                m_replay.addResult(userInfoItem->wCharid, betIdx, betScore, addScore, strRet, false);
            }
			allBetScore += betScore;
        }
		if (allBetScore > 0)
		{
			//对局记录详情
			SetGameRecordDetail(userid, userInfoItem->wCharid, pUserItem->GetUserScore(), userInfoItem);
		}
        int64_t betDaXiao = userInfoItem->dAreaJetton[5]-userInfoItem->dAreaJetton[6];
        int64_t betDanShuang = userInfoItem->dAreaJetton[7]-userInfoItem->dAreaJetton[8];
        int64_t betHongHei = userInfoItem->dAreaJetton[9]-userInfoItem->dAreaJetton[10];
        int64_t betSix = 0;
         betSix=userInfoItem->dAreaJetton[5]+userInfoItem->dAreaJetton[6]
                 +userInfoItem->dAreaJetton[7]+userInfoItem->dAreaJetton[8]
                 +userInfoItem->dAreaJetton[9]+userInfoItem->dAreaJetton[10];

        int64_t reallyBet = userInfoItem->dCurrentPlayerJetton;
        if(betDaXiao<0)
        {
            betDaXiao = betDaXiao*-1;
        }
        if(betDanShuang<0)
        {
            betDanShuang = betDanShuang*-1;
        }
        if(betHongHei<0)
        {
            betHongHei = betHongHei*-1;
        }
        //王
        if(userInfoItem->lResultWangSeven==2)
        {
            reallyBet=reallyBet-userInfoItem->dAreaJetton[0]-userInfoItem->dAreaJetton[1]-userInfoItem->dAreaJetton[2]-userInfoItem->dAreaJetton[3];
            //reallyBet=reallyBet-betSix+betDanShuang+betDaXiao+betHongHei;
        }//7
        else if(userInfoItem->lResultWangSeven==1)
        {
            reallyBet=reallyBet-(userInfoItem->dAreaJetton[5]+userInfoItem->dAreaJetton[6]
                    +userInfoItem->dAreaJetton[7]+userInfoItem->dAreaJetton[8]
                    +userInfoItem->dAreaJetton[9]+userInfoItem->dAreaJetton[10])+betHongHei;
        }//普通
        else
        {
            reallyBet=reallyBet-betSix+betDanShuang+betDaXiao+betHongHei;
        }
        LOG(WARNING) << "结算时记录 =>";

        scoreInfo.chairId = userInfoItem->wCharid;
        scoreInfo.addScore = userInfoItem->m_winAfterTax; 
        scoreInfo.revenue = userInfoItem->m_userTax; 
        scoreInfo.betScore = userInfoItem->dCurrentPlayerJetton;
        scoreInfo.rWinScore = reallyBet; //
        //牌型
        scoreInfo.cardValue = strRet;// str(boost::format("%d-%d;") % m_iResultId % m_iRetMul);
        scoreInfo.startTime = m_startTime;
        infoVec.push_back(scoreInfo);

        isNeedRecord = true;
        // 结算时记录 
        // m_replay.addResult(userInfoItem->wCharid, userInfoItem->wCharid, scoreInfo.betScore, scoreInfo.addScore, scoreInfo.cardValue, false);
    }
    //保存对局结果
    if(isNeedRecord)
        m_pITableFrame->SaveReplay(m_replay);

    //先关掉写分
    m_pITableFrame->WriteUserScore(&infoVec[0], infoVec.size(), m_strRoundId);
}

void TableFrameSink::ReadInI()
{

    if(!boost::filesystem::exists("conf/hhmf_config.ini")){

        return;
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("conf/hhmf_config.ini",pt);

    int64_t nAreaMaxJetton[BET_ITEM_COUNT] = { 0 };
    memcpy(nAreaMaxJetton, m_nAreaMaxJetton, sizeof(nAreaMaxJetton));    
    m_dControlkill = pt.get<double>("GAME_CONFIG.Controlkill", 0.3);
    m_lLimitPoints = pt.get<double>("GAME_CONFIG.LimitPoint", 500000);
    //m_lBlackBetLimit 黑名单控制下注多少开始控，m_lControlAndLoseLimit控制的时候允许系统一次输多少
    m_lBlackBetLimit = pt.get<int64_t>("GAME_CONFIG.BlackBetLimit", 10000);
    m_lControlAndLoseLimit = pt.get<int64_t>("GAME_CONFIG.ControlAndLoseLimit", 500000);


    string areastr = "";
    for(int i = 0;i<BET_ITEM_COUNT;i++){      
        areastr="GAME_AREA_MAX_JETTON_"+to_string(m_pITableFrame->GetGameRoomInfo()->roomId)+".MaxJetton"+to_string(i);
        m_nAreaMaxJetton[i] = COIN_RATE * pt.get<int64_t>(areastr, 10000);
        assert(m_nAreaMaxJetton[i]>0);
        if(m_nAreaMaxJetton[i]==0)
            m_nAreaMaxJetton[i] = nAreaMaxJetton[i];
        // 开奖权重 

        //LOG(WARNING) << "--- ReadInI--- "  <<" "<< m_nAreaMaxJetton[i] <<" "<< weight[i]<<" ";
    }
    m_fUpdateGameMsgTime = pt.get<double>("GAME_CONFIG.TM_UpdateGameMsgTime", 1.0);
    stockWeak = pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    // 倒计时时间
    m_betCountDown = pt.get<int64_t>("GAME_STATE_TIME.TM_GAME_START", 20);
    m_endCountDown = pt.get<int64_t>("GAME_STATE_TIME.TM_GAME_END", 10);

    m_fGameStartAniTime = pt.get<double>("GAME_STATE_TIME.TM_GAME_START_ANI", 1.0);
    m_fGameEndAniTime = pt.get<double>("GAME_STATE_TIME.TM_GAME_END_ANI", 1.0);

    m_fResetPlayerTime = pt.get<double>("GAME_STATE_TIME.TM_RESET_PLAYER_LIST", 5.0);

    // 控制概率
    m_ctrlRate         =(pt.get<int64_t>("GAME_STATE_TIME.CTRL_RATE", 50) * 1.0)/100.0;
    m_stockPercentRate =(pt.get<int64_t>("GAME_STATE_TIME.STOCK_CTRL_RATE", 50) * 1.0)/100.0;

    m_bIsIncludeAndroid = pt.get<int64_t>("GAME_CONFIG.USER_ANDROID_DATA_TEST_AISC", 0);
    m_MarqueeScore = pt.get<int64_t>("GAME_CONFIG.PAOMADENG_CONDITION_SET", 1000);
    m_fTaxRate = pt.get<int64_t>("GAME_CONFIG.REVENUE", 0.025);
    m_fTMJettonBroadCard = pt.get<int64_t>("GAME_CONFIG.TM_JETTON_BROADCAST", 0.5);
	// 
    testIsOpen = pt.get<double>("TEST.TestRet", 0);
    testList[0] = pt.get<double>("TEST.TestList1", 1);
    testList[1] = pt.get<double>("TEST.TestList2", 2);
    testList[2] = pt.get<double>("TEST.TestList3", 2);



    string strCHIP_CONFIGURATION = "";
    strCHIP_CONFIGURATION = "CHIP_CONFIGURATION_" + to_string(m_pITableFrame->GetGameRoomInfo()->roomId);

    m_lLimitScore[0] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMin", 100);
    m_lLimitScore[1] = pt.get<int64_t>(strCHIP_CONFIGURATION + ".LimitScoreMax", 2000000);
	// m_bTest = pini->GetLong("GAME_CONFIG", "TestGame", 0);
 //    m_nTestTimes = pini->GetLong("GAME_CONFIG", "TestGameTimes", 0);
 //    m_nTestType = pini->GetLong("GAME_CONFIG", "nTestType", 3);
    // 是否写日记
    m_bWritelog = false;
    if (pt.get<int64_t>("GAME_CONFIG.nWritelog", 0) != 0)
    {
        m_bWritelog = true;
    }
    vector<int64_t> scorelevel;
    vector<int64_t> chips;
    vector<int64_t> allchips;
    allchips.clear();
    scorelevel.clear();
    chips.clear();
    m_useIntelligentChips=pt.get<int64_t>(strCHIP_CONFIGURATION + ".useintelligentchips",1);
    scorelevel = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear1","1,2,5,10,50"));
    for(int i=0;i<4;i++)
    {
        m_userScoreLevel[i]=scorelevel[i];
    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
        m_userChips[0][i]=chips[i];
        allchips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>(strCHIP_CONFIGURATION + ".chipgear2","1,2,5,10,50"));
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
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
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
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
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
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
    if(!m_useIntelligentChips)
    {
        for(int i=0;i<MAX_CHOUMA_NUM;i++)
        {
            m_userChips[0][i]=chips[i];
            m_userChips[1][i]=chips[i];
            m_userChips[2][i]=chips[i];
            m_userChips[3][i]=chips[i];
        }
    }

    m_cbJettonMultiple.clear();
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
        m_cbJettonMultiple.push_back(m_userChips[0][i]);
    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[1][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[1][i]);
        }
    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[2][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[2][i]);
        }
    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
        vector<int64_t>::iterator it=find(m_cbJettonMultiple.begin(),m_cbJettonMultiple.end(),m_userChips[3][i]);
        if(it!=m_cbJettonMultiple.end())
        {

        }
        else
        {
            m_cbJettonMultiple.push_back(m_userChips[3][i]);
        }
    }
	sort(m_cbJettonMultiple.begin(), m_cbJettonMultiple.end());   
}

// 发送开始游戏命令
void TableFrameSink::NormalGameStart()
{
    LOG(WARNING) << "--- 开始游戏 2---" << __FUNCTION__;
    openSLog("本局游戏开始;");
	getJSodds();
    // if (m_bTest>0)
    // {
    //     return;
    // }	
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<IServerUserItem> pServerUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;

        hhmf::CMD_S_GameStart gameStart;
        gameStart.set_cbplacetime(m_betCountDown);
        gameStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameStart.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            gameStart.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);           
        }
        if (!pUserItem->IsAndroidUser())
        {
            LOG(WARNING) << "--------------------GetUserScore()==" <<pUserItem->GetUserId() <<" "<< pUserItem->GetUserScore();
        }
        // 
        gameStart.set_cbroundid(m_strRoundId);
        gameStart.set_luserscore(pUserItem->GetUserScore());
        gameStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
        gameStart.add_llimitscore(m_lLimitScore[0]);
        gameStart.add_llimitscore(m_lLimitScore[1]);

        gameStart.set_tableid(m_strTableId);
        gameStart.set_nroundcount(m_iRoundCount);

		i = 0;
		for (auto &uInfoItem : m_pPlayerInfoList)
		{
			xzScore = 0;
			pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
			if (!pServerUserItem)
				nowScore = 0;
			else
			{
                for (int j = 0; j < BET_ITEM_COUNT; j++)
				{
					xzScore += uInfoItem->dAreaJetton[j];
				}
				nowScore = pServerUserItem->GetUserScore() - xzScore;
			}
			/*if (userInfoItem->chairId == m_wBankerUser)
			{
				continue;
			}*/
			if (uInfoItem->headerId > 12)
			{
				LOG_INFO << " ========== 5 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
			}
            hhmf::PlayerListItem* palyer = gameStart.add_players();
            palyer->set_dwuserid(uInfoItem->wUserid);
			palyer->set_headerid(uInfoItem->headerId);
			palyer->set_nviplevel(0);
			palyer->set_nickname(uInfoItem->nickName);


            LOG(ERROR)<<" 传递的玩家列表分数 "<<nowScore;
			palyer->set_luserscore(nowScore);

			palyer->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
			palyer->set_ltwentywincount(uInfoItem->lTwentyWinCount);
            int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(i + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			//if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
			//{
			   // palyer->set_isbanker(1);
			//}
			palyer->set_gender(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
			palyer->set_szlocation(uInfoItem->location);
			palyer->set_headboxid(0);
			palyer->set_luserwinscore(uInfoItem->RealMoney);

			if (++i >= size)
				break;
		}
		
        vector<OpenResultInfo> vResultList;
		vResultList = m_vOpenResultListCount_last[0];
		int32_t openCount = vResultList.size();
		for (int i = 0;i < BET_ITEM_COUNT; ++i)
		{
			vResultList = m_vOpenResultListCount_last[i];
			if (openCount == 0)
				gameStart.add_notopencount(0);
			else
			{
                gameStart.add_notopencount(vResultList[openCount - 1].notOpenCount);
			}				
		}

        std::string endData = gameStart.SerializeAsString();
        m_pITableFrame->SendTableData(pUserItem->GetChairId(), hhmf::SUB_S_GameStart, (uint8_t *)endData.c_str(), endData.size());
    }
}

// 跑马灯显示
void TableFrameSink::BulletinBoard()
{
    int32_t winscore = 0;
    int idex = 0;
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if (userInfoItem->RealMoney >= m_pGameRoomInfo->broadcastScore)
        {
            int sgType = SMT_GLOBAL | SMT_SCROLL;
            m_pITableFrame->SendGameMessage(userInfoItem->wCharid, "", sgType, userInfoItem->RealMoney);
            LOG(INFO) << "广播玩家 "<< userInfoItem->wCharid << " 获奖分数 " <<  userInfoItem->RealMoney;
        }
    }
}

// 重置桌子数据(记录重押数据)
void TableFrameSink::ResetTableDate()
{
    LOG(WARNING) << "重置桌子数据" << __FUNCTION__;

    m_iRetMul = 0;
    // m_iResultId = 0;
    // m_iStopIndex = 0;
    m_BestPlayerUserId = 0;
    m_BestPlayerChairId = 0;
    m_BestPlayerWinScore = 0.0;

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if(userInfoItem->dCurrentPlayerJetton == 0)
        {
            userInfoItem->clear();
            continue;
        }
        // 否则把重押分数记录
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            userInfoItem->dLastJetton[j] = userInfoItem->dAreaJetton[j];
        }

        userInfoItem->clear();
    }

    m_pBetArea->ThisTurnClear();
}
void TableFrameSink::Algorithm()
{
    int  isHaveBlackPlayer=0;

	
    for(int i=0;i<MAX_PLAYER;i++)
    {
        shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
        if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
        {
            continue;
        }
        int64_t allJetton=0;
        for(int i=0;i<BET_ITEM_COUNT;i++)
        {
            allJetton+=m_UserInfo[user->GetUserId()]->dAreaJetton[i];
        }
        if(allJetton<=m_lBlackBetLimit) continue;
        if(user->GetBlacklistInfo().listRoom.size()>0)
        {
            auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
            if(it != user->GetBlacklistInfo().listRoom.end())
            {
                if(it->second>0)
                {
                    isHaveBlackPlayer=it->second;//
                    openSLog("黑名单:%d房间控制系数读到 :%d",user->GetUserId(),isHaveBlackPlayer);
                }
                //openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                //openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
            }
        }

    }
    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;

    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,命中概率系统必须赚钱
    if(randomRet<m_difficulty)
    {

        hhmfAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
        hhmfAlgorithm.SetMustKill(-1);
        for(int i=0;i<MAX_JETTON_AREA;i++)
         hhmfAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
        hhmfAlgorithm.GetOpenCard(m_strResultInfo,m_CardVecData);
        openSLog("全局难度,正常算法最终出结果: %d",m_iResultId);
    }
    else
    {
        if(isHaveBlackPlayer)
        {
            int32_t killProbili=0;
            float gailv = 0.0;
            int64_t blackerBet[BET_ITEM_COUNT]={0};
            for(int i=0;i<MAX_PLAYER;i++)
            {
                shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                if(!user||user->IsAndroidUser())
                {
                    continue;
                }
                int64_t allJetton=0;
                for(int j=0;j<BET_ITEM_COUNT;j++)
                {
                    allJetton+=m_UserInfo[user->GetUserId()]->dAreaJetton[j];
                }
                if(allJetton==0) continue;

                if(user->GetBlacklistInfo().status == 1 )
                {

                    int shuzhi=0;
                    auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                    if(it != user->GetBlacklistInfo().listRoom.end())
                    {
                        shuzhi=it->second;
                    }
                    else
                    {
                        shuzhi=0;
                    }
                    if(shuzhi >= killProbili)
                    {
                        killProbili=shuzhi;
                        for(int j=0;j<BET_ITEM_COUNT;j++)
                        {
                            blackerBet[j] =m_UserInfo[user->GetUserId()]->dAreaJetton[j];
                        }
                    }
                    gailv=(float)killProbili/1000;//获得最高控制概率
//                    int64_t allWinscore=0;
//                    for(int j=0;j<MAX_JETTON_AREA;j++)
//                    {
//                        allWinscore+=m_pBetArea->AndroidmifourAreajetton[j];
//                    }

//                   int64_t AllWinDaXiao = m_pBetArea->AndroidmifourAreajetton[eDa]+m_pBetArea->AndroidmifourAreajetton[eXiao];
//                   int64_t AllWinDanShuang = m_pBetArea->AndroidmifourAreajetton[eDan]+m_pBetArea->AndroidmifourAreajetton[eShuang];
//                   int64_t AllWinDianShu = 0;
//                   for(int i=eDian4;i<eSanjun1;i++)
//                   {
//                       AllWinDianShu+=m_pBetArea->AndroidmifourAreajetton[i];
//                   }

//                   for(int j=0;j<MAX_JETTON_AREA;j++)
//                   {
//                       if(allWinscore>0)
//                       {
//                           probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->dAreaJetton[j])/(float)m_pBetArea->AndroidmifourAreajetton[j];
//                           openSLog("黑名单玩家 :%d 确定调用黑名单算法 每门传的黑名单控制概率是[%d]：%f",user->GetUserId(),j,(gailv*(float)m_UserInfo[user->GetUserId()]->dAreaJetton[j])/(float)allWinscore);
//                       }

//                   }

                }
            }


            hhmfAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<MAX_JETTON_AREA;i++)
             hhmfAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            hhmfAlgorithm.BlackGetOpenCard(m_strResultInfo,gailv,blackerBet,m_lControlAndLoseLimit,m_CardVecData);

        }
        else
        {
            hhmfAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
            for(int i=0;i<MAX_JETTON_AREA;i++)
             hhmfAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
            hhmfAlgorithm.GetOpenCard(m_strResultInfo,m_CardVecData);

        }

    }

    string openStr = "开奖动物累计:";
    for (int i = 0; i < BET_ITEM_COUNT; ++i)
    {
        openStr += to_string(i) + "_" + to_string(m_lOpenResultIndexCount[i])+ "  ";
    }
    int32_t cardColor=logic.GetCardColor(m_strResultInfo.icard);
    m_iHhmfwNum[cardColor]++;
    if(/*m_CardVecData.size() < MIN_LEFT_CARDS_RELOAD_NUM*/m_iRoundCount>=OPEN_RESULT_LIST_COUNT)
    {
        m_CardVecData.clear();
        m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

        m_iRoundCount = 0;
        //clear open record
        //m_openRecord.clear();
        //bRestart = true;
        memset(m_iHhmfwNum, 0, sizeof(m_iHhmfwNum));

    }
}

void TableFrameSink::NormalCalculate()
{

    Algorithm();

    if(testIsOpen)
    {
        hhmfAlgorithm.SetAlgorithemNum(m_llStorageHighLimit ,m_llStorageLowerLimit, m_llStorageControl,m_dControlkill, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值
        for(int i=0;i<MAX_JETTON_AREA;i++)
         hhmfAlgorithm.SetBetting(i,m_pBetArea->AndroidmifourAreajetton[i]);//设置每门真人押注值
        hhmfAlgorithm.TestOpenCard(m_strResultInfo,testList);
    }


    LOG(WARNING) <<"**********牌"<<m_strResultInfo.icard;
    for(int i=0;i<eMax;i++)
    {
        LOG(WARNING) <<"**********第"<<i<<"门倍数  "<<m_strResultInfo.muticlList[i];
        LOG(WARNING) <<"**********第"<<i<<"门中奖  "<<m_strResultInfo.winList[i];
    }

    // 选择出本轮最大赢分玩家
    int32_t winscore = 0;
    int bestChairid = 0;
    int bestUserid = 0;
    bool bFirstOnlineUser = true;
	//m_pPlayerInfoList.clear();
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem || !userInfoItem->Isplaying)
            continue;
        if (bFirstOnlineUser) // 预防在线的玩家都输钱的情况
        {
            bFirstOnlineUser = false;
            winscore = userInfoItem->ReturnMoney;
            bestChairid = userInfoItem->wCharid;
            bestUserid = userInfoItem->wUserid;
        }
        //税收比例
        userInfoItem->Culculate(m_fTaxRate,m_strResultInfo); //m_pITableFrame,
        if (winscore <= userInfoItem->ReturnMoney)
        {
            winscore = userInfoItem->ReturnMoney;
            bestChairid = userInfoItem->wCharid;
            bestUserid = userInfoItem->wUserid;
        }
		//m_pPlayerInfoList.push_back(userInfoItem);
    }
    // 把最大赢分玩家添加到列表(20秒)
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(bestChairid);
    if (player && m_UserInfo[bestUserid]->ReturnMoney > 0)
    {
        BestPlayerList pla;
        pla.NikenName = player->GetNickName();
        pla.bestID = player->GetChairId();
        pla.WinScore = m_UserInfo[bestUserid]->ReturnMoney;
        time_t t = time(nullptr);

        char ch[64] = {0};

        strftime(ch, sizeof(ch) - 1, "%Y-%m-%d %H:%M:%S", localtime(&t)); //年-月-日 时-分-秒
        pla.onTime = ch;
        //cout<<p->tm_year+1900<<"  "<<p->tm_mon+1<<"  "<<p->tm_mday<<"  "<<p->tm_hour<<

        if (m_vPlayerList.size() >= BEST_PLAYER_LIST_COUNT){
            m_vPlayerList.erase(m_vPlayerList.begin());
        }

        m_vPlayerList.push_back(pla);
    }
	//PlayerRichSorting();

	//写玩家分(注意顺序)
	WriteUserScore();
	m_bGameEnd = true;
	// 填充路单(长度9)
    if (m_vResultRecored.size() >= RES_RECORD_COUNT)
	{
		m_lastRetId = m_vResultRecored[0];
		m_vResultRecored.erase(m_vResultRecored.begin());
        m_vResultRecored.push_back(m_strResultInfo);
	}
	else
	{
        m_vResultRecored.push_back(m_strResultInfo);
	}

    // udpate state
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
	shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem)
            continue;
        if (userInfoItem->dCurrentPlayerJetton<=0)
        {
            userInfoItem->NotBetJettonCount++;
        }
        else //if (m_pPlayerList[i].NotBetJettonCount < 5)
        {            
            userInfoItem->NotBetJettonCount = 0;
        }
        hhmf::CMD_S_GameEnd gameEnd;
        gameEnd.set_cbplacetime((int32_t)(m_endCountDown ));//12); //总时间
        gameEnd.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
        for (int j = 0; j < BET_ITEM_COUNT; j++)
        {
            gameEnd.add_selfjettonscore(userInfoItem->dAreaJetton[j]);
            gameEnd.add_lalljettonscore(m_pBetArea->mifourAreajetton[j]);
        }
        gameEnd.set_lusermaxscore(0);
        gameEnd.set_cbroundid(m_strRoundId);

        LOG(WARNING) << "下发玩家分数 "<< userInfoItem->wCharid << " RealMoney=" << userInfoItem->RealMoney << " " << pUserItem->GetUserScore();

        gameEnd.set_luserscore(pUserItem->GetUserScore()); // //m_nPlayerScore[wCharid]
        gameEnd.set_mostwinscore(m_UserInfo[bestUserid]->ReturnMoney);

        shared_ptr<CServerUserItem> bestplayer = m_pITableFrame->GetTableUserItem(bestChairid);
        gameEnd.set_headid(bestplayer->GetHeaderId());
        gameEnd.set_headboxid(0); //bestplayer->GetHeadBoxID());
        gameEnd.set_gender(0);    //bestplayer->GetGender());
        m_BestPlayerUserId = bestplayer->GetUserId();
        m_BestPlayerChairId = bestChairid;
        m_BestPlayerWinScore = m_UserInfo[bestUserid]->ReturnMoney;
        gameEnd.set_bestuserid(bestplayer->GetUserId());
        gameEnd.set_bestusernikename(bestplayer->GetNickName());
        gameEnd.set_userwinscore(userInfoItem->RealMoney);
        gameEnd.set_onlinenum(m_pITableFrame->GetPlayerCount(true));

        for(int i=0;i<5;i++)
        {
            gameEnd.add_hhmfwnum(m_iHhmfwNum[i]);
        }
        hhmf::ResultStr* res = gameEnd.mutable_resultinfo();
        res->set_ndanshuang(m_strResultInfo.iDanshuang);
        res->set_ndaxiao(m_strResultInfo.iDaxiao);
        res->set_ncard(m_strResultInfo.icard);
        res->set_nhhmf(m_strResultInfo.iHhmf);
        res->set_nhonghei(m_strResultInfo.iHonghei);
        //-1 是不中奖，0是返还，大于零则是中将倍数
        for(int i=0;i<eMax;i++)
        {   if(m_strResultInfo.bReturnMoney[i])
            {
                res->add_nwinmutic(0);
            }
            else if(m_strResultInfo.muticlList[i])
            {
                res->add_nwinmutic(m_strResultInfo.muticlList[i]);
            }
            else
            {
                res->add_nwinmutic(-1);
            }
        }


		i = 0;
        for (auto &uInfoItem : m_pPlayerInfoList)
		{
			xzScore = 0;
			pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
			if (!pServerUserItem)
				nowScore = 0;
			else
			{
				nowScore = pServerUserItem->GetUserScore();
			}
			if (uInfoItem->headerId > 12)
			{
				LOG_INFO << " ========== 1 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
			}
            hhmf::PlayerListItem *palyer = gameEnd.add_players();
            palyer->set_dwuserid(uInfoItem->wUserid);
            palyer->set_headerid(uInfoItem->headerId);
			palyer->set_nviplevel(0);
            palyer->set_nickname(uInfoItem->nickName);

			palyer->set_luserscore(nowScore);

            palyer->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
            palyer->set_ltwentywincount(uInfoItem->lTwentyWinCount);
            int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
			palyer->set_isdivinemath(shensuanzi);
			palyer->set_index(i + 1);
			if (shensuanzi == 1)
				palyer->set_index(0);

			palyer->set_isbanker(0);
			palyer->set_gender(0);
			palyer->set_isapplybanker(0);
			palyer->set_applybankermulti(0);
			palyer->set_ljetton(0);
            palyer->set_szlocation(uInfoItem->location);
			palyer->set_headboxid(0);
			palyer->set_luserwinscore(uInfoItem->RealMoney);
			if (++i >= size)
				break;
		}
        std::string endData = gameEnd.SerializeAsString();
        m_pITableFrame->SendTableData(pUserItem->GetChairId(), hhmf::SUB_S_GameEnd, (uint8_t *)endData.c_str(), endData.size());
        openSLog("本局结算: wCharid=%d,userid=%d,UserScore=%d,输赢=%d;",pUserItem->GetChairId(),pUserItem->GetUserId(),pUserItem->GetUserScore(),userInfoItem->RealMoney);
    }

	m_pPlayerInfoList.clear();
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		if (!pUserItem || !userInfoItem->Isplaying)
			continue;
		m_pPlayerInfoList.push_back(userInfoItem);
	}
	PlayerRichSorting();
    //100局记录
    updateResultList( m_strResultInfo );
}

// 玩家财富排序
void TableFrameSink::PlayerRichSorting(bool iChooseShenSuanZi)
{
    if (m_pPlayerInfoList.size() > 1)
    {
		if (iChooseShenSuanZi)
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions);
			sort(++m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
			m_iShenSuanZiUserId = m_pPlayerInfoList[0]->wUserid;
			m_iShenSuanZiChairId = m_pPlayerInfoList[0]->wCharid;
			m_ShenSuanZiId = m_pPlayerInfoList[0]->wUserid;
		} 
		else
		{
			sort(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end(), Comparingconditions1);
		}
    }   

	SortPlayerList();
    
}

void TableFrameSink::SendPlayerList(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{

    if (m_iGameState == hhmf::SUB_S_GameEnd)
    {
        if (m_vPlayerList.size() > 1)
        {
            hhmf::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 2; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                hhmf::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
				playerinfo->set_luserscore(player->GetUserScore());
            }
            string sendData = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, hhmf::SUB_S_PLAYERLIST, (uint8_t *)sendData.c_str(), sendData.size());
        }
    }
    else
    {
        if (m_vPlayerList.size() > 0)
        {
            hhmf::CMD_S_UserWinList PlayerList;
            for (int i = m_vPlayerList.size() - 1; i > -1; i--)
            {
                shared_ptr<CServerUserItem> player = m_pITableFrame->GetTableUserItem(m_vPlayerList[i].bestID);
                if (!player)
                {
                    continue;
                }
                hhmf::PlayerInfo *playerinfo = PlayerList.add_player();
                playerinfo->set_winscore(m_vPlayerList[i].WinScore);
                playerinfo->set_wintime(m_vPlayerList[i].onTime);
                playerinfo->set_dwuserid(player->GetUserId());
                playerinfo->set_gender(0);    //player->GetGender());
                playerinfo->set_headboxid(0); //player->GetHeadBoxID());
                playerinfo->set_headerid(player->GetHeaderId());
                playerinfo->set_nickname(player->GetNickName());
                playerinfo->set_nviplevel(0); //player->GetVipLevel());
                playerinfo->set_szlocation(player->GetLocation()); 
				playerinfo->set_luserscore(player->GetUserScore());
            }
            string sendData = PlayerList.SerializeAsString();
            m_pITableFrame->SendTableData(chairid, hhmf::SUB_S_PLAYERLIST, (uint8_t *)sendData.c_str(), sendData.size());
        }
    }

    LOG(WARNING) << "----发送排行榜数据------";
}
bool TableFrameSink::GamePlayerJetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> pSelf = m_pITableFrame->GetTableUserItem(chairid);
    if (!pSelf)
        return false;
    hhmf::CMD_C_PlaceJet placeJet;
    placeJet.ParseFromArray(data, datasize);
    int cbJetArea = placeJet.cbjettonarea();
    int64_t lJetScore = placeJet.ljettonscore();
    
    if (lJetScore <= 0 || cbJetArea < 0 || cbJetArea >= BET_ITEM_COUNT)
    {
        LOG(ERROR) << "---------------押分有效性检查出错---------->" << cbJetArea <<" "<<lJetScore;
        return false;
    }
    bool bPlaceJetScuccess = false;
    bool nomoney = false;
    int errorAsy = 0;

    do
    {
        if (1!= m_bBetStatus)
        {
            errorAsy = -1;
            break;
        }

        if (pSelf)
        {
            // 玩家押分判断
            errorAsy = PlayerJettonPanDuan(cbJetArea, lJetScore, chairid, pSelf);
            if (errorAsy != 0)
            {
                break;
            }
        }
        bPlaceJetScuccess = true;
    } while (0);

    // 玩家下注无效
    if (!bPlaceJetScuccess || 1!= m_bBetStatus)
    {
        // 非机器人才返回
        if(!pSelf->IsAndroidUser()){
            hhmf::CMD_S_PlaceJettonFail placeJetFail;
            int64_t userid = pSelf->GetUserId();
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(cbJetArea);
            placeJetFail.set_lplacescore(lJetScore);
            placeJetFail.set_cbandroid(pSelf->IsAndroidUser());
            placeJetFail.set_returncode(errorAsy);

        	std::string sendData = placeJetFail.SerializeAsString();
            // if (m_bTest==0)
               m_pITableFrame->SendTableData(pSelf->GetChairId(), hhmf::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        }
        LOG(WARNING) << "-------------------玩家下注无效---------------" << errorAsy;
        return false;
    }
	
	uint32_t userId = pSelf->GetUserId();
	if (m_ShenSuanZiId == userId && m_ShenSuanZiId != 0)
	{
		m_lShenSuanZiJettonScore[cbJetArea] += lJetScore;
	}

	//LOG(WARNING) << "------- Place Jet Scuccess-------- UserBet: userid:" << pSelf->GetUserId() << " cbJetArea:" << cbJetArea << " lJetScore:" << lJetScore;
    int pmayernum = 0; 
    //shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
	shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
		pServerUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pServerUserItem)
            continue;
        hhmf::CMD_S_PlaceJetSuccess placeJetSucc;
        placeJetSucc.Clear();

        int64_t userid = pSelf->GetUserId();
        placeJetSucc.set_dwuserid(userid);
        placeJetSucc.set_cbjettonarea(cbJetArea);
        placeJetSucc.set_ljettonscore(lJetScore);
        placeJetSucc.set_bisandroid(pSelf->IsAndroidUser());

        for (int x = 1; x <= MAX_JETTON_AREA; x++)
        {
            placeJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[x - 1]);
        }

        int32_t alljetton = 0;
        alljetton = userInfoItem->dCurrentPlayerJetton;
		//int UserScore = pServerUserItem->GetUserScore() - alljetton;
        placeJetSucc.set_luserscore(pServerUserItem->GetUserScore() - alljetton);
        for (int k = 1; k <= MAX_JETTON_AREA; k++)
        {
            placeJetSucc.add_selfjettonscore(userInfoItem->dAreaJetton[k - 1]);
        } 
		//LOG(WARNING) << "------- Place Jet Scuccess-------- sendUser userid:" << pServerUserItem->GetUserId() << " GetUserScore():" << pServerUserItem->GetUserScore()  << " alljetton:" << alljetton <<  " leftUserScore:" << UserScore;
        std::string sendData = placeJetSucc.SerializeAsString();
        // if (m_bTest==0)
            m_pITableFrame->SendTableData(userInfoItem->wCharid, hhmf::SUB_S_JETTON_SUCCESS, (uint8_t *)sendData.c_str(), sendData.size());
        pmayernum++;
    }
    // LOG(WARNING) << "-------------------- Place Jet Scuccess------------ Num=" << pmayernum;

    return true;
}
void TableFrameSink::GamePlayerJetton1(int64_t score, int index, int chairid, shared_ptr<CServerUserItem> &pSelf)
{
    int64_t lJetScore = score;
    int cbJetArea = index;

    bool bPlaceJetScuccess = false;
    int errorAsy = 0;
    do
    {
        if (1!= m_bBetStatus)
        {
            errorAsy = -1;
            break;
        }
        if (pSelf)
        { 
            errorAsy = PlayerJettonPanDuan(cbJetArea, lJetScore, chairid, pSelf);
            if (errorAsy != 0)
            {
                break;
            }
        }
        bPlaceJetScuccess = true;
    } while (0);

    // 
    if (!bPlaceJetScuccess)
    {
        hhmf::CMD_S_PlaceJettonFail placeJetFail;
        int64_t userid = pSelf->GetUserId();
        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(errorAsy); //-1=shijian guo   1==wanjia shibai 2=qu yu man 3=wanjia fen buzu
        placeJetFail.set_returncode(errorAsy);

        placeJetFail.set_lplacescore(lJetScore);
        placeJetFail.set_cbandroid(pSelf->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(pSelf->GetChairId(), hhmf::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());

        LOG(WARNING) << "-------------------续押玩家下注无效---------------" << errorAsy;
        return;
    }
    
    hhmf::CMD_S_PlaceJetSuccess placeJetSucc;
    int64_t userid = pSelf->GetUserId();
    placeJetSucc.set_dwuserid(userid);
    placeJetSucc.set_cbjettonarea(cbJetArea);
    placeJetSucc.set_ljettonscore(lJetScore);
    placeJetSucc.set_bisandroid(pSelf->IsAndroidUser());
    int32_t alljetton = 0;
    // alljetton = m_pPlayerList[chairid].dCurrentPlayerJetton;
    alljetton = m_UserInfo[userid]->dCurrentPlayerJetton;
    placeJetSucc.set_luserscore(pSelf->GetUserScore() - alljetton);
    for (int i = 0; i < MAX_JETTON_AREA; i++)
    {
        placeJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[i]);
    }

	if (m_ShenSuanZiId == userid && m_ShenSuanZiId != 0)
	{
		m_lShenSuanZiJettonScore[cbJetArea] += lJetScore;
	}

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if(!pUserItem )
            continue;
        placeJetSucc.clear_selfjettonscore();
        for (int k = 0; k < MAX_JETTON_AREA; k++)
        {
            placeJetSucc.add_selfjettonscore(userInfoItem->dAreaJetton[k]);
        }
        std::string sendData = placeJetSucc.SerializeAsString();
        m_pITableFrame->SendTableData(userInfoItem->wCharid, hhmf::SUB_S_JETTON_SUCCESS, (uint8_t *)sendData.c_str(), sendData.size());
    }
}
// 重押
void TableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
    if (!Usr){ LOG(WARNING) << "----查找玩家不在----"; return;}

    if (!m_UserInfo[Usr->GetUserId()]->Isplaying)
    {
        LOG(WARNING) << "-------------续押失败(玩家状态不对)-------------";
        return;
    }

    LOG(WARNING) << "-----------Rejetton续押-------------";
    // 游戏状态判断
//    if (1!= m_bBetStatus)
//    {
//        LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_iGameState << " " << hhmf::SUB_S_GameStart;
//        return;
//    }
    int64_t alljetton = 0;
    int64_t areajetton[BET_ITEM_COUNT] = { 0 };
    int64_t userid = Usr->GetUserId();

    hhmf::CMD_C_ReJetton reJetton;
    reJetton.ParseFromArray(data, datasize);
    int reSize = reJetton.ljettonscore_size();
    for (int i = 0;i < reSize;i++)
    {
        m_UserInfo[userid]->dLastJetton[i] = reJetton.ljettonscore(i);
        if (m_UserInfo[userid]->dLastJetton[i] < 0)
        {
            LOG(ERROR) << "---续押押分有效性检查出错--->" << i << " " << m_UserInfo[userid]->dLastJetton[i];
        }
    }
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        alljetton += m_UserInfo[userid]->dLastJetton[i];
        areajetton[i] = m_UserInfo[userid]->dLastJetton[i];
        LOG(WARNING) << "--dLastJetton-" << areajetton[i] ;
        if (areajetton[i] < 0)
        {
            alljetton = 0;
            break;
        }
    }
    if(1!= m_bBetStatus)
    {
        hhmf::CMD_S_PlaceJettonFail placeJetFail;

        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(3);
        placeJetFail.set_lplacescore(0);
        placeJetFail.set_returncode(-1);
        placeJetFail.set_cbandroid(Usr->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(Usr->GetChairId(), hhmf::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        return;
    }
    // 续押失败
    if (alljetton == 0 || Usr->GetUserScore() < alljetton)
    {
        hhmf::CMD_S_PlaceJettonFail placeJetFail;

        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(3);
        placeJetFail.set_lplacescore(0);
        placeJetFail.set_returncode(3);
        placeJetFail.set_cbandroid(Usr->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(Usr->GetChairId(), hhmf::SUB_S_JETTON_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
        return;
    }

    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
        if (m_nAreaMaxJetton[i] < m_UserInfo[userid]->dAreaJetton[i] + m_UserInfo[userid]->dLastJetton[i])
        {
            hhmf::CMD_S_PlaceJettonFail placeJetFail;
            placeJetFail.set_dwuserid(userid);
            placeJetFail.set_cbjettonarea(i);
            placeJetFail.set_lplacescore(m_UserInfo[userid]->dLastJetton[i]);
            placeJetFail.set_returncode(2);
            placeJetFail.set_cbandroid(Usr->IsAndroidUser());

            std::string data = placeJetFail.SerializeAsString();
             //if (m_bTest==0)
                m_pITableFrame->SendTableData(Usr->GetChairId(), hhmf::SUB_S_JETTON_FAIL, (uint8_t *)data.c_str(), data.size());
            LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << m_nAreaMaxJetton[i] <<" < "<<  m_pBetArea->mifourAreajetton[i] <<"+"<< m_UserInfo[userid]->dLastJetton[i];
            return;
        }
    }

    //返回续押数组
    // slwh::CMD_S_RepeatJetSuccess RepeatJet;
    // RepeatJet.set_dwuserid(userid);
    // RepeatJet.set_bisandroid(Usr->IsAndroidUser());
    // RepeatJet.set_luserscore(Usr->GetUserScore() - alljetton);

    // bool bIsJetOk = false;
    // //RepeatJet
    // for (tagBetInfo betVal : m_pPlayerList[chairid].JettonValues)
    // {
    //     slwh::RepeatInfo *RepeatInfo = RepeatJet.add_trepeat();
    //     RepeatInfo->set_luserid(userid);
    //     RepeatInfo->set_tljettonscore(betVal.betScore);
    //     RepeatInfo->set_tjetidx(betVal.betIdx);
    //     bIsJetOk = GamePlayerJetton1(betVal.betScore, betVal.betIdx, chairid, Usr);
    //     areajetton[betVal.betIdx] -= betVal.betScore;
    // }

    // // 重押失败
    // if(!bIsJetOk) {
    //     LOG(ERROR) << "---------重押失败 重押失败 重押失败--------" << m_pPlayerList[chairid].JettonValues.size();
    //     return;
    // }

    // //每个区域的总下分
    // for (int i = 0; i < MAX_JETTON_AREA; i++){
    //     RepeatJet.add_alljettonscore(m_pBetArea->mifourAreajetton[i]);
    // }

    // //单个玩家每个区域的总下分
    // for (int i = 0; i < MAX_PLAYER; i++)
    // {
    //     shared_ptr<CServerUserItem> player = this->m_pITableFrame->GetTableUserItem(i);
    //     if (!player) continue;

    //     RepeatJet.clear_selfjettonscore();
    //     for (int k = 0; k < MAX_JETTON_AREA; k++){
    //         RepeatJet.add_selfjettonscore(m_pPlayerList[i].dAreaJetton[k]);
    //     }

    //     std::string data = RepeatJet.SerializeAsString();
    //     m_pITableFrame->SendTableData(i, slwh::SUB_S_REPEAT_JETTON_OK, (uint8_t *)data.c_str(), data.size());
    // }

    // LOG(ERROR) << "---------重押成功--------";
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        if (areajetton[i] <= 0)
        {
            continue;
        }

        while (areajetton[i] > 0)
        {
            // if (areajetton[i] >= m_pBetArea->GetBetItem(4))//500
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(4);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(3))//100)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(3);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(2))//50)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(2);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else if (areajetton[i] >= m_pBetArea->GetBetItem(1))//10)
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(1);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            // else
            // {
            //     int32_t tmpVar = m_pBetArea->GetBetItem(0);
            //     GamePlayerJetton1(tmpVar, i, chairid, Usr);
            //     areajetton[i] -= tmpVar;
            // }
            int jSize = m_cbJettonMultiple.size();
            for (int j=jSize-1;j>=0;j--)
            {
                if (areajetton[i] >= m_cbJettonMultiple[j])
                {
                    int64_t tmpVar = m_cbJettonMultiple[j];
                    GamePlayerJetton1(tmpVar, i, chairid, Usr);
                    areajetton[i] -= tmpVar;
                    break;
                }
            }
        }
    }
}
void TableFrameSink::GamePlayerQueryPlayerList(uint32_t chairId, uint8_t subid, const uint8_t *data, uint32_t datasize, bool IsEnd)
{
    shared_ptr<CServerUserItem> pIIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if (!pIIServerUserItem)
        return;

	m_pPlayerInfoList.clear();

	shared_ptr<IServerUserItem> pUserItem;
	shared_ptr<UserInfo> userInfoItem;
	for (auto &it : m_UserInfo)
	{
		userInfoItem = it.second;
		pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
		if (!pUserItem)
			continue;
		if (!userInfoItem->Isplaying)
		{
			continue;
		}
		m_pPlayerInfoList.push_back(userInfoItem);
	}

    hhmf::CMD_C_PlayerList querylist;
    querylist.ParseFromArray(data, datasize);
    int32_t limitcount = querylist.nlimitcount();
    int32_t beginindex = querylist.nbeginindex();
    int32_t resultcount = 0;
    int32_t endindex = 0;
    hhmf::CMD_S_PlayerList playerlist;
    int32_t wMaxPlayer = m_pITableFrame->GetMaxPlayerCount();
    if (!limitcount)
        limitcount = wMaxPlayer;
    limitcount = 10;
    int index = 0;
    PlayerRichSorting(false);
	SortPlayerList();
    for (vector<UserInfo>::iterator it = m_pPlayerInfoList_20.begin(); it != m_pPlayerInfoList_20.end(); it++)
    {
        shared_ptr<CServerUserItem> pUser = this->m_pITableFrame->GetTableUserItem((*it).wCharid);
        if (!pUser)
            continue;
        if (!(*it).Isplaying)
            continue;
		if (pUser->GetHeaderId() > 12)
		{
            LOG_INFO << " ========== 2 wUserid " << pUser->GetUserId() << " == headerId:" << pUser->GetHeaderId();
		}
        hhmf::PlayerListItem *item = playerlist.add_players();
        item->set_dwuserid(pUser->GetUserId());
        item->set_headerid(pUser->GetHeaderId());
        item->set_nviplevel(0); //pUser->GetVipLevel());
        item->set_nickname(pUser->GetNickName());
        item->set_headboxid(0); //pUser->GetHeadBoxID());
        if (!m_bGameEnd)
        {
            item->set_luserscore(pUser->GetUserScore() - (*it).dCurrentPlayerJetton);
        }
        else
        {
            item->set_luserscore(pUser->GetUserScore());
        }
        item->set_isdivinemath(0);
        if (pUser->GetUserId() == m_iShenSuanZiUserId)
        {
            item->set_isdivinemath(1);
        }
        item->set_index(index);
        item->set_ltwentywinscore((*it).GetTwentyWinScore());
        item->set_ltwentywincount((*it).GetTwentyWin());

        item->set_gender(0); //pUser->GetGender());
        item->set_luserwinscore((*it).RealMoney);
        endindex = index + 1;
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
    m_pITableFrame->SendTableData(pIIServerUserItem->GetChairId(), hhmf::SUB_S_QUERY_PLAYLIST, (uint8_t *)lstData.c_str(), lstData.size());
	PlayerRichSorting();
}
// 发送场景消息(押分阶段)
void TableFrameSink::SendGameSceneStart(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;

    LOG(WARNING) << "********** Send GameScene Start **********" << userid;

    hhmf::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime(m_betCountDown);
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);
    //筹码数量
    int32_t dAlljetton = 0;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        hhmf::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_UserInfo[player->GetUserId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);

    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
         scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
         //LOG(ERROR)<<"m_currentchips[]"<<m_currentchips[player->GetChairId()][i];
    }
    scenceStart.add_llimitscore(m_lLimitScore[0]);
    scenceStart.add_llimitscore(m_lLimitScore[1]);
    scenceStart.set_tableid(m_strTableId);
    scenceStart.set_nroundcount(m_iRoundCount);
    scenceStart.set_ncurstate(0); 
    scenceStart.set_ncurretidx(m_iStopIndex); 

    scenceStart.set_luserscore(player->GetUserScore() - dAlljetton);
    scenceStart.set_dwuserid(player->GetUserId());
    scenceStart.set_gender(0);    //player->GetGender());
    scenceStart.set_headboxid(0); //player->GetHeadBoxID());
    scenceStart.set_headerid(player->GetHeaderId());
    scenceStart.set_nickname(player->GetNickName());
    scenceStart.set_nviplevel(0); //player->GetVipLevel());
    //scenceStart.set_szlocation(player->GetLocation());
    scenceStart.set_cbroundid(m_strRoundId);
	scenceStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));

    for(int i=0;i<5;i++)
    {
        scenceStart.add_hhmfwnum(m_iHhmfwNum[i]);
    }
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &uInfoItem : m_pPlayerInfoList)
    {
        xzScore = 0;
        pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
        if (!pServerUserItem)
            nowScore = 0;
        else
        {
            for (int j = 0; j < BET_ITEM_COUNT; j++)
            {
                xzScore += uInfoItem->dAreaJetton[j];
            }
            nowScore = pServerUserItem->GetUserScore() - xzScore;
        }
        /*if (userInfoItem->chairId == m_wBankerUser)
        {
            continue;
        }*/
        if (uInfoItem->headerId > 12)
        {
            LOG_INFO << " ========== 3 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
        }
        hhmf::PlayerListItem *palyerItem = scenceStart.add_players();
        palyerItem->set_dwuserid(uInfoItem->wUserid);
        palyerItem->set_headerid(uInfoItem->headerId);
        palyerItem->set_nviplevel(0);
        palyerItem->set_nickname(uInfoItem->nickName);

        palyerItem->set_luserscore(nowScore);

        palyerItem->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
        palyerItem->set_ltwentywincount(uInfoItem->lTwentyWinCount);
        int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
        palyerItem->set_isdivinemath(shensuanzi);
        palyerItem->set_index(i + 1);
        if (shensuanzi == 1)
            palyerItem->set_index(0);

        palyerItem->set_isbanker(0);
        //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
        //{
           // palyer->set_isbanker(1);
        //}
        palyerItem->set_gender(0);
        palyerItem->set_isapplybanker(0);
        palyerItem->set_applybankermulti(0);
        palyerItem->set_ljetton(0);
        palyerItem->set_szlocation(uInfoItem->location);
        palyerItem->set_headboxid(0);
        palyerItem->set_luserwinscore(uInfoItem->RealMoney);
        if (++i >= size)
            break;
    }


    hhmf::ResultStr * lastresult = scenceStart.mutable_nlastresult();
    lastresult->set_ndanshuang(m_lastRetId.iDanshuang);
    lastresult->set_ndaxiao(m_lastRetId.iDaxiao);
    lastresult->set_ncard(m_lastRetId.icard);
    lastresult->set_nhhmf(m_lastRetId.iHhmf);
    lastresult->set_nhonghei(m_lastRetId.iHonghei);

    for(int i=0;i<eMax;i++)
    {   if(m_lastRetId.bReturnMoney[i])
        {
            lastresult->add_nwinmutic(0);
        }
        else if(m_lastRetId.muticlList[i])
        {
            lastresult->add_nwinmutic(m_lastRetId.muticlList[i]);
        }
        else
        {
            lastresult->add_nwinmutic(-1);
        }
    }


    //记录
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {

        hhmf::ResultStr * sResultList = scenceStart.add_sresultlist();
        sResultList->set_ndanshuang(m_vResultRecored[i].iDanshuang);
        sResultList->set_ndaxiao(m_vResultRecored[i].iDaxiao);
        sResultList->set_ncard(m_vResultRecored[i].icard);
        sResultList->set_nhhmf(m_vResultRecored[i].iHhmf);
        sResultList->set_nhonghei(m_vResultRecored[i].iHonghei);

        for(int j=0;j<eMax;j++)
        {   if(m_vResultRecored[i].bReturnMoney[j])
            {
                sResultList->add_nwinmutic(0);
            }
            else if(m_vResultRecored[i].muticlList[j])
            {
                sResultList->add_nwinmutic(m_vResultRecored[i].muticlList[j]);
            }
            else
            {
                sResultList->add_nwinmutic(-1);
            }
        }

    }
    /*gameStart.set_lfeiqincount(fqWin);
    gameStart.set_lzoushoucount(zsWin);*/


    vector<OpenResultInfo> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();

    for (int i = 0;i < BET_ITEM_COUNT; ++i)
    {
        vResultList = m_vOpenResultListCount_last[i];
        if (openCount == 0)
            scenceStart.add_notopencount(0);
        else
        {
            scenceStart.add_notopencount(vResultList[openCount - 1].notOpenCount);
        }
    }

    std::string sendData = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), hhmf::SUB_S_SCENE_START, (uint8_t *)sendData.c_str(), sendData.size());
}

// 发送场景消息(结算阶段)
void TableFrameSink::SendGameSceneEnd(int64_t userid, bool lookon)
{
    shared_ptr<CServerUserItem> player = m_pITableFrame->GetUserItemByUserId(userid);

    if (!player)
        return;


    hhmf::CMD_S_Scene_GameStart scenceStart;

    scenceStart.set_cbplacetime((int32_t)(m_endCountDown ));
    scenceStart.set_cbtimeleave((int32_t)time(nullptr) - m_EapseTime);


    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        hhmf::CMD_AeraInfo *areainfo = scenceStart.add_aerainfo();
        areainfo->set_selfjettonscore(m_UserInfo[player->GetUserId()]->dAreaJetton[i]);
        areainfo->set_lalljettonscore(m_pBetArea->mifourAreajetton[i]);

    }
    for(int i=0;i<MAX_CHOUMA_NUM;i++)
    {
         scenceStart.add_betitems(m_currentchips[player->GetChairId()][i]);
    }
    scenceStart.add_llimitscore(m_lLimitScore[0]);
    scenceStart.add_llimitscore(m_lLimitScore[1]);
    scenceStart.set_tableid(m_strTableId);
    scenceStart.set_nroundcount(m_iRoundCount);

    scenceStart.set_ncurstate(1); //游戏状态 0,押注状态；1,结算状态
    scenceStart.set_ncurretidx(m_iStopIndex);
    scenceStart.set_luserscore(player->GetUserScore());
    scenceStart.set_dwuserid(player->GetUserId());
    scenceStart.set_gender(0);
    scenceStart.set_headboxid(0);
    scenceStart.set_headerid(player->GetHeaderId());
    scenceStart.set_nickname(player->GetNickName());
    scenceStart.set_nviplevel(0);
    scenceStart.set_cbroundid(m_strRoundId);
    scenceStart.set_onlinenum(m_pITableFrame->GetPlayerCount(true));
    for(int i=0;i<5;i++)
    {
        scenceStart.add_hhmfwnum(m_iHhmfwNum[i]);
    }
    int i = 0;
	uint32_t size = 6;
	int64_t nowScore = 0;
	int64_t xzScore = 0;
	shared_ptr<IServerUserItem> pServerUserItem;
    for (auto &uInfoItem : m_pPlayerInfoList)
    {
        xzScore = 0;
        pServerUserItem = m_pITableFrame->GetTableUserItem(uInfoItem->wCharid);
        if (!pServerUserItem)
            nowScore = 0;
        else
        {
            /*for (int j = 0; j < BET_ITEM_COUNT; j++)
            {
                xzScore += uInfoItem->dAreaJetton[j];
            }*/
            nowScore = pServerUserItem->GetUserScore();
        }
        /*if (userInfoItem->chairId == m_wBankerUser)
        {
            continue;
        }*/
        if (uInfoItem->headerId>12)
        {
            LOG_INFO << " ========== 4 wUserid " << uInfoItem->wUserid << " == headerId:" << uInfoItem->headerId;
        }
        hhmf::PlayerListItem *palyerItem = scenceStart.add_players();
        palyerItem->set_dwuserid(uInfoItem->wUserid);
        palyerItem->set_headerid(uInfoItem->headerId);
        palyerItem->set_nviplevel(0);
        palyerItem->set_nickname(uInfoItem->nickName);

        palyerItem->set_luserscore(nowScore);

        palyerItem->set_ltwentywinscore(uInfoItem->lTwentyAllUserScore);
        palyerItem->set_ltwentywincount(uInfoItem->lTwentyWinCount);
        int shensuanzi = uInfoItem->wUserid == m_ShenSuanZiId ? 1 : 0;
        palyerItem->set_isdivinemath(shensuanzi);
        palyerItem->set_index(i + 1);
        if (shensuanzi == 1)
            palyerItem->set_index(0);

        palyerItem->set_isbanker(0);
        //if (m_wBankerUser != INVALID_CHAIR && m_wBankerUser == userInfoItem->chairId) // 庄家
        //{
           // palyer->set_isbanker(1);
        //}
        palyerItem->set_gender(0);
        palyerItem->set_isapplybanker(0);
        palyerItem->set_applybankermulti(0);
        palyerItem->set_ljetton(0);
        palyerItem->set_szlocation(uInfoItem->location);
        palyerItem->set_headboxid(0);
        palyerItem->set_luserwinscore(uInfoItem->RealMoney);
        if (++i >= size)
            break;
    }


    hhmf::ResultStr * lastresult = scenceStart.mutable_nlastresult();

    for(int i=0;i<eMax;i++)
    {   if(m_lastRetId.bReturnMoney[i])
        {
            lastresult->add_nwinmutic(0);
        }
        else if(m_lastRetId.muticlList[i])
        {
            lastresult->add_nwinmutic(m_lastRetId.muticlList[i]);
        }
        else
        {
            lastresult->add_nwinmutic(-1);
        }
    }

    lastresult->set_ndanshuang(m_lastRetId.iDanshuang);
    lastresult->set_ndaxiao(m_lastRetId.iDaxiao);
    lastresult->set_ncard(m_lastRetId.icard);
    lastresult->set_nhhmf(m_lastRetId.iHhmf);
    lastresult->set_nhonghei(m_lastRetId.iHonghei);


    //记录
    for (int i = m_vResultRecored.size() - 1; i >= 0; i--)
    {

        hhmf::ResultStr * sResultList = scenceStart.add_sresultlist();
        sResultList->set_ndanshuang(m_vResultRecored[i].iDanshuang);
        sResultList->set_ndaxiao(m_vResultRecored[i].iDaxiao);
        sResultList->set_ncard(m_vResultRecored[i].icard);
        sResultList->set_nhhmf(m_vResultRecored[i].iHhmf);
        sResultList->set_nhonghei(m_vResultRecored[i].iHonghei);
        for(int j=0;j<eMax;j++)
        {   if(m_vResultRecored[i].bReturnMoney[j])
            {
                sResultList->add_nwinmutic(0);
            }
            else if(m_vResultRecored[i].muticlList[j])
            {
                sResultList->add_nwinmutic(m_vResultRecored[i].muticlList[j]);
            }
            else
            {
                sResultList->add_nwinmutic(-1);
            }
        }

    }



    vector<OpenResultInfo> vResultList;
	vResultList = m_vOpenResultListCount_last[0];
	int32_t openCount = vResultList.size();
    for (int i = 0;i < BET_ITEM_COUNT; ++i)
    {
        vResultList = m_vOpenResultListCount_last[i];
        if (openCount == 0)
            scenceStart.add_notopencount(0);
        else
        {
            scenceStart.add_notopencount(vResultList[openCount - 1].notOpenCount);
        }
    }

    std::string sendData = scenceStart.SerializeAsString();
    m_pITableFrame->SendTableData(player->GetChairId(), hhmf::SUB_S_SCENE_START, (uint8_t *)sendData.c_str(), sendData.size());

    LOG(WARNING) << "********** 发送场景消息(结算阶段) **********" << userid;
}

// 押分判断
int TableFrameSink::PlayerJettonPanDuan(int index, int64_t score, int32_t chairid, shared_ptr<CServerUserItem> &pSelf)
{
    if (index < 0 || index >= BET_ITEM_COUNT || score <= 0)
    {
        LOG(ERROR) << "---------押分判断:失败 --------index=" << index << ",score=" << score;
        return 1;
    }

    int64_t allBetScore = 0;
    for (int i = 0; i < BET_ITEM_COUNT; i++)
    {
        // allBetScore += m_pPlayerList[chairid].dAreaJetton[i];
        allBetScore += m_UserInfo[pSelf->GetUserId()]->dAreaJetton[i];
    }

    // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
    /*
    if (m_nAreaMaxJetton[index] < m_pBetArea->mifourAreajetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }
    */
    
    // 判断是否超过区域最大押分   < 玩家每个区域的总下分  + 本次玩家押分值
    if (m_nAreaMaxJetton[index] < m_UserInfo[pSelf->GetUserId()]->dAreaJetton[index] + score)
    {
        LOG(ERROR) << "---------超过区域最大押分--------" << m_nAreaMaxJetton[index];
        return 2;
    }

    // 判断是否小于准许进入分
    int64_t enterMinScore = m_pGameRoomInfo->enterMinScore; // m_pITableFrame->GetUserEnterMinScore();
    if (pSelf->GetUserScore() - allBetScore - score < enterMinScore)
    {
        LOG(WARNING) << "---------小于准许进入分--------" << " UserId:" << pSelf->GetUserId();
        return 3;
    }
	// 押注项超出限制范围
    // if (m_nAreaMaxJetton[index] < m_pBetArea->mifourAreajetton[index] + score)
    // {
    //     LOG(ERROR) << "------押注项超出限制范围-----" << m_nAreaMaxJetton[index] <<" < "<<score << " + "<< m_pBetArea->mifourAreajetton[index];
    //     return 4;
    // }
    bool bFind = false;
    int leng =m_cbJettonMultiple.size();
    for(int i=0;i<leng;i++)
    {
        if(m_cbJettonMultiple[i]==score)
        {
            bFind = true;
            break;
        }
    }
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<score;
        return 4;       // 下注的筹码值无效
    }

    //已经下的总注
    if(pSelf->GetUserScore() < allBetScore + score)
    {
        LOG_ERROR << " Real User Score is not enough ";        
        return 5;      // 押注已达最大的下注金额
    }
    
    // 清缓存筹码
    if (m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton == 0){
        m_UserInfo[pSelf->GetUserId()]->clearJetton();
    }

    // 记录真人押分,有玩家押分则添加
    if(!pSelf->IsAndroidUser()) {

        // 记录押分步骤
        m_replay.addStep((uint32_t)time(nullptr) - m_dwReadyTime,to_string(score),-1,opBet,chairid,index);

        // 记录真人押分(有玩家押分则添加)
        if(m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton == 0)
        {
            LOG(WARNING) << "********** 记录真人押分（有玩家押分则添加）**********" << pSelf->GetUserId();
            m_replay.addPlayer(pSelf->GetUserId(),pSelf->GetAccount(),pSelf->GetUserScore(),chairid);
        }
    }

    tagBetInfo betinfo = {0};
    betinfo.betIdx = index;
    betinfo.betScore = score;
    // 记录玩家押分筹码值
    m_UserInfo[pSelf->GetUserId()]->JettonValues.push_back(betinfo);

    // 允许下注(分数累加)
    m_UserInfo[pSelf->GetUserId()]->dAreaJetton[index] += score;
    // 当前玩家押分
    m_UserInfo[pSelf->GetUserId()]->dCurrentPlayerJetton = allBetScore + score;
    // 押分区增加押分
    m_pBetArea->SetJettonScore(index, score, /*mpBankerManager->GetBankerScore()*/0, pSelf->IsAndroidUser());
    //一段时间内各区域其他玩家的总下注
    // for (int i = 0; i < MAX_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> UserItem = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == UserItem || i==chairid)
    //         continue;
    //     m_pPlayerList[i].dsliceSecondsAllJetton += score;
    //     m_pPlayerList[i].dsliceSecondsAllAreaJetton[index] += score;
    // }
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for (auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
        if (nullptr == pUserItem || userInfoItem->wCharid==chairid)
            continue;
        userInfoItem->dsliceSecondsAllJetton += score;
        userInfoItem->dsliceSecondsAllAreaJetton[index] += score;
    }
    return 0;
}

//发送一段时间内相对自己的其他所有玩家的总押注数据
void TableFrameSink::OnTimerSendOtherBetJetton()
{
    // for (int i = 0; i < MAX_PLAYER; ++i)
    // {
    //     shared_ptr<CServerUserItem> pUserItem = this->m_pITableFrame->GetTableUserItem(i);
    //     if (nullptr == pUserItem)
    //         continue;
    //     // if (!m_pPlayerList[i].Isplaying)
    //     // {
    //     //     continue;
    //     // }
    //     // if (pUserItem->IsAndroidUser())
    //     // {
    //     //     continue;
    //     // }
    //     if (m_pPlayerList[i].dsliceSecondsAllJetton <= 0)
    //     {
    //         continue;
    //     }
    //     // 押分区增加押分
    //     // int32_t score = 0;
    //     // for (int index = 0; index < MAX_JETTON_AREA; index++)
    //     // {
    //     //     score = m_pPlayerList[i].dsliceSecondsAllAreaJetton[index];
    //     //     if ( score <= 0 ) continue;
    //     //     m_pBetArea->SetJettonScore(index, score, 0, pUserItem->IsAndroidUser());
    //     // }

    //     Jsys::CMD_S_OtherPlaceJetSuccess otherPlaceJetSucc;
    //     otherPlaceJetSucc.Clear();

    //     int64_t userid = -1;
    //     otherPlaceJetSucc.set_dwuserid(userid);
    //     for (int x = 1; x <= MAX_JETTON_AREA; x++)
    //     {
    //         otherPlaceJetSucc.add_alljettonscore(m_pPlayerList[i].dsliceSecondsAllAreaJetton[x-1]);
    //         otherPlaceJetSucc.add_alljettonscore(m_pBetArea->mifourAreajetton[x - 1]);
    //     }

    //     std::string data = otherPlaceJetSucc.SerializeAsString();
    //     // m_pITableFrame->SendTableData(i, Jsys::SUB_S_JETTON_SUCCESS_OTHER, (uint8_t *)data.c_str(), data.size());

    //     m_pPlayerList[i].clearOtherJetton();
    // }
}

void TableFrameSink::openSLog(const char *p, ...)
{
    if(!m_bWritelog) 
        return;
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//hhmf//hhmf%s_%d%d%d_%d%d.log",m_strTableId.c_str(),t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId,m_pITableFrame->GetTableId());
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d] ", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}

//设置当局游戏详情
void TableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
    hhmf::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
    hhmf::ResultStr* res = details.mutable_res();
    res->set_ndanshuang(m_strResultInfo.iDanshuang);
    res->set_ndaxiao(m_strResultInfo.iDaxiao);
    res->set_ncard(m_strResultInfo.icard);
    res->set_nhhmf(m_strResultInfo.iHhmf);
    res->set_nhonghei(m_strResultInfo.iHonghei);

    for(int i=0;i<eMax;i++)
    {   if(m_strResultInfo.bReturnMoney[i])
        {
            res->add_nwinmutic(0);
        }
        else if(m_strResultInfo.muticlList[i])
        {
            res->add_nwinmutic(m_strResultInfo.muticlList[i]);
        }
        else
        {
            res->add_nwinmutic(-1);
        }
    }



	details.set_idxmul(m_iRetMul);					//结果倍数
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->RealMoney); //总输赢积分

	//if (!m_pITableFrame->IsExistUser(chairId))
	{
		for (int betIdx = 0;betIdx < BET_ITEM_COUNT; betIdx++)
		{
			int64_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->dAreaJetton[betIdx];
            int32_t	nMuilt = m_strResultInfo.muticlList[betIdx];// 倍率表m_nMuilt
			{
                hhmf::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//区域倍数
				detail->set_dareaidmul(nMuilt);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
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

//更新100局开奖结果信息
void TableFrameSink::updateResultList(strResultInfo result)
{
    vector<OpenResultInfo> vResultList;
    for (int i = 0;i < BET_ITEM_COUNT;++i)
    {
        OpenResultInfo iResultIndex;
        OpenResultInfo iResultIndex_ls;
        iResultIndex.init(result);
        iResultIndex_ls.init(result);

        vResultList = m_vOpenResultListCount[i];
        if (vResultList.size() >= OPEN_RESULT_LIST_COUNT)
        {
            vResultList.erase(vResultList.begin());
        }
        int resultCount = vResultList.size();
        if (resultCount > 0)
        {
            iResultIndex_ls = vResultList[resultCount - 1];
            iResultIndex.notOpenCount = (result.winList[i]) ? 0 : (iResultIndex_ls.notOpenCount + 1);
        }
        else
        {
            iResultIndex.notOpenCount = (result.winList[i]) ? 0 : (iResultIndex.notOpenCount + 1);
        }
        vResultList.push_back(iResultIndex);
        m_vOpenResultListCount[i] = vResultList;

    }

}


void TableFrameSink::sendGameRecord(int32_t chairid)
{
	//记录
    hhmf::CMD_S_GameRecord gameRecordRsp;

    vector<OpenResultInfo> vResultList;
    vResultList = m_vOpenResultListCount_last[0];
    int32_t openCount = vResultList.size();
    gameRecordRsp.set_opencount(openCount);
    gameRecordRsp.set_openallcount(m_iRoundCount);
    for(int i=0;i<eMax;i++)
    {
        vResultList = m_vOpenResultListCount_last[i];

        hhmf::GameRecordList *gamerecordList = gameRecordRsp.add_recordlists();
        for (int j = 0; j < vResultList.size(); j++)
        {
            hhmf::GameRecordListItem *listItem = gamerecordList->add_records();
            listItem->set_danshuang(vResultList[j].iIsDanShuangSevenWang);
            listItem->set_daxiao(vResultList[j].iIsDaXiaoSevenWang);
            listItem->set_notopencount(vResultList[j].notOpenCount);
            listItem->set_hhmf(vResultList[j].iHhmfw);
            listItem->set_honghei(vResultList[j].iHongSeHeiseWang);

        }
    }


	std::string sendData = gameRecordRsp.SerializeAsString();
    m_pITableFrame->SendTableData(chairid, hhmf::SUB_S_QUERY_GAMERECORD, (uint8_t *)sendData.c_str(), sendData.size());
}

void TableFrameSink::getJSodds()
{

}

void TableFrameSink::RefreshPlayerList(bool isGameEnd)
{
	m_pPlayerInfoList_20.clear();
	shared_ptr<IServerUserItem> pServerUserItem;
    for(int i=0;i<m_pPlayerInfoList.size();i++)
	{
        if (!m_pPlayerInfoList[i])
            continue;
        pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->wCharid);
		if (!pServerUserItem)
			continue;
        m_pPlayerInfoList[i]->GetTwentyWinScore();
        m_pPlayerInfoList[i]->RefreshGameRusult(isGameEnd);
	}

    for(int i=0;i<m_pPlayerInfoList.size();i++)
     {
        if (!m_pPlayerInfoList[i])
           continue;
        pServerUserItem = m_pITableFrame->GetTableUserItem(m_pPlayerInfoList[i]->wCharid);
        if (!pServerUserItem)
            continue;
        m_pPlayerInfoList_20.push_back((*m_pPlayerInfoList[i]));
        LOG(ERROR) << "old二十局赢分:"<< (m_pPlayerInfoList[i]->GetTwentyWinScore());
        LOG(ERROR) << "二十局赢分:"<< (m_pPlayerInfoList_20[i].GetTwentyWinScore());
     }
	//m_pPlayerInfoList_20.assign(m_pPlayerInfoList.begin(), m_pPlayerInfoList.end());
	m_LoopThread->getLoop()->cancel(m_TimerIdResetPlayerList);
	PlayerRichSorting();
	//更新开奖结果
	m_vOpenResultListCount_last.clear();
	for (auto &it : m_vOpenResultListCount)
	{
		m_vOpenResultListCount_last[it.first] = it.second;
	}
	if (isGameEnd)
	{
		shared_ptr<IServerUserItem> pUserItem;
		shared_ptr<UserInfo> userInfoItem;
		for (auto &it : m_UserInfo)
		{
			userInfoItem = it.second;
			pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->wCharid);
			if (!pUserItem)
				continue;
			sendGameRecord(userInfoItem->wCharid);
		}
	}	
}

void TableFrameSink::SortPlayerList()
{
	for (int i = 0;i < m_pPlayerInfoList_20.size();i++)
	{
		for (int j = i + 1;j < m_pPlayerInfoList_20.size();j++)
		{
			UserInfo userinfo;
			if (m_pPlayerInfoList_20[i].GetTwentyWinScore() < m_pPlayerInfoList_20[j].GetTwentyWinScore())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyWinScore() == m_pPlayerInfoList_20[j].GetTwentyWinScore()
				&& m_pPlayerInfoList_20[i].GetTwentyJetton() < m_pPlayerInfoList_20[j].GetTwentyJetton())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
			else if (m_pPlayerInfoList_20[i].GetTwentyJetton() == m_pPlayerInfoList_20[j].GetTwentyJetton()
				&& m_pPlayerInfoList_20[i].GetTwentyWin() < m_pPlayerInfoList_20[j].GetTwentyWin())
			{
				userinfo = m_pPlayerInfoList_20[i];
				m_pPlayerInfoList_20[i] = m_pPlayerInfoList_20[j];
				m_pPlayerInfoList_20[j] = userinfo;
			}
		}

	}
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<TableFrameSink> pTableFrameSink(new TableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink> &pSink)
{
    pSink.reset();
}
