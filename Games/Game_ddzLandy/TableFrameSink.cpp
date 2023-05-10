#include <glog/logging.h>
#include "Log.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Mutex.h>

#include "stdafx.h"

#include "PathMagic.h"
#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdint.h>
#include <string>
#include "CString.h"

#include "proto/Common.Message.pb.h"
#include "proto/ddz.Message.pb.h"
#include "proto/Game.Common.pb.h"
#include "proto/GameServer.Message.pb.h"

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/INIHelp.h"
#include "public/IServerUserItem.h"
#include "public/ITableFrameSink.h"
#include "public/ITableFrame.h"
#include "public/StdRandom.h"

#include "CMD_Game.h"
#include "GameLogic.h"
#include "HistoryScore.h"
#include "TableFrameSink.h"

//////////////////////////////////////////////////////////////////////////////////
#define IDI_OFFLINE_TRUSTEE			(10)
#define TIME_DEAL_CARD				2					//发牌时间(2)
#define TIME_CALL_BANKER			15					//叫地主时间(15)
#define TIME_CALL_BANKERSend		5					//后续叫地主时间(5)
#define TIME_ADD_DOUBLE				10					//加陪时间(5)
#define	TIME_OUT_CARD				15					//出牌时间(15)
#define	TIME_OUT_CARDTrust			2					//托管出牌时间(2)
#define TIME_FIRST_OUT_CARD			25					//第一次出牌时间
#define TIME_TICK_USER				5					//剔除用户时间 15
#define TIME_CHECK_END				600					//检测桌子结束

//////////////////////////////////////////////////////////////////////////////////
DWORD                     openCount = 1;
//////////////////////////////////////////////////////////////////////////

bool IsGameCheatUser(int flag)
{
    bool ret= false;
    return ret;
}

//构造函数
CTableFrameSink::CTableFrameSink()
{
    //游戏记录
    //m_GameRecord.Empty();
    ZeroMemory(m_szNickName,sizeof(m_szNickName));
    ZeroMemory(m_bTrustee,sizeof(m_bTrustee));

    m_bOffLineTrustee = false;
    m_bFisrtCall = true;
    m_cbNoCallCount = 0;

    //炸弹变量
    m_wFirstUser=INVALID_CHAIR;
    m_wBankerUser=INVALID_CHAIR;
    m_wCurrentUser=INVALID_CHAIR;
    ZeroMemory(m_cbOutCardCount,sizeof(m_cbOutCardCount));

    //游戏变量
    m_cbBombCount=0;
    m_cbFirstCall= INVALID_CHAIR;
    ZeroMemory(m_cbEachBombCount,sizeof(m_cbEachBombCount));

    //叫分信息
    m_cbBankerScore=0;
    ZeroMemory(m_cbScoreInfo,sizeof(m_cbScoreInfo));
    ZeroMemory(m_cbStakeInfo, sizeof(m_cbStakeInfo));

    //出牌信息
    m_cbTurnCardCount=0;
    m_wTurnWiner=INVALID_CHAIR;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

    //扑克信息
    ZeroMemory(m_cbBankerCard,sizeof(m_cbBankerCard));
    ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
    ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));

    ZeroMemory(m_cbSurplusCard,sizeof(m_cbSurplusCard));
    m_cbSurplusCount=0;
	m_bReplayRecordStart = false;
	m_bReplayRecordStartFlag = false;
	m_WinScoreSystem = 0;

    //剩余时间
    m_cbRemainTime = 0;
    m_MSGID= 0;

    // m_TimerLoopThread->startLoop();
    m_isflag = true;
	stockWeak = 0.0f;

    //累计匹配时长
    totalMatchSeconds_ = 0;
    //分片匹配时长(可配置)
    sliceMatchSeconds_ = 0.2f;
    //超时匹配时长(可配置)
    timeoutMatchSeconds_ = 3.6f;
    //机器人候补空位超时时长(可配置)
    timeoutAddAndroidSeconds_ = 1.4f;

	m_bTestGame = 0;
	m_cbGoodCardValue = 0;
	m_cbGoodCardTime = 1;
	m_cbTestUser = 0;
	m_bhaveCancelTrustee = false;
    return;
}

//析构函数
CTableFrameSink::~CTableFrameSink()
{
}


int64_t CTableFrameSink::GetCellScore()
{
    return m_lCellScore;
}


int CTableFrameSink::OnNeedAndroid()
{
    int bRet = 1;

    return bRet;
}

bool CTableFrameSink::OnCheckGameStart()
{
    bool bRet = false;
    DWORD wMaxPlayer = m_pITableFrame->GetMaxPlayerCount();
    DWORD wReadyPlayerCount = 0;
    DWORD wUserCount = 0;
    // loop to sum all special player.
    for (DWORD i=0; i<wMaxPlayer; i++)
    {
        shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if (!pIServerUserItem) continue;

        wUserCount++;
        if (pIServerUserItem->GetUserStatus()== sReady )
        {
            wReadyPlayerCount++;
        }
    }
    if (wReadyPlayerCount == wMaxPlayer)
    {
        return true;
    }
    return bRet;
}

//初始化
bool  CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    //查询接口
    ASSERT(pTableFrame!=NULL);
    m_pITableFrame=pTableFrame;
    if (m_pITableFrame==NULL) return false;

	m_TimerLoopThread = m_pITableFrame->GetLoopThread();
    //开始模式
    m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
    m_cbGameStatus = GAME_SCENE_FREE;

    m_lCellScore = m_pITableFrame->GetGameRoomInfo()->floorScore;
    m_replay.cellscore = m_lCellScore;
    m_replay.roomname = m_pITableFrame->GetGameRoomInfo()->roomName;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata

    if(access("./log/ddz", 0) == -1)
        mkdir("./log/ddz",0777);

	ReadConfig();

    return true;
}
//读取配置
bool CTableFrameSink::ReadConfig()
{
	//=====config=====
	if (!boost::filesystem::exists("./conf/ddz_config.ini"))
	{
		openLog("./conf/ddz_config.ini not exists");
		return false;
	}

	boost::property_tree::ptree pt;
	boost::property_tree::read_ini("./conf/ddz_config.ini", pt);

	string StorageRationame = "GameServer_";
	StorageRationame += std::to_string(m_pITableFrame->GetGameRoomInfo()->roomId % 1000);
	StorageRationame += ".StorageRatio";
	m_StorageRatio = pt.get<int>(StorageRationame.c_str(), 50);

	string StorageMinRationame = "GameServer_";
	StorageMinRationame += std::to_string(m_pITableFrame->GetGameRoomInfo()->roomId % 1000);
	StorageMinRationame += ".StorageMinRatio";
	m_StorageMinRatio = pt.get<int>(StorageMinRationame.c_str(), 80);

	string StorageMaxRationame = "GameServer_";
	StorageMaxRationame += std::to_string(m_pITableFrame->GetGameRoomInfo()->roomId % 1000);
	StorageMaxRationame += ".StorageMaxRatio";
    m_StorageMaxRatio = pt.get<int>(StorageMaxRationame.c_str(), 10);

	/*string ISReplayRecordname = "GameServer_";
	ISReplayRecordname += std::to_string(m_pITableFrame->GetGameRoomInfo()->roomId % 1000);
	ISReplayRecordname += ".ISReplayRecord";
	int   ReplayRecord = pt.get<int>(ISReplayRecordname.c_str(), 0);

	if (1 == ReplayRecord)
		m_bReplayRecordStartFlag = true;
	else
		m_bReplayRecordStartFlag = false;*/

	stockWeak = pt.get<double>("GAME_CONF.STOCK_WEAK", 1.0);
	m_bTestGame = pt.get<int32_t>("GAME_CONF.TESTGAME", 0);
	m_cbGoodCardValue = pt.get<int32_t>("GAME_CONF.CARDVALUE", 0);
	m_cbGoodCardTime = pt.get<int32_t>("GAME_CONF.CARDTIME", 1);
	m_cbTestUser = pt.get<WORD>("GAME_CONF.TESTUSER", 0);
	return true;
}
//复位桌子
VOID CTableFrameSink::RepositionSink()
{
    //游戏记录
    //m_GameRecord.Empty();
    m_TimerLoopThread->getLoop()->cancel(m_TimerId);
    m_TimerLoopThread->getLoop()->cancel(m_TimerCallBankerId);
    m_TimerLoopThread->getLoop()->cancel(m_TimerCallStakeId);
    m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
    //游戏变量
    if(m_isflag == false)//游戏维护，踢出所有的玩家,游戏不再开始,这个是游戏服中途维护做的
    {
        for (WORD i=0;i<GAME_PLAYER;i++)
        {
            shared_ptr<IServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(i) ;
            if(pServerUserItem==nullptr)
                continue;

            m_pITableFrame->ClearTableUser(i,true,true,ERROR_ENTERROOM_SERVERSTOP);
        }
        return;
    }
    m_cbBombCount=0;
    m_wBankerUser=INVALID_CHAIR;
    m_wCurrentUser=INVALID_CHAIR;
    ZeroMemory(m_cbOutCardCount,sizeof(m_cbOutCardCount));
    ZeroMemory(m_cbEachBombCount,sizeof(m_cbEachBombCount));
    ZeroMemory(m_bTrustee,sizeof(m_bTrustee));

    //叫分信息
    m_cbBankerScore=0;
    ZeroMemory(m_cbScoreInfo,sizeof(m_cbScoreInfo));
    ZeroMemory(m_cbStakeInfo, sizeof(m_cbStakeInfo));

    //出牌信息
    m_cbTurnCardCount=0;
    m_wTurnWiner=INVALID_CHAIR;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

    //扑克信息
    ZeroMemory(m_cbBankerCard,sizeof(m_cbBankerCard));
    ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
    ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));
	ZeroMemory(m_cbHandCardAllData, sizeof(m_cbHandCardAllData));

    ZeroMemory(m_cbOutCardListCount,sizeof(m_cbOutCardListCount));
    ZeroMemory(m_cbOutCardList,sizeof(m_cbOutCardList));

    ZeroMemory(m_cbSurplusCard,sizeof(m_cbSurplusCard));
    m_cbSurplusCount=0;
    ZeroMemory(m_EndWinScore,sizeof(m_EndWinScore));

    //剩余时间
    m_cbRemainTime = 0;
    m_MSGID = 0;
	m_replay.clear();
    //m_dwStartGameTime = 0;		//开始时间
	m_dwEndTime = 0;
	m_dwReadyTime = 0;			//ready时间
	m_bhaveCancelTrustee = false;
    return;
}

//配置桌子
bool CTableFrameSink::Initialization()
{
    if (nullptr != m_pITableFrame)
    {
        m_dwlCellScore = m_pITableFrame->GetGameRoomInfo()->ceilScore;
        m_pGameRoomKindInfo = m_pITableFrame->GetGameRoomInfo();
    }
    return true;
}

//消费能力
double CTableFrameSink::QueryConsumeQuota(shared_ptr<IServerUserItem> pIServerUserItem)
{
    return 0L;
}

//最少积分
double CTableFrameSink::QueryLessEnterScore(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    return 0L;
}

//游戏开始,游戏开始是指已经重新匹配人数到达三个人，并且有人叫三分，或者叫了一圈,那么就开始发牌啊
void CTableFrameSink::OnGameStart()
{
    RepositionSink();
	if (m_isflag == false)//游戏维护，踢出所有的玩家,游戏不再开始,这个是游戏服中途维护做的
	{
		for (WORD i = 0;i < GAME_PLAYER;i++)
		{
			shared_ptr<IServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (pServerUserItem == nullptr)
				continue;

			m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SERVERSTOP);
        }
		return;
	}
	ReadConfig();
    timeCount = 0;
    m_bFisrtCall = true;
    m_cbNoCallCount += 1;
    m_dwStartTime = 1;
    //tmp......
    RepayRecordStart();

	m_dwStartGameTime = chrono::system_clock::now();
	m_dwReadyTime = (uint32_t)time(NULL);

    //设置状
    m_pITableFrame->SetGameStatus(GAME_STATUS_START);
    //状态设置为开始叫庄
    m_cbGameStatus = GAME_SCENE_CALL;

    //混乱扑克
	uint8_t cbRandCard[FULL_COUNT];
	m_GameLogic.RandCardList(cbRandCard,CountArray(cbRandCard));

    ZeroMemory(m_cbSurplusCard,sizeof(m_cbSurplusCard));
    m_cbSurplusCount=0;

    //抽取明牌
    uint8_t cbValidCardData=0;
    uint8_t cbValidCardIndex=0;
    WORD wStartUser=m_wFirstUser;
    WORD wCurrentUser=m_wFirstUser;

    //抽取玩家
    if (wStartUser==INVALID_CHAIR)
    {
        //抽取扑克
        cbValidCardIndex=rand()%DISPATCH_COUNT;
        cbValidCardData=cbRandCard[cbValidCardIndex];

        //设置用户
        wStartUser=m_GameLogic.GetCardValue(cbValidCardData)%GAME_PLAYER;
        wCurrentUser=(wStartUser+cbValidCardIndex/NORMAL_COUNT)%GAME_PLAYER;		
    }
	//测试首先叫庄的玩家 
	if (m_bTestGame == 1)
	{
		for (int i = 0;i < GAME_PLAYER;i++)
		{
			shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (!pServerUserItem->IsAndroidUser())
			{
				wCurrentUser = i;
				break;
			}
		}
		StoreageControl();
	}
	else
	{
		//黑白名单控制
	    //ListControl();
		int32_t testCount = 0;
		int32_t handValue = 0;
		int32_t goodCardCount = 0;
		int32_t cbCardValue = 0;
		int32_t cbAllCardValue = 0;
		//本次请求开始时间戳(微秒)
		//muduo::Timestamp timestart = muduo::Timestamp::now();
		do
		{
			handValue = 0;
			goodCardCount = 0;
			cbCardValue = 0;
			testCount++;
			StoreageControl();			
			//统计牌的权值
			for (int i = 0;i < GAME_PLAYER;i++)
			{
				handValue = m_GameLogic.get_HandCardValue(m_cbHandCardData[i], m_cbHandCardCount[i]);
				cbCardValue += handValue;
				openLog("===洗牌次数=%d,玩家=%d,手牌权值=%d;", testCount, i, handValue);
				if (handValue > m_cbGoodCardValue)
				{
					goodCardCount++;
				}
			}
			if (m_lStockScore <= m_lStorageMin) //杀分模式
			{
				openLog("===杀分模式 洗牌次数=%d,设置次数=%d,好牌权值=%d,本次平均权值=%d;", testCount, m_cbGoodCardTime, m_cbGoodCardValue, cbCardValue / 3);
				bool bHaveSame = bHaveTheSameCard();
				if (!bHaveSame)
				{
					testCount = m_cbGoodCardTime;
				}
			}
			else
			{
				openLog("===正常模式 洗牌次数=%d,设置次数=%d,好牌权值=%d,本次平均权值=%d;", testCount, m_cbGoodCardTime, m_cbGoodCardValue, cbCardValue / 3);
			}			
			cbAllCardValue += cbCardValue / 3;
		} while (testCount < m_cbGoodCardTime && goodCardCount < 2);
		openLog("===总洗牌次数=%d,总平均权值=%d;", testCount, cbAllCardValue / testCount);
		// 打印日志
		//LOG(ERROR) << ">>> 耗时[" << muduo::timeDifference(muduo::Timestamp::now(), timestart) << "]秒 " << __FUNCTION__;
	}    

    //设置用户
    m_wFirstUser=wCurrentUser;
    m_wCurrentUser=wCurrentUser;

	TestCard();
    
    //tmp......
    //RepayRecordStart();
    //发送数据
    m_strRoundId= m_pITableFrame->GetNewRoundId();
	m_replay.gameinfoid = m_strRoundId;
	openLog("===游戏开始 RoundId=%s", m_strRoundId.c_str());
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        CMD_S_GameStart GameStart;
        GameStart.set_dwstartuser(wStartUser);
        GameStart.set_dwcurrentuser( wCurrentUser);
        GameStart.set_dwstarttime( 1);
        GameStart.set_cbtimeleft(TIME_CALL_BANKER);
        GameStart.set_cbroundid(m_strRoundId);

		shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pServerUserItem==NULL)
        {
             continue;//玩家到达这里肯定是三个人的，不可能有空玩家，所以不用了
        }
        if (!pServerUserItem->IsAndroidUser())
        {
            for(int j =0;j<m_cbHandCardCount[i];j++)
            {
                //把牌发给玩家
                GameStart.add_cbcarddata(m_cbHandCardData[i][j]);
            }
            //发送数据
            std::string data = GameStart.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_GAME_START, (uint8_t*)data.c_str(), data.size());

            ////tmp......
            //if(m_bReplayRecordStart)
            //{
            //    for(int k =0;k<m_cbHandCardCount[0];k++)
            //        GameStart.add_cbcarddata0(m_cbHandCardData[0][k]);
            //    for(int k =0;k<m_cbHandCardCount[1];k++)
            //        GameStart.add_cbcarddata1(m_cbHandCardData[1][k]);
            //    for(int k =0;k<m_cbHandCardCount[2];k++)
            //        GameStart.add_cbcarddata2(m_cbHandCardData[2][k]);
            //    std::string data = GameStart.SerializeAsString();
            //    RepayRecord(i,SUB_S_GAME_START, (uint8_t* )data.c_str(),data.size());
            //}
        }
        else
        {
            //机器人数据
            CMD_S_GameStartAi AndroidCard ;
            AndroidCard.set_dwstartuser(wStartUser);
            AndroidCard.set_dwcurrentuser(wCurrentUser);
            AndroidCard.set_dwstarttime(m_dwStartTime);
            //用户扑克
            //把牌发给机器人
            for(int i =0;i<3;i++)
                AndroidCard.add_cbbankercard(m_cbBankerCard[i]);

            for (int i = 0; i < GAME_PLAYER; i++)
            {
                for (WORD j = 0; j < NORMAL_COUNT; j++)
                {
                    AndroidCard.add_cbcarddata(m_cbHandCardData[i][j]);
                }
            }

            std::string Androiddata = AndroidCard.SerializeAsString();
            m_pITableFrame->SendTableData(i, SUB_S_GAME_START, (uint8_t*)Androiddata.c_str(), Androiddata.size());
        }
        //发送数据 // 把每个玩家的状态设置为playing
        if(pServerUserItem->GetUserStatus() != sOffline)
            pServerUserItem->SetUserStatus(sPlaying);

        _sntprintf(m_szNickName[i],LEN_NICKNAME,TEXT("%s"),pServerUserItem->GetNickName().c_str());
        pServerUserItem->setTrustee(false);//这个时候玩家不能离开游戏了
		m_replay.addPlayer(pServerUserItem->GetUserId(), pServerUserItem->GetAccount(), pServerUserItem->GetUserScore(), i);
    }

    //排列扑克
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        m_GameLogic.SortCardList(m_cbHandCardData[i],m_cbHandCardCount[i],ST_ORDER);
    }
    
	openLog("===底牌:0x%02x,0x%02x,0x%02x;", m_cbBankerCard[0], m_cbBankerCard[1], m_cbBankerCard[2]);
    //m_GameRecord+=TEXT("#");
    for(uint8_t i = 0;i<GAME_PLAYER;i++)
    {
        char tmp[100]={0};
        GetValue(m_cbHandCardData[i],m_cbHandCardCount[i],tmp);
        openLog("===玩家手牌:i=%d %s",i,tmp);
    }
    m_cbRemainTime = TIME_CALL_BANKER;
    m_dwTime = (DWORD)time(NULL);
	m_TimerLoopThread->getLoop()->cancel(m_TimerId);
    m_TimerLoopThread->getLoop()->cancel(m_TimerCallBankerId);
    m_TimerCallBankerId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime,boost::bind(&CTableFrameSink::OnCallBankerTimer, this));
    openLog("===TIME_CALL_BANKER m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
    AutoWork();
    return ;
}

void CTableFrameSink::TestCard()
{
    if (0)
	{
		//构造变量
		ZeroMemory(&m_cbHandCardData, sizeof(m_cbHandCardData));
		uint8_t cbHandCard[GAME_PLAYER][NORMAL_COUNT] = {
			{0x4E,0x22,0x01,0x3A,0x29,0x18,0x17,0x07,0x06,0x35,0x25,0x15,0x14,0x33,0x23,0x13,0x03},
			{0x32,0x02,0x31,0x21,0x2D,0x2C,0x2A,0x1A,0x38,0x37,0x36,0x26,0x16,0x05,0x34,0x24,0x04},
			{0x4F,0x11,0x3D,0x1D,0x3C,0x1C,0x0C,0x3B,0x2B,0x1B,0x0B,0x0A,0x39,0x19,0x09,0x28,0x27}
		};
		WORD dwTrueUser = 0;
		int cardIndex = 0;
		for (int i = 0;i < GAME_PLAYER;i++)
		{
			shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem(i);
			if (!pServerUserItem->IsAndroidUser())
			{
                dwTrueUser = i;
				break;
			}
		}
		for (WORD i = 0;i < GAME_PLAYER;i++)
		{
			if (i == dwTrueUser)
			{
				CopyMemory(m_cbHandCardData[dwTrueUser], cbHandCard[m_cbTestUser], sizeof(uint8_t)*NORMAL_COUNT);
			}
			else
			{
				cardIndex++;
				CopyMemory(m_cbHandCardData[i], cbHandCard[cardIndex], sizeof(uint8_t)*NORMAL_COUNT);
			}
		}
		CopyMemory(m_cbHandCardData[(dwTrueUser + 1) % GAME_PLAYER], cbHandCard[(m_cbTestUser + 1) % GAME_PLAYER], sizeof(uint8_t)*NORMAL_COUNT); //下家
		CopyMemory(m_cbHandCardData[(dwTrueUser + GAME_PLAYER - 1) % GAME_PLAYER], cbHandCard[(m_cbTestUser + GAME_PLAYER - 1) % GAME_PLAYER], sizeof(uint8_t)*NORMAL_COUNT); //上家
		m_cbBankerCard[0] = 0x12;
		m_cbBankerCard[1] = 0x0D;
		m_cbBankerCard[2] = 0x08;
	}
}

bool CTableFrameSink::ReadCardConfig()
{
	if (m_bTestGame == 0)
		return false;
	m_listTestCardArray.clear();
	if (!boost::filesystem::exists("./conf/cards_ddz.json"))
		return false;
	boost::property_tree::ptree pt;
	try {
		boost::property_tree::read_json("./conf/cards_ddz.json", pt);
	}
	catch (...) {
		LOG(INFO) << "cards_ddz.json firmat error !!!!!!!!!!!!!!!";
		return false;
	}

	boost::property_tree::ptree  pCard_weave = pt.get_child("card_weave");
	boost::property_tree::ptree cards;
	for (auto& weave : pCard_weave) {
		cards = weave.second.get_child("player_0");
		for (auto& it : cards) {
			m_listTestCardArray.push_back(stod(it.second.data()));
		}
		cards = weave.second.get_child("player_1");
		for (auto& it : cards) {
			m_listTestCardArray.push_back(stod(it.second.data()));
		}
		cards = weave.second.get_child("player_2");
		for (auto& it : cards) {
			m_listTestCardArray.push_back(stod(it.second.data()));
		}
		cards = weave.second.get_child("banker");
		for (auto& it : cards) {
			m_listTestCardArray.push_back(stod(it.second.data()));
		}
	}

	return true;
}
//解散结束
bool  CTableFrameSink::OnDismissGameConclude()
{
    return true;
}
//常规结束
bool  CTableFrameSink::OnNormalGameConclude(WORD wChairID)
{
    //游戏记录
    //m_GameRecord+=TEXT("END-NORMAL:#");
    //变量定义
    ddz::DDZ_CMD_S_GameEnd GameEnd;
    openLog("**********************OnNormalGameConclude*************************** ");
    //设置变量
    GameEnd.set_cbcallscore(m_cbBankerScore);
    //炸弹信息
    GameEnd.set_cbbombtimes(m_cbBombCount);
    GameEnd.set_cbmissiletimes(0);						//火箭倍数
    //用户扑克
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        GameEnd.add_cbdoubleinfo(m_cbScoreInfo[i]);
        //拷贝扑克
        GameEnd.add_cbcardcount(m_cbHandCardCount[i]);
        for(WORD j=0;j<m_cbHandCardCount[i];j++)
            GameEnd.add_cbhandcarddata(m_cbHandCardData[i][j]);
    }
    //炸弹统计
    LONG lScoreTimes=__max(1, m_cbBankerScore);
    for (uint8_t i=0;i<m_cbBombCount;i++)
        lScoreTimes*=2;
    //春天判断
    if (wChairID==m_wBankerUser)
    {
        //用户定义
        WORD wUser1=(m_wBankerUser+1)%GAME_PLAYER;
        WORD wUser2=(m_wBankerUser+2)%GAME_PLAYER;
        //用户判断
        if ((m_cbOutCardCount[wUser1]==0)&&(m_cbOutCardCount[wUser2]==0))
        {
            lScoreTimes*=2;
            GameEnd.set_bchuntian(TRUE);
            GameEnd.set_bfanchuntian(FALSE);
			m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, "1", -1, opSpring, m_wBankerUser, m_wBankerUser);
        }
        else
        {
            GameEnd.set_bchuntian(FALSE);
            GameEnd.set_bfanchuntian(FALSE);
        }
    }
    //fanchun春天判断
    if (wChairID!=m_wBankerUser)
    {
        if (m_cbOutCardCount[m_wBankerUser]==1)
        {
            lScoreTimes*=2;
            GameEnd.set_bchuntian(FALSE);
            GameEnd.set_bfanchuntian(TRUE);
			m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, "2", -1, opSpring, m_wBankerUser, m_wBankerUser);
        }
        else
        {
            GameEnd.set_bchuntian(FALSE);
            GameEnd.set_bfanchuntian(FALSE);
        }
    }
    GameEnd.set_bwinmaxscore(FALSE);
	int64_t	lTempUserScpre[GAME_PLAYER] = { 0 };
	int64_t lWinScore = 0;
    int tempStakeTimes = 0;
	//农民总赢的金币
	int64_t iUserTotalWinScore = 0;
	//农民总输的金币
	int64_t iUserTotalLoseScore = 0;
	//闲家本金
	int64_t llUserBaseScore[GAME_PLAYER] = { 0 };
	//庄家总输赢金币
	int64_t iBankerTotalWinScore = 0;
	int64_t llBankerBaseScore = m_pITableFrame->GetTableUserItem(m_wBankerUser)->GetUserScore();
    //统计积分
    for (WORD i = 0; i < GAME_PLAYER; i++)
    {
        //变量定义
		int64_t lUserTimes=0;
		int64_t lCellScore = GetCellScore();
		int64_t lUserScore = m_pITableFrame->GetTableUserItem(i)->GetUserScore();
		
		//积分基数
		llUserBaseScore[i] = lUserScore;
        if (i==m_wBankerUser) continue;
        /*{
            lUserTimes=(m_cbHandCardCount[m_wBankerUser]==0)?1:-1;
            uint8_t stakes = 0;
            for (int j = 0; j < GAME_PLAYER; ++j)
            {
                if (j == m_wBankerUser)
                    continue;
                if (m_cbStakeInfo[j] == CB_ADD_DOUBLE)
                    stakes +=2;
                else
                    stakes +=1;
            }
            lUserTimes = lUserTimes * stakes;			
        }
        else*/
        {
            lUserTimes=(m_cbHandCardCount[m_wBankerUser]==0)?-1:1;
            //农民加倍倍数
            uint8_t stakeTimes = m_cbStakeInfo[i];
            if (stakeTimes == CB_ADD_DOUBLE)
                stakeTimes = 2;
            else
                stakeTimes = 1;

            tempStakeTimes += stakeTimes;
            lUserTimes = lUserTimes * stakeTimes;
        }
        lTempUserScpre[i] = lUserTimes*lCellScore*lScoreTimes;
        if(lTempUserScpre[i]<0)
        {
            if(lUserScore<=0)
            {
                lTempUserScpre[i] = 0;
            }
            else if(lUserScore + lTempUserScpre[i]<0)
            {
                lTempUserScpre[i] = -lUserScore;
            }
            openLog("===lTempUserScpre<0 lUserScore=%d TempUserScpre[%d]=%d",lUserScore,i ,lTempUserScpre[i]);
            lWinScore -= lTempUserScpre[i];

			iBankerTotalWinScore -= lTempUserScpre[i];
			iUserTotalLoseScore += lTempUserScpre[i];
        }
		else
		{
			if (lTempUserScpre[i] > lUserScore)
			{
				lTempUserScpre[i] = lUserScore;
			}
			iBankerTotalWinScore -= lTempUserScpre[i];
			iUserTotalWinScore += lTempUserScpre[i];
		}
    }

    openLog("===m_wBankerUser=%d,lWinScore=%d", m_wBankerUser, lTempUserScpre[m_wBankerUser]);
    //banker lose
    /*if(m_cbHandCardCount[m_wBankerUser]!=0 )
    {
		int64_t winscore = 0;
        uint8_t stakeTimesTmp = 0;
        for (WORD i = 0; i < GAME_PLAYER; i++)
        {
            if (i == m_wBankerUser)
                continue;

            uint8_t stakeTimes = m_cbStakeInfo[i];
            if (stakeTimes == CB_ADD_DOUBLE)
                stakeTimesTmp += 2;
            else
                stakeTimesTmp += 1;

            winscore += lTempUserScpre[i];
        }
        int64_t tmpscore = 0;
        if(abs(lTempUserScpre[m_wBankerUser] - winscore)>=0)
        {
            tmpscore = lWinScore/stakeTimesTmp;
        }
        for (WORD i = 0; i < GAME_PLAYER; i++)
        {
            if (i == m_wBankerUser)
                continue;

            uint8_t stakeTimes = m_cbStakeInfo[i];
            if (stakeTimes == CB_ADD_DOUBLE)
                lTempUserScpre[i] = tmpscore*2;
            else
                lTempUserScpre[i] = tmpscore;

            openLog("===玩家结算 lTempUserScpre[%d]=%d",i,lTempUserScpre[i]);
        }
    }*/
    //是否以小博大判断
	if (iBankerTotalWinScore > llBankerBaseScore)
	{
		int64_t bankerliss = 0;
		for (int i = 0; i < GAME_PLAYER; ++i)
		{
			if (i == m_wBankerUser)
				continue;
			//当前玩家真正要输的金币分摊一下
			double iUserNeedLostScore = llBankerBaseScore * lTempUserScpre[i] / iUserTotalLoseScore;
			if (iUserNeedLostScore > llUserBaseScore[i])
			{
				iUserNeedLostScore = llUserBaseScore[i];
				bankerliss -= (iUserNeedLostScore - llUserBaseScore[i]);///////////yufang wanjia bu gou
			}
			lTempUserScpre[i] = -iUserNeedLostScore;
		}

		//庄家赢的钱是庄家本金
		lTempUserScpre[m_wBankerUser] = llBankerBaseScore + bankerliss;
	}
	else if (iBankerTotalWinScore + llBankerBaseScore < 0)   //如果庄家的钱不够赔偿
	{
		//庄家能赔付的钱总额
		int64_t iBankerMaxScore = llBankerBaseScore;

		//闲赢 (庄家本金 + 庄赢的总金币)*当前闲家赢的金币 /（闲家总赢）
		int64_t bankerscore = 0.0;
		for (int i = 0; i < GAME_PLAYER; ++i)
		{
			if (i == m_wBankerUser)
				continue;

			//输钱不算
			if (lTempUserScpre[i] < 0)
				continue;

			double iUserRealWinScore = iBankerMaxScore * lTempUserScpre[i] / iUserTotalWinScore;
			if (iUserRealWinScore > llUserBaseScore[i])
			{
				iUserRealWinScore = llUserBaseScore[i];
				bankerscore += (iUserRealWinScore - llUserBaseScore[i]);
			}

			lTempUserScpre[i] = iUserRealWinScore;
		}

		//庄家赢的钱
		lTempUserScpre[m_wBankerUser] -= llBankerBaseScore;
		lTempUserScpre[m_wBankerUser] += bankerscore;
	}
	else
	{
		//其他情况，庄家输赢
		lTempUserScpre[m_wBankerUser] = iBankerTotalWinScore;
	}
	//统计积分
    ZeroMemory(&m_CalculateRevenue,sizeof(m_CalculateRevenue));
    ZeroMemory(&m_CalculateAgentRevenue,sizeof(m_CalculateAgentRevenue));
	int64_t lGameScore[GAME_PLAYER]={0};
    //积分变量
    vector<tagScoreInfo> scoreInfoVec;
	tagScoreInfo scoreInfo;
    openLog("===游戏结束 Game end...... openCount=%d",openCount);
    for (WORD i=0;i<GAME_PLAYER;i++)
    {        
		scoreInfo.clear();
        int64_t lUserScore = 0;
        scoreInfo.chairId = i;
        //// 地主赢
        //if (i==m_wBankerUser&&m_cbHandCardCount[m_wBankerUser]==0)
        //{
        //    lUserScore = lTempUserScpre[m_wBankerUser];
        //    //scoreInfo[i].cardValue = SCORE_TYPE_WIN;
        //}
        //// 农民赢
        //else if (i!=m_wBankerUser && m_cbHandCardCount[m_wBankerUser]!=0)
        //{
        //    if (tempStakeTimes == 0)
        //        tempStakeTimes = 1;
        //    lUserScore = lTempUserScpre[i];
        //    //scoreInfo[i].cardValue = SCORE_TYPE_WIN;
        //}
        //// 输家
        //else
        {
            lUserScore = lTempUserScpre[i];
            //scoreInfo[i].cardValue = SCORE_TYPE_LOST;
        }
		/*if (i != m_wBankerUser) {
			uint8_t cbHandCard[NORMAL_COUNT] = { 0 };
			CopyMemory(&cbHandCard, &m_cbHandCardAllData[i], sizeof(uint8_t)*NORMAL_COUNT);
			scoreInfo.cardValue = GlobalFunc::converToHex(cbHandCard, MAX_COUNT);
			m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore,GlobalFunc::converToHex(cbHandCard, NORMAL_COUNT), i == m_wBankerUser);
		}
		else {
			scoreInfo.cardValue = GlobalFunc::converToHex(m_cbHandCardAllData[i], MAX_COUNT);
			m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore,GlobalFunc::converToHex(m_cbHandCardAllData[i], MAX_COUNT), i == m_wBankerUser);
		}*/
		scoreInfo.cardValue = GlobalFunc::converToHex(m_cbHandCardAllData[i], MAX_COUNT);
        // 记录写分
        scoreInfo.addScore = lUserScore;
		scoreInfo.startTime = m_dwStartGameTime;
        scoreInfo.revenue = lUserScore > 0 ? m_pITableFrame->CalculateRevenue(lUserScore) : 0;
        if(lUserScore>0)
        {
            scoreInfo.addScore -= scoreInfo.revenue;
            m_CalculateRevenue[i] = scoreInfo.revenue;
            m_CalculateAgentRevenue[i] = scoreInfo.revenue;            
            GameEnd.add_blose(GAME_WIN);
            m_EndWinScore[i] = scoreInfo.addScore;
        }
        else
        {
            scoreInfo.betScore = 0;
            m_CalculateRevenue[i] = 0;
            GameEnd.add_blose(GAME_LOSE);
            // m_EndWinScore[i] = scoreInfo[i].nAddScore;
        }
        lGameScore[i] = lUserScore;
        m_iUserWinScore[i] = lUserScore - scoreInfo.revenue;
        m_HistoryScore.OnEventUserScore(i, m_iUserWinScore[i]);

		scoreInfo.rWinScore = abs(m_iUserWinScore[i]);
		scoreInfo.betScore = scoreInfo.rWinScore;

        GameEnd.add_dgamescore(m_iUserWinScore[i]);
		scoreInfoVec.push_back(scoreInfo);	
		
		m_replay.addResult(i, i, scoreInfo.betScore, scoreInfo.addScore, GlobalFunc::converToHex(m_cbHandCardAllData[i], MAX_COUNT), i == m_wBankerUser);
    }

    double winScore = 0;
    for(WORD i=0;i<GAME_PLAYER;i++)
    {
        //读取用户
        shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem( i ) ;

		if (pServerUserItem->IsAndroidUser() == true)
		{
			winScore += lGameScore[i];
		}
    }
	if (winScore > 0)
	{
        winScore = (winScore*(1000.0 - m_pITableFrame->GetGameRoomInfo()->systemReduceRatio) / 1000.0);
	}

    m_pITableFrame->UpdateStorageScore(winScore);

    //写入积分
	m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
	m_pITableFrame->SaveReplay(m_replay);

    string sdata = GameEnd.SerializeAsString();
    //发送数据
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        if(NULL == m_pITableFrame->GetTableUserItem(i))
            continue;

        m_pITableFrame->SendTableData(i,SUB_S_GAME_CONCLUDE,(uint8_t*)sdata.c_str(),sdata.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_GAME_CONCLUDE, (uint8_t*)sdata.c_str(),sdata.size());
        }

    }
    SendWinSysMessage();
    //切换用户
    m_wFirstUser=wChairID;
	/*for (int i = 0; i < GAME_PLAYER; ++i)
	{
		shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
		if (!user)
		{
			continue;
		}
		if (user->GetUserStatus() != sOffline)
		{
			user->SetUserStatus(sReady);
		}
		LOG(INFO) << "SET ALL PALYER  READY-------------------";
	}*/

    return true;
}
//用户强退
bool  CTableFrameSink::OnUserLeaveGameConclude(uint8_t cbReason, WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    return true;
}

//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairid, uint8_t nEndTag)
{
    WORD wChairID = chairid;
    uint8_t cbReason = nEndTag;
    shared_ptr<IServerUserItem>pIServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);

    if(pIServerUserItem==NULL)
    {
        return false;
    }

    m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);

    switch (cbReason)
    {
		case GER_NORMAL_:		//常规结束
		{
			OnNormalGameConclude(wChairID);
			break;
		}
		case GER_DISMISS_:		//游戏解散
		{
			OnDismissGameConclude();
			break;
		}
		case GER_USER_LEAVE:	//用户强退
		case GER_NETWORK_ERROR:	//网络中断
		{
			OnUserLeaveGameConclude(cbReason, wChairID, pIServerUserItem);
			break;
		}
    }

    //tmp......
    if(m_bReplayRecordStart)
    {
        RepayRecordEnd();
    }

    //结束游戏
	totalMatchSeconds_ = 0;
	m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    m_isflag =  m_pITableFrame->ConcludeGame(GAME_STATUS_END);
	m_pITableFrame->SetGameStatus(GAME_STATUS_END);
    m_cbGameStatus = GAME_SCENE_FREE;
    m_cbRemainTime = TIME_TICK_USER;
    m_dwTime = (DWORD)time(NULL);
    m_TimerId = m_TimerLoopThread->getLoop()->runAfter(TIME_TICK_USER,boost::bind(&CTableFrameSink::OnTimer, this));
    openLog("===游戏进入空闲状态 GAME_SCENE_FREE m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
    AutoWork();

    return true;
}

bool CTableFrameSink::OnSceneFree(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    CMD_S_StatusFree StatusFree;

    StatusFree.set_dcellscore( m_lCellScore);
    StatusFree.set_dwcurrplaycount( m_pITableFrame->GetPlayerCount(true));

    //发送数据
    std::string data = StatusFree.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SC_GAMESCENE_FREE, (uint8_t*)data.c_str(), data.size());

    return true;
}

bool CTableFrameSink::OnSceneCall(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    //构造数据
    CMD_S_StatusCall StatusCall;
    //memset(&StatusCall,0, sizeof(StatusCall));

    //单元积分
    StatusCall.set_dcellscore( m_lCellScore);

    //剩余时间
    DWORD dwCallTime = (DWORD)time(NULL) - m_dwTime;
    DWORD cbTimeLeave = dwCallTime >= m_cbRemainTime ? 0 : m_cbRemainTime-dwCallTime;		//剩余时间
    StatusCall.set_cblefttime(cbTimeLeave);
    //游戏信息
    StatusCall.set_wcurrentuser( m_wCurrentUser);

    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //叫庄标志(-1:未叫, 0:不叫, 1:叫庄)
        StatusCall.add_cbcallbankerinfo(m_cbScoreInfo[i]);
        //托管状态
        StatusCall.add_cbtrustee(m_bTrustee[i]);
    }

    //手上扑克
    for(int i =0;i<m_cbHandCardCount[wChairID];i++)
        StatusCall.add_cbhandcarddata(m_cbHandCardData[wChairID][i]);

	StatusCall.set_cbroundid(m_strRoundId);
    //发送场景
    std::string data = StatusCall.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SC_GAMESCENE_CALL, (uint8_t*)data.c_str(), data.size());

    return true;
}

bool CTableFrameSink::OnSceneStake(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    CMD_S_StatusDouble StatusDouble;
    //memset(&StatusDouble, 0, sizeof(StatusDouble));

    //单元积分
    StatusDouble.set_dcellscore( m_lCellScore);

    StatusDouble.set_cbcallscore( m_cbBankerScore);
    DWORD currtime=1;
    if(wChairID == m_wBankerUser)
    {
        currtime = 2;
        for(int i = 0; i < GAME_PLAYER; i++)
        {
            if(i == m_wBankerUser)
                continue;
            if(m_cbStakeInfo[i] == CB_ADD_DOUBLE)
                currtime++;
        }
    }
    else
    {
        if(m_cbStakeInfo[wChairID] == CB_ADD_DOUBLE)
            currtime++;
    }
    StatusDouble.set_wcurrtime(currtime);
    //剩余时间
    DWORD dwCallTime = (DWORD)time(NULL) - m_dwTime;
    DWORD cbTimeLeave = dwCallTime >= m_cbRemainTime ? 0 : m_cbRemainTime-dwCallTime;		//剩余时间
    StatusDouble.set_cblefttime(cbTimeLeave);
    StatusDouble.set_wbankeruser(m_wBankerUser);
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //托管状态
        StatusDouble.add_cbtrustee(m_bTrustee[i]);
        StatusDouble.add_cbhandcardcount(m_cbHandCardCount[i]);
        StatusDouble.add_cbdoubleinfo(m_cbStakeInfo[i]);
    }

    //手上扑克
    for(int i =0;i<m_cbHandCardCount[wChairID];i++)
        StatusDouble.add_cbhandcarddata(m_cbHandCardData[wChairID][i]);

    //StatusDouble.set_wcurrtime(  m_wEachTotalTime[wChairID]);

    //扑克信息
    for(int i =0;i<3;i++)
        StatusDouble.add_cbbankercard(m_cbBankerCard[i]);

	StatusDouble.set_cbroundid(m_strRoundId);
    //发送场景
    std::string data = StatusDouble.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SC_GAMESCENE_DOUBLE, (uint8_t*)data.c_str(), data.size());

    return true;
}

bool CTableFrameSink::OnScenePlay(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    //构造数据
    CMD_S_StatusPlay StatusPlay;
    //memset(&StatusPlay, 0, sizeof(StatusPlay));

    //单元积分
    StatusPlay.set_dcellscore( m_lCellScore);

    //剩余时间
    DWORD dwCallTime = (DWORD)time(NULL) - m_dwTime;
    DWORD cbTimeLeave = dwCallTime >= m_cbRemainTime ? 0 : m_cbRemainTime-dwCallTime;		//剩余时间
    StatusPlay.set_cblefttime(cbTimeLeave);
    //出牌信息0
    WORD	turnwiner = (m_wCurrentUser-1)%GAME_PLAYER;
    StatusPlay.set_wturnwiner(turnwiner);
    StatusPlay.set_cbturncardcount(m_cbTurnCardCount);
    for(int i =0;i<m_cbTurnCardCount;i++)
        StatusPlay.add_cbturncarddata(m_cbTurnCardData[i]);

    //游戏变量
    StatusPlay.set_dwbankeruser( m_wBankerUser);
    StatusPlay.set_dwcurrentuser(m_wCurrentUser);
    StatusPlay.set_cbcallscore( m_cbBankerScore);
    DWORD userTime = 1;
    if (wChairID == m_wBankerUser)
    {
        for (WORD j = 0; j<GAME_PLAYER; j++)
        {
            if (j == m_wBankerUser)
            {
                userTime++;
                continue;
            }
            if(m_cbStakeInfo[j] == CB_ADD_DOUBLE)
                userTime++;
        }
        for (uint8_t i=0;i<m_cbBombCount;i++)
            userTime*=2;
    }
    else
    {
        if(m_cbStakeInfo[wChairID] == CB_ADD_DOUBLE)
            userTime++;

        for (uint8_t i=0;i<m_cbBombCount;i++)
            userTime*=2;
    }

    StatusPlay.set_dwbombtime(userTime);
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        //托管状态
        StatusPlay.add_cbtrustee(m_bTrustee[i]);
        StatusPlay.add_cbhandcardcount(m_cbHandCardCount[i]);
        StatusPlay.add_cbadddoubleinfo(m_cbStakeInfo[i]);
        StatusPlay.add_cboutcount(m_cbOutCardCount[i]);

        for(int j=0;j<m_cbOutCardCount[i];j++)
        {
            StatusPlay.add_cboutcardlistcount(m_cbOutCardListCount[i][j]);
            for(int k=0;k<m_cbOutCardListCount[i][j];k++)
                StatusPlay.add_cboutcardlist(m_cbOutCardList[i][j][k]);
        }
    }

    //手上扑克
    for(int i =0;i<m_cbHandCardCount[wChairID];i++)
        StatusPlay.add_cbhandcarddata(m_cbHandCardData[wChairID][i]);
    for(int i =0;i<3;i++)
        StatusPlay.add_cbbankercard(m_cbBankerCard[i]);

	StatusPlay.set_cbroundid(m_strRoundId);
    //发送场景
    std::string data = StatusPlay.SerializeAsString();
    m_pITableFrame->SendTableData(wChairID, SC_GAMESCENE_PLAY, (uint8_t*)data.c_str(), data.size());

    return true;
}

bool CTableFrameSink::RepayRecordGameScene(DWORD chairid, bool bisLookon)
{
    shared_ptr<IServerUserItem> pIServerUserItem=m_pITableFrame->GetTableUserItem(chairid);

    if(pIServerUserItem==NULL)
    {
        return false;
    }
    WORD wChairID = pIServerUserItem->GetChairId();
    switch (m_cbGameStatus)
    {
		case GAME_SCENE_FREE:	//空闲状态
		{
			return true;
		}
		case GAME_SCENE_CALL:	//叫分状态
		{
			return true;
		}
		case GAME_SCENE_STAKE:		//加倍状态
		{
			return true;
		}
		case GAME_SCENE_PLAY:	//游戏状态
		{
			return true;
		}
		default: 
			return true;
    }
}

//发送场景
//bool CTableFrameSink::OnEventSendGameScene(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, uint8_t cbGameStatus, bool bSendSecret)
bool CTableFrameSink::OnEventGameScene(uint32_t chairid, bool bisLookon)
{
    shared_ptr<IServerUserItem> pIServerUserItem=m_pITableFrame->GetTableUserItem(chairid);

    if(pIServerUserItem==NULL)
    {
        return false;
    }
	m_bhaveCancelTrustee = false;
    WORD wChairID = pIServerUserItem->GetChairId();
	
    switch (m_cbGameStatus)
    {
		case GAME_SCENE_FREE:	//空闲状态
		{
			return OnSceneFree(wChairID, pIServerUserItem);
		}
		case GAME_SCENE_CALL:	//叫分状态
		{
			return OnSceneCall(wChairID, pIServerUserItem);
		}
		case GAME_SCENE_STAKE:		//加倍状态
		{
			return OnSceneStake(wChairID, pIServerUserItem);
		}
		case GAME_SCENE_PLAY:	//游戏状态
		{
			return OnScenePlay(wChairID, pIServerUserItem);
		}
    }

    //错误断言
    //ASSERT(FALSE);

    return false;
}

//这个鸡巴函数就是定时器设定的时间到了，一般像叫分这种不叫那就默认值了，或者出牌，就是托管出牌了，等等
void CTableFrameSink::OnTimer()
{
	openLog("===OnTimer m_cbGameStatus=%d,m_wCurrentUser=%d; ", m_cbGameStatus, m_wCurrentUser);
    switch (m_cbGameStatus)
    {
		case GAME_SCENE_FREE:
		{
			//openLog("===cancel timecount=%d", m_cbRemainTime--);
			m_TimerLoopThread->getLoop()->cancel(m_TimerId);
			m_pITableFrame->SetGameStatus(GAME_STATUS_FREE);
			openLog("===OnTimer() SetGameStatus=%d", GAME_STATUS_FREE);
			if (!m_isflag)
			{
				for (int i = 0; i < GAME_PLAYER; i++)
				{
					shared_ptr<CServerUserItem> user = m_pITableFrame->GetTableUserItem(i);
					if (user)
					{
						openLog("3ConcludeGame=false  ClearTableUser(i,true,true) user=%d   and chairID=%d", user->GetUserId(), user->GetChairId());
						user->SetUserStatus(sOffline);
						m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SERVERSTOP);
					}
				}
				break;
			}
            int readyUserCount = 0;
			for (int i = 0; i < GAME_PLAYER; ++i)
			{
				if (!m_pITableFrame->IsExistUser(i))
				{
					continue;
				}
				shared_ptr<IServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (pServerUserItem == nullptr)
					continue;

				openLog("===UserID=%d GAME_SCENE_FREE GetUserStatus=%d", pServerUserItem->GetUserId(), pServerUserItem->GetUserStatus());

				if (pServerUserItem->GetUserStatus() == sOffline)
				{
					DWORD ChairID = pServerUserItem->GetChairId();
					bool isAndroid = pServerUserItem->IsAndroidUser();
					openLog("===sOffline ClearTableUser userid=%d ", pServerUserItem->GetUserId());
					m_pITableFrame->ClearTableUser(i, true, true);
					openLog("===sOffline ClearTableUser End userid=%d ", pServerUserItem->GetUserId());
					continue;
				}

				//if (0 == m_cbRemainTime /*&& pServerUserItem->GetUserStatus() == sSit*/)
				{
					if (pServerUserItem == nullptr)
						continue;
					DWORD ChairID = pServerUserItem->GetChairId();
					pServerUserItem->SetUserStatus(sGetout);
					m_pITableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
					openLog("===OnTimer() 长时间不操作踢出房间 ChairId=%d,UserId=%d", i, pServerUserItem->GetUserId());
					bool isAndroid = pServerUserItem->IsAndroidUser();
					if (!isAndroid)
					{
						CleanUser();
					}
					continue;
				}
				/*
				if (m_cbRemainTime < 10 && pServerUserItem->IsAndroidUser() && rand() % 100 < 30)
				{
					openLog("=== IsAndroidUser ClearTableUser");
					m_pITableFrame->ClearTableUser(i, true, true);
					openLog("=== IsAndroidUser ClearTableUser end");
					continue;
				}

                if (0 == m_cbRemainTime && pServerUserItem->GetUserStatus() == sPlaying)
                {
                    readyUserCount++;
                }*/
			}
            /*if (readyUserCount==3)
            {
                if(m_pITableFrame->GetPlayerCount(true) == GAME_PLAYER && m_pITableFrame->GetGameStatus() < GAME_STATUS_START)
                {       
                    OnGameStart();
                }
            }*/
			/*if (m_cbRemainTime > 0)
				m_TimerId = m_TimerLoopThread->getLoop()->runAfter(1.0, boost::bind(&CTableFrameSink::OnTimer, this));*/
			break;
		}
		case GAME_SCENE_CALLEnd:
		{
			openLog("===叫分结束 GAME_SCENE_CALLEnd");
			m_cbGameStatus = GAME_SCENE_STAKE;
			CMD_S_BankerInfo BankerInfo;
			BankerInfo.set_dwbankeruser(m_wBankerUser);
			BankerInfo.set_dwcurrentuser(m_wCurrentUser);
			BankerInfo.set_cbtimeleft(TIME_ADD_DOUBLE);
			for (int i = 0;i < 3;i++)
				BankerInfo.add_cbbankercard(m_cbBankerCard[i]);

			BankerInfo.set_cbcallscore(m_cbBankerScore);

			std::string BankerInfodata = BankerInfo.SerializeAsString();

			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_BANKER_INFO, (uint8_t*)BankerInfodata.c_str(), BankerInfodata.size());

			//tmp......
			if (m_bReplayRecordStart)
			{
				RepayRecord(INVALID_CHAIR, SUB_S_BANKER_INFO, (uint8_t*)BankerInfodata.c_str(), BankerInfodata.size());
			}

			m_cbRemainTime = TIME_ADD_DOUBLE;
			m_dwTime = (DWORD)time(NULL);
            m_TimerLoopThread->getLoop()->cancel(m_TimerId);
            m_TimerLoopThread->getLoop()->cancel(m_TimerCallBankerId);
            m_TimerLoopThread->getLoop()->cancel(m_TimerCallStakeId);
			m_TimerCallStakeId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime, boost::bind(&CTableFrameSink::OnCallStakeTimer, this));
			openLog("===开始加倍 TIME_ADD_DOUBLE m_cbRemainTime=%d timecount=%d", static_cast<int>(m_cbRemainTime), ++timeCount);
			AutoWork();
			break;
		}
		case GAME_SCENE_STAKEEnd:
		{
			openLog("===加倍结束 GAME_SCENE_STAKEEnd");
			m_cbGameStatus = GAME_SCENE_PLAY;
			CMD_S_StartOutCard startOutCard;
			startOutCard.set_dwbankeruser(m_wBankerUser);
			startOutCard.set_dwcurrentuser(m_wCurrentUser);
			startOutCard.set_cblefttime(TIME_FIRST_OUT_CARD);

			std::string data = startOutCard.SerializeAsString();
			m_pITableFrame->SendTableData(INVALID_CHAIR, SUB_S_OUT_START_START, (uint8_t*)data.c_str(), data.size());

			//tmp......
			if (m_bReplayRecordStart)
			{
				RepayRecord(INVALID_CHAIR, SUB_S_OUT_START_START, (uint8_t*)data.c_str(), data.size());
			}

			if (m_bTrustee[m_wCurrentUser])
				m_cbRemainTime = TIME_OUT_CARDTrust;
			else
				m_cbRemainTime = TIME_FIRST_OUT_CARD;
			m_dwTime = (DWORD)time(NULL);
            m_TimerLoopThread->getLoop()->cancel(m_TimerId);
            m_TimerLoopThread->getLoop()->cancel(m_TimerCallStakeId);
            m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
			m_TimerTrustId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime, boost::bind(&CTableFrameSink::OnUserTrustTimer, this));
			openLog("===开始出牌 m_wCurrentUser=%d TIME_FIRST_OUT_CARD m_cbRemainTime=%d timecount=%d", m_wCurrentUser, static_cast<int>(m_cbRemainTime), ++timeCount);
			AutoWork();
			break;
		}
		default:
			//ASSERT(FALSE);
			break;
    }
}
//自动叫分定时器
void CTableFrameSink::OnCallBankerTimer()
{
    openLog("===CallBankerTimer m_cbGameStatus=%d,m_wCurrentUser=%d; ", m_cbGameStatus, m_wCurrentUser);
    if (m_cbGameStatus == GAME_SCENE_CALL)
    {
        openLog("===自动叫分 GAME_SCENE_CALL m_wCurrentUser=%d", m_wCurrentUser);
        OnAutoCallScore(m_wCurrentUser);
    }
}
//自动加倍选择定时器
void CTableFrameSink::OnCallStakeTimer()
{
    openLog("===CallStakeTimer m_cbGameStatus=%d,m_wCurrentUser=%d; ", m_cbGameStatus, m_wCurrentUser);
    if (m_cbGameStatus == GAME_SCENE_STAKE)
    {
        for (int i = 0; i < GAME_PLAYER; ++i)
        {
            if (m_wBankerUser != i && m_cbStakeInfo[i] <= 0)
            {
                OnAutoStakeScore(i);
                openLog("===自动选择加倍 GAME_SCENE_STAKE auto id=%d", i);
            }
        }
    }
}
//系统托管出牌定时器
void CTableFrameSink::OnUserTrustTimer()
{
    openLog("===TrustTimer m_cbGameStatus=%d,m_wCurrentUser=%d; ", m_cbGameStatus, m_wCurrentUser);
    if (m_cbGameStatus == GAME_SCENE_PLAY)
    {
        openLog("===玩家托管自动出牌 GAME_SCENE_PLAY m_wCurrentUser=%d,cancel timecount=%d", m_wCurrentUser, timeCount);
        openLog("===cancel(m_TimerTrustId) SUB_C_TRUSTEE ");
        OnUserTrust(m_wCurrentUser, true);
        m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
        OnAutoOutCard(m_wCurrentUser);
    }
}

void CTableFrameSink::AutoWork()
{
	openLog("===AutoWork m_cbGameStatus=%d,m_wCurrentUser=%d;", m_cbGameStatus, m_wCurrentUser);
    switch (m_cbGameStatus)
    {
		case GAME_SCENE_FREE:
		{
			openLog("===AutoWork GAME_SCENE_FREE ===");
			break;
		}
		case GAME_SCENE_CALL:
		{
			//进入叫庄状态后续不用再给玩家发什么数据
			if (IsOfflineTrustee(m_wCurrentUser))
			{
				OnAutoCallScore(m_wCurrentUser);
			}
			break;
		}
		case GAME_SCENE_STAKE:
		{
			for (int i = 0; i < GAME_PLAYER; ++i)
			{
				if (IsOfflineTrustee(i) && m_cbStakeInfo[i] <= 0 && m_wBankerUser != i)
				{
					OnAutoStakeScore(i);
				}
			}
			break;
		}
		case GAME_SCENE_PLAY:
		{
			openLog("===GAME_SCENE_PLAY m_wCurrentUser=%d", m_wCurrentUser);
			shared_ptr<CServerUserItem> pServerUserItem = m_pITableFrame->GetTableUserItem(m_wCurrentUser) ;
			if(pServerUserItem==nullptr)
				break;
			if (m_pITableFrame->GetTableUserItem(m_wCurrentUser)->GetUserStatus() == sOffline)
			{
				openLog("===GetUserStatus sOffline m_wCurrentUser=%d",m_wCurrentUser);
				OnUserTrust(m_wCurrentUser, true);
				OnAutoOutCard(m_wCurrentUser);
			}
			else if (IsCanOutAllCard(m_wCurrentUser))
			{
				if(m_pITableFrame->GetTableUserItem(m_wCurrentUser)->IsAndroidUser() && m_bTrustee[m_wCurrentUser])
				{
					if(rand()%100 <20)
						OnUserTrust(m_wCurrentUser, false);
				}
				openLog("===IsCanOutAllCard2 m_wCurrentUser=%d",m_wCurrentUser);
				OnAutoOutCard(m_wCurrentUser);
			}
			break;
		}
		default:
			//ASSERT(FALSE);
			break;
    }
}

//数据事件
bool CTableFrameSink::OnDataBaseMessage(WORD wRequestID, VOID * pData, WORD wDataSize)
{
    return false;
}

//积分事件
bool CTableFrameSink::OnUserScroeNotify(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, uint8_t cbReason)
{
    return true;
}

//游戏消息
bool CTableFrameSink::OnGameMessage( DWORD chairid, uint8_t subid, const uint8_t* data, uint32_t datasize)
{
    shared_ptr<IServerUserItem>pIServerUserItem=m_pITableFrame->GetTableUserItem(chairid);
    if(pIServerUserItem == nullptr)
    {
        openLog("OnGameMessage pIServerUserItem == nullptr");
        return true;
    }
	openLog("====OnGameMessage  chairid=%d  subid=%d ", chairid, subid);
    switch (subid)
    {
		case SUB_C_CALL_BANKER:			//用户叫分
		{
			WORD wChairID=pIServerUserItem->GetChairId();
			if (wChairID == INVALID_CHAIR)
				return false;

			//用户效验
			//ASSERT(pIServerUserItem->GetUserStatus() == sPlaying);
			if (pIServerUserItem->GetUserStatus() != sPlaying)
			{
				if(!pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()!=sOffline)
					return true;
				if(pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()==sOffline)
					pIServerUserItem->SetUserStatus(sPlaying);
			}
			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus() == GAME_SCENE_CALL);
			if (m_cbGameStatus != GAME_SCENE_CALL)
				return true;
			m_bTrustee[wChairID] = false;   //这个是是够要托管,估计到时候是要去掉的

			ddz::CMD_C_CallBanker pCallScore;
			pCallScore.ParseFromArray(data,datasize);

			return OnUserCallScore(wChairID, pCallScore.cbcallinfo());//

		}
		case SUB_C_ADD_DOUBLE:			 //用户加倍
		{
			WORD wChairID = pIServerUserItem->GetChairId();
			if (wChairID == INVALID_CHAIR)
				return false;
			if (pIServerUserItem->GetUserStatus() != sPlaying)
			{
				if(!pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()!=sOffline)
					return true;
				if(pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()==sOffline)
					pIServerUserItem->SetUserStatus(sPlaying);
			}
			//ASSERT(wChairID !=m_wBankerUser);
			if (wChairID == m_wBankerUser)
				return true;

			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus() == GAME_SCENE_STAKE);
			if (m_cbGameStatus != GAME_SCENE_STAKE)
				return true;
			m_bTrustee[wChairID] = false;

			CMD_C_Double  wDouble;
			wDouble.ParseFromArray(data,datasize);

			//消息处理
			return OnUserStakeScore(wChairID,wDouble.cbdoubleinfo());

		}
		case SUB_C_OUT_CARD:		   //用户出牌
		{
			WORD wChairID=pIServerUserItem->GetChairId();
			if (pIServerUserItem->GetUserStatus() != sPlaying)
			{
				if(!pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()!=sOffline)
					return true;
				if(pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()==sOffline)
					pIServerUserItem->SetUserStatus(sPlaying);
			}

			if (m_cbGameStatus != GAME_SCENE_PLAY)
				return true;

			//ASSERT(wChairID != INVALID_CHAIR);
			if (wChairID == INVALID_CHAIR)
				return false;
			m_bTrustee[wChairID] = false;
			CMD_C_OutCard  OutCard;
			OutCard.ParseFromArray(data,datasize);

			uint8_t cbCardData[MAX_COUNT] = {0};
			const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >& CardData= OutCard.cbcarddata();
			for(int i=0;i<CardData.size();i++)
				cbCardData[i] = CardData[i];

			char tmp[100] = { 0 };
			GetValue(cbCardData, CardData.size(), tmp);
			openLog("===OnGameMessage SUB_C_OUT_CARD wChairID=%d %s", wChairID, tmp);

			//消息处理
			return OnUserOutCard(wChairID, cbCardData, OutCard.cbcardcount());

		}
		case SUB_C_PASS_CARD:	    //用户不出牌
		{
			//状态效验
			//ASSERT(m_pITableFrame->GetGameStatus()==GAME_SCENE_PLAY);
			if (m_cbGameStatus !=GAME_SCENE_PLAY) return true;

			//用户效验
			//ASSERT(pIServerUserItem->GetUserStatus()==sPlaying);
			if (pIServerUserItem->GetUserStatus()!=sPlaying)
			{
				if(!pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()!=sOffline)
					return true;
				if(pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()==sOffline)
					pIServerUserItem->SetUserStatus(sPlaying);
			}
			WORD wChairID = pIServerUserItem->GetChairId();
			//ASSERT(wChairID != INVALID_CHAIR);
			if (wChairID == INVALID_CHAIR) return false;

			m_bTrustee[wChairID] = false;

			return OnUserPassCard(wChairID);
		}
		case SUB_C_TRUSTEE:			//用户委托
		{
			WORD wChairID = pIServerUserItem->GetChairId();
			//ASSERT(wChairID != INVALID_CHAIR);
			if (wChairID == INVALID_CHAIR) return false;
			if (m_cbGameStatus == GAME_SCENE_FREE)
			{
				if(!pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()!=sOffline)
					return true;
				if(pIServerUserItem->IsAndroidUser() && pIServerUserItem->GetUserStatus()==sOffline)
					pIServerUserItem->SetUserStatus(sPlaying);
			}

			CMD_C_Trustee  pTrustee;
			pTrustee.ParseFromArray(data,datasize);

			OnUserTrust(wChairID, true);

			if( wChairID == m_wCurrentUser && m_cbGameStatus == GAME_SCENE_PLAY)
			{
				m_bhaveCancelTrustee = false;
				openLog("===cancel(m_TimerTrustId) SUB_C_TRUSTEE ");
				m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
				OnAutoOutCard(m_wCurrentUser);
			}

			return true;
		}
		case SUB_C_CANCEL_TRUSTEE: //取消托管
		{
			//变量定义
			CMD_C_CancelTrustee  pCancelTrustee;
			pCancelTrustee.ParseFromArray(data,datasize);

			WORD wChairID = pCancelTrustee.wchairid();
			if (wChairID == INVALID_CHAIR)
				return true;

			if (!m_bTrustee[wChairID] || m_bhaveCancelTrustee)
			{
				openLog("===玩家不在托管状态或者已经取消过托管了 wChairID=%d ", wChairID);
				return true;
			}
			OnUserTrust(wChairID,false);
			
			if(wChairID == m_wCurrentUser && m_cbGameStatus == GAME_SCENE_PLAY)
			{
				m_bhaveCancelTrustee = true;
				m_cbRemainTime = TIME_OUT_CARD - TIME_OUT_CARDTrust;
				m_dwTime = (DWORD)time(NULL);
				m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
				m_TimerTrustId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime,boost::bind(&CTableFrameSink::OnUserTrustTimer, this));
				openLog("===SUB_C_CANCEL_TRUSTEE m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
				AutoWork();
			}

			return true;
		}
		case SUB_C_CHEAT_LOOK_CARD:
		{
			// if (dwDataSize != sizeof(CMD_C_CheatLookCard)) return false;
			WORD wChairID = pIServerUserItem->GetChairId();
			bool bAndroid = m_pITableFrame->GetTableUserItem(wChairID)->IsAndroidUser();
			if (!bAndroid)
			{
				return false;
			}
			//变量定义
			CMD_C_CheatLookCard  pCheatLookCard;
			pCheatLookCard.ParseFromArray(data,datasize);
			if (pCheatLookCard.wbecheatchairid() >= GAME_PLAYER)
			{
				return true;
			}
			CMD_S_CheatLookCard cheatCardData;
			cheatCardData.set_wcarduser(  pCheatLookCard.wbecheatchairid());
			cheatCardData.set_cbcardcount(  m_cbHandCardCount[pCheatLookCard.wbecheatchairid()]);
			for(int i=0;i< m_cbHandCardCount[pCheatLookCard.wbecheatchairid()];i++)
				cheatCardData.add_cbcarddata(m_cbHandCardData[pCheatLookCard.wbecheatchairid()][i]);

			std::string data = cheatCardData.SerializeAsString();
			m_pITableFrame->SendTableData(wChairID, SUB_S_CHEAT_LOOK_CARD, (uint8_t*)data.c_str(), data.size());

			if(m_bReplayRecordStart)
			{
				RepayRecord(wChairID,SUB_S_CHEAT_LOOK_CARD, (uint8_t*)data.c_str(),data.size());
			}
			return true;
		}
		case SUB_C_GAMESCENE:
		{
			OnEventGameScene(pIServerUserItem->GetChairId(),false);
		}
    }

    return false;
}

//框架消息
bool CTableFrameSink::OnFrameMessage(DWORD chairid, uint8_t subid, const uint8_t* pData, DWORD wDataSize)
{
    return false;
}

//用户坐下
bool CTableFrameSink::OnActionUserSitDown(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, bool bLookonUser)
{
    //历史积分
    if (bLookonUser==false)
    {
        ASSERT(wChairID!=INVALID_CHAIR);
        m_HistoryScore.OnEventUserEnter(wChairID);
    }

    return true;
}

//用户起立
bool CTableFrameSink::OnActionUserStandUp(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem, bool bLookonUser)
{
    //历史积分
    if (bLookonUser==false)
    {
        ASSERT(wChairID!=INVALID_CHAIR);
        m_HistoryScore.OnEventUserLeave(wChairID);
    }
    return true;
}

bool CTableFrameSink::OnUserEnter(int64_t userid, bool islookon)
{
    m_pUserItem=m_pITableFrame->GetUserItemByUserId(userid);
    if(!m_pUserItem)
    {
        return false;
    }
    if(m_pUserItem->GetChairId()== INVALID_CHAIR)
    {        
        return false;
    }
    GSUserBaseInfo& userInfo = m_pUserItem->GetUserBaseInfo();
	openLog("===OnUserEnter userid=%d,ChairId=%d,account=%s;", userid, m_pUserItem->GetChairId(), userInfo.account.c_str());
    if (m_pITableFrame->GetPlayerCount(true) == 1) {
        ////////////////////////////////////////
        //第一个进入房间的也必须是真人
        //////////////////测试机器人玩注释//////
        assert(m_pITableFrame->GetPlayerCount(false) == 1);
        //重置累计时间
        totalMatchSeconds_ = 0;
    }

    //非游戏进行中，修改玩家准备
    if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START) {
        m_pUserItem->SetUserStatus(sReady);
    }
    //游戏进行中，修改玩家游戏中
    else {
        m_pUserItem->SetUserStatus(sPlaying);
    }
	
    //进入房间广播通知到其他玩家
    m_pITableFrame->BroadcastUserInfoToOther(m_pUserItem);
    m_pITableFrame->BroadcastUserStatus(m_pUserItem,true);
    m_pITableFrame->SendAllOtherUserInfoToUser(m_pUserItem);

	//发场景消息
	if (m_cbGameStatus != GAME_STATUS_INIT)
	{
		OnEventGameScene(m_pUserItem->GetChairId(), false);
	}
    if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START)
    {
        if(m_pITableFrame->GetPlayerCount(true) == GAME_PLAYER)
        {       
            OnGameStart();
        }
        else
        {
            if (m_pITableFrame->GetPlayerCount(true) == 1) {
                ////////////////////////////////////////
                //第一个进入房间的也必须是真人
                //////////////////测试机器人玩注释//////////////////////
                assert(m_pITableFrame->GetPlayerCount(false) == 1);

                m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
            }
        } 
    }
    
    //if(m_bReplayRecordStart)
    //{
    //    RepayRecorduserinfo(m_pUserItem->GetChairId());
    //}
    return OnActionUserSitDown( m_pUserItem->GetChairId(),m_pUserItem,islookon);
}

bool CTableFrameSink::OnUserReady(int64_t userid, bool islookon)
{
    m_pUserItem=m_pITableFrame->GetUserItemByUserId(userid);
    if(!m_pUserItem)
    {
        openLog("===OnUserReady m_pUserItem!");
        return false;
    }

    return OnActionUserOnReady( m_pUserItem->GetChairId(),m_pUserItem,NULL,0);
}

bool CTableFrameSink::OnUserLeft( int64_t userid, bool islookon)
{
    if (!CanLeftTable(userid))
	{
		return false;
	}
    m_pUserItem=m_pITableFrame->GetUserItemByUserId(userid);
    if(!m_pUserItem)
    {
		LOG(ERROR) << "---------------------------player == NULL---------------------------";
        return false;
    }
    DWORD ChairID = m_pUserItem->GetChairId();
    bool isAndroid = m_pUserItem->IsAndroidUser();

    if(m_pITableFrame->GetGameStatus()!=GAME_STATUS_START)
    {
        openLog("===userid=%d OnUserLeft ClearTableUser...", userid);
        //用户空闲状态
        m_pUserItem->SetUserStatus(sGetout);
        //玩家离开广播
        m_pITableFrame->BroadcastUserStatus(m_pUserItem, true);
        //清除用户信息
        m_pITableFrame->ClearTableUser(ChairID, true, true);
        //ret = true;
    }

    if(!isAndroid)
        return OnActionUserStandUp( ChairID,nullptr,islookon);
    return true;
}
bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(userId);
    if (!userItem) {
        return true;
    }
    if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START ||
        m_pITableFrame->GetGameStatus() == GAME_STATUS_END)
    {
        return true;
    }
    return false;
}

void CTableFrameSink::OnTimerGameReadyOver()
{
    //assert(m_wGameStatus < GAME_STATUS_START);
    ////////////////////////////////////////
    //销毁当前旧定时器
    m_TimerLoopThread->getLoop()->cancel(m_TimerIdReadyOver);
    if (m_pITableFrame->GetGameStatus() >= GAME_STATUS_START) {
        return;
    }
    ////////////////////////////////////////
    //计算累计匹配时间，当totalMatchSeconds_ >= timeoutMatchSeconds_时，
    //如果桌子游戏人数不足会自动开启 CanJoinTable 放行机器人进入桌子补齐人数
    totalMatchSeconds_ += sliceMatchSeconds_;
    ////////////////////////////////////////
    //达到游戏人数要求开始游戏
    if (m_pITableFrame->GetPlayerCount(true) >= GAME_PLAYER) {
        assert(m_pITableFrame->GetPlayerCount(true) == GAME_PLAYER);
    }
    else {
        ////////////////////////////////////////
        //桌子游戏人数不足，且没有匹配超时，再次启动定时器
        if (totalMatchSeconds_ < timeoutMatchSeconds_) {
            m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
        }
        else if (totalMatchSeconds_ >= timeoutMatchSeconds_ + timeoutAddAndroidSeconds_) {
            ////////////////////////////////////////
            //桌子游戏人数不足，且机器人候补空位超时
            if (m_pITableFrame->GetPlayerCount(true) >= GAME_PLAYER) {
                ////////////////////////////////////////
                //达到最小游戏人数，开始游戏
                openLog("--- 桌子游戏人数不足，且机器人候补空位超时 当前游戏人数(%d)，开始游戏......\n", m_pITableFrame->GetPlayerCount(true));
                OnGameStart();
            }
            else {
                ////////////////////////////////////////
                //仍然没有达到最小游戏人数，继续等待
                m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
            }
        }
        else if (totalMatchSeconds_ >= timeoutMatchSeconds_) {
            ////////////////////////////////////////
            //匹配超时，桌子游戏人数不足，CanJoinTable 放行机器人进入桌子补齐人数
            // printf("--- *** --------- 匹配超时 %.1fs，桌子游戏人数不足(%d)人，CanJoinTable 放行机器人进入桌子补齐人数\n", totalMatchSeconds_, MaxGamePlayers_);
            ////////////////////////////////////////
            //定时器检测机器人候补空位后是否达到游戏要求人数
            m_TimerIdReadyOver = m_TimerLoopThread->getLoop()->runAfter(sliceMatchSeconds_, boost::bind(&CTableFrameSink::OnTimerGameReadyOver, this));
        }
    }
}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
	//////////////////测试机器人玩注释//////////////////////
	// if (m_pITableFrame->GetPlayerCount(true) == 0) {
	//     ////////////////////////////////////////
	//     //第一个进入房间的也必须是真人
	//     //assert(m_pITableFrame->GetPlayerCount(false) == 1);
	//     //重置累计时间
	//     totalMatchSeconds_ = 4;
	// }
	/////////////////////////////////////////////////////////
    char msg[1024] = { 0 };
	/*if (!pUser->IsAndroidUser())
	{
		openLog("===当前桌子 TableId=%d 状态 GameStatus=%d", m_pITableFrame->GetTableId(), m_pITableFrame->GetGameStatus());
	}*/	
    //初始化或空闲准备阶段，可以进入桌子
	if (m_pITableFrame->GetGameStatus() == GAME_STATUS_END) {
		return false;
	}
    //if (m_pITableFrame->GetGameStatus() < GAME_STATUS_START) {
	if (m_cbGameStatus < GAME_SCENE_CALL)
	{
        //达到游戏人数要求禁入游戏
        if (m_pITableFrame->GetPlayerCount(true) >= GAME_PLAYER) {
            return false;
        }

        // timeoutMatchSeconds_(3.6s)内不能让机器人进入桌子
		 if (!pUser || pUser->GetUserId() == -1) 
		 {
		 	if (totalMatchSeconds_ < timeoutMatchSeconds_)
		 	{
		 		return false;
		 	}
		 }
		 else
		 {
		 	shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
		 	if (userItem) {
		 		if (userItem->IsAndroidUser()) {
		 			if (totalMatchSeconds_ < timeoutMatchSeconds_) {
		 				return false;
		 			}
		 		}
		 		else {
		 			//真实玩家底分不足，不能进入房间
		 		}
		 	}
		 }
        return true;
    }
    else if (m_cbGameStatus >= GAME_SCENE_CALL) { //(m_pITableFrame->GetGameStatus() == GAME_STATUS_START) {
        //游戏进行状态，处理断线重连，玩家信息必须存在
        shared_ptr<CServerUserItem> userItem = m_pITableFrame->GetUserItemByUserId(pUser->GetUserId());
        if (userItem == NULL) {
            return false;
        }
		////用户已出局(弃牌/比牌输)，未离开桌子，刷新页面，断线重连
		//if (isGiveup_[userItem->GetChairId()] || isComparedLost_[userItem->GetChairId()]) {
		//	//清除保留信息
		//	saveUserIDs_[userItem->GetChairId()] = 0;
		//	//用户空闲状态
		//	userItem->SetUserStatus(sFree);
		//	//清除用户信息
		//	m_pTableFrame->ClearTableUser(userItem->GetChairId(), true, false, ERROR_ENTERROOM_LONGTIME_NOOP);
		//	return false;
		//}
        return true;
    }
    //游戏状态为GAME_STATUS_START(100)时，不会进入该函数
    //assert(false);
    return false;
}
//用户断线
bool CTableFrameSink::OnActionUserOffLine(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{
    ASSERT(pIServerUserItem != NULL);
    if (pIServerUserItem == NULL) return true;
    if(m_bOffLineTrustee)
    {
    }
    return true;
}

bool CTableFrameSink::OnActionUserConnect(WORD wChairID, shared_ptr<IServerUserItem> pIServerUserItem)
{

    ASSERT(pIServerUserItem != NULL);
    if (pIServerUserItem == NULL)
    {
        return false;
    }

    pIServerUserItem->setTrustee(false);

    return true;
}

//用户放弃
bool CTableFrameSink::OnUserPassCard(WORD wChairID)
{
    //效验状态
    //ASSERT((wChairID==m_wCurrentUser)&&(m_cbTurnCardCount!=0));
    if ((wChairID!=m_wCurrentUser)||(m_cbTurnCardCount==0))
        return true;

    //设置变量
    m_wCurrentUser=(m_wCurrentUser+1)%GAME_PLAYER;
	if (m_wCurrentUser == m_wTurnWiner)
	{
		m_cbTurnCardCount = 0;
		memset(m_cbTurnCardData, 0, sizeof(m_cbTurnCardData));
	}
    openLog("===用户出牌 放弃 OnUserPassCard wChairID=%d 切换到用户 m_wCurrentUser=%d", wChairID,m_wCurrentUser);
	m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, "", -1, opPass, wChairID, wChairID);

    CMD_S_PassCard PassCard;
    PassCard.set_msgid(++m_MSGID);
    PassCard.set_dwpasscarduser(wChairID);
    PassCard.set_dwcurrentuser( m_wCurrentUser);
    PassCard.set_cbturnover(  (m_cbTurnCardCount == 0) ? 1 : 0);
    PassCard.set_cbtimeleft(  TIME_OUT_CARD);

    for (uint8_t i = 0; i < GAME_PLAYER; i++)
    {
        std::string data = PassCard.SerializeAsString();
        m_pITableFrame->SendTableData(i, SUB_S_PASS_CARD, (uint8_t*)data.c_str(), data.size());
        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_PASS_CARD,  (uint8_t* )data.c_str(),data.size());
        }
    }
    if(m_bTrustee[m_wCurrentUser])
        m_cbRemainTime = TIME_OUT_CARDTrust;
    else
        m_cbRemainTime = TIME_OUT_CARD;
    m_dwTime = (DWORD)time(NULL);
	m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
    m_TimerTrustId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime,boost::bind(&CTableFrameSink::OnUserTrustTimer, this));
    openLog("===设置出牌定时器 OnUserPassCard TIME_OUT_CARD m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
    AutoWork();
    return true;
}

//用户叫分
bool CTableFrameSink::OnUserCallScore(WORD wChairID, uint8_t cbCallScore)
{
    //效验状态
    //ASSERT(wChairID==m_wCurrentUser);
    if (wChairID!=m_wCurrentUser)
    {
        openLog("wChairID!=m_wCurrentUser");
        return true;
    }
    if (isBankerDecided())//是的，已经有地主了
    {
        openLog("isBankerDecided()");
        return true;
    }
    //效验参数,255表示不叫分
    //ASSERT(((cbCallScore>=1)&&(cbCallScore<=3)&&(cbCallScore>m_cbBankerScore))||(cbCallScore==255));
    if (cbCallScore< CB_NOT_CALL || cbCallScore > CB_NO_CALL_BENKER)
        cbCallScore = CB_NO_CALL_BENKER;

    //设置状态
    if ((cbCallScore>=CB_CALL_BENKER_1 && cbCallScore<=CB_CALL_BENKER_3) && cbCallScore>m_cbBankerScore)
    {
        m_cbBankerScore=cbCallScore;
        m_wBankerUser=m_wCurrentUser;
        openLog("===用户叫分 wChairID=%d cbCallScore=%d",wChairID,cbCallScore);
    }
    else if((cbCallScore>=CB_CALL_BENKER_1 && cbCallScore<=CB_CALL_BENKER_3) && cbCallScore<=m_cbBankerScore)
        cbCallScore = CB_NOT_CALL;
    else if((cbCallScore<CB_CALL_BENKER_1 && cbCallScore>CB_CALL_BENKER_3))
        cbCallScore = CB_NOT_CALL;

    //设置叫分
    m_cbScoreInfo[wChairID]=cbCallScore;
	m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(cbCallScore), -1, opBet, wChairID, wChairID);

    if (m_bFisrtCall)
    {
        m_bFisrtCall = false;
        m_cbFirstCall = wChairID;
    }

    //设置用户
    if ((m_cbBankerScore==3) || (m_wFirstUser==(wChairID+1)%GAME_PLAYER))
    {
        m_wCurrentUser=INVALID_CHAIR;
    }
    else
    {
        m_wCurrentUser=(wChairID+1)%GAME_PLAYER;
    }
    m_dwStartTime = m_cbBankerScore;
    //构造变量
    CMD_S_CallBanker CallBanker;
    CallBanker.set_dwlastuser(wChairID);
    CallBanker.set_cbtimeleft(TIME_CALL_BANKERSend);

    //开始判断
    if ((m_cbBankerScore==3)||(m_wFirstUser==(wChairID+1)%GAME_PLAYER))//轮一圈或者有人叫三分了
    {
        if (m_wBankerUser == INVALID_CHAIR && m_cbNoCallCount<3)
        {
            //这里是用作假如三个玩家都不叫地主的话，就重新开局
            OnGameStart();
            return true;
        }
        m_cbNoCallCount = 0;
        //发送消息
        CallBanker.set_dwbankeruser(m_wBankerUser);
        CallBanker.set_dwcurrentuser( m_wCurrentUser);
        CallBanker.set_cbcallinfo(cbCallScore);
        std::string data = CallBanker.SerializeAsString();
        //发送消息
        for (WORD i=0;i<GAME_PLAYER;i++)
        {
            if(NULL == m_pITableFrame->GetTableUserItem(i)) continue;
            m_pITableFrame->SendTableData(i,SUB_S_CALL_BANKER,(uint8_t*)data.c_str(),data.size());
            //tmp......
            if(m_bReplayRecordStart)
            {
                RepayRecord(i,SUB_S_CALL_BANKER, (uint8_t* )data.c_str(),data.size());
            }
        }
        //设置变量
        if (m_cbBankerScore==0)
            m_cbBankerScore=1;
        if (m_wBankerUser==INVALID_CHAIR)
            m_wBankerUser=m_wFirstUser;
		m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(m_wBankerUser), -1, opBanker, -1, -1);
        //发送底牌
        m_cbHandCardCount[m_wBankerUser]+=CountArray(m_cbBankerCard);
        CopyMemory(&m_cbHandCardData[m_wBankerUser][NORMAL_COUNT],m_cbBankerCard,sizeof(m_cbBankerCard));
		m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, GlobalFunc::converToHex(m_cbBankerCard, 3), -1, opShowCard, m_wBankerUser, m_wBankerUser);
        //排列扑克
        m_GameLogic.SortCardList(m_cbHandCardData[m_wBankerUser],m_cbHandCardCount[m_wBankerUser],ST_ORDER);
		CopyMemory(m_cbHandCardAllData, m_cbHandCardData, sizeof(m_cbHandCardData));
        //设置用户
        m_wTurnWiner=m_wBankerUser;
        m_wCurrentUser=m_wBankerUser;
        m_cbGameStatus = GAME_SCENE_CALLEnd;
        m_cbRemainTime = 3;
        m_dwTime = (DWORD)time(NULL);
		m_TimerLoopThread->getLoop()->cancel(m_TimerCallBankerId);
        m_TimerId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime*0.1,boost::bind(&CTableFrameSink::OnTimer, this));
        openLog("===叫分结束 GAME_SCENE_CALLEnd m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
        AutoWork();
        return true;
    }
    //发送消息
    CallBanker.set_dwbankeruser(INVALID_CHAIR);
    CallBanker.set_dwcurrentuser( m_wCurrentUser);
    CallBanker.set_cbcallinfo(cbCallScore);
    std::string data = CallBanker.SerializeAsString();
    for (WORD i=0;i<GAME_PLAYER;i++)
    {
        if(NULL == m_pITableFrame->GetTableUserItem(i))
            continue;
        m_pITableFrame->SendTableData(i, SUB_S_CALL_BANKER, (uint8_t*)data.c_str(), data.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_CALL_BANKER, (uint8_t* )data.c_str(),data.size());
        }
    }

    m_cbRemainTime = TIME_CALL_BANKERSend;
    m_dwTime = (DWORD)time(NULL);
	m_TimerLoopThread->getLoop()->cancel(m_TimerCallBankerId);
    m_TimerCallBankerId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime,boost::bind(&CTableFrameSink::OnCallBankerTimer, this));
    openLog("===TIME_CALL_BANKERSend m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
    AutoWork();
    return true;
}

bool CTableFrameSink::OnUserStakeScore(WORD wChairID, uint8_t cbStakeScore)
{
    //效验参数
    if (CB_NOT_ADD_DOUBLE == cbStakeScore)
        cbStakeScore = CB_NO_ADD_DOUBLE;

    //设置状态
    //m_GameRecord.AppendFormat(TEXT("%s加倍,%d"), m_szNickName[wChairID], cbStakeScore);
	if (m_cbStakeInfo[wChairID] != 0 || m_wBankerUser == wChairID)
		return true;

    //设置下注
    openLog("===玩家选择 wChairID=%d %s",wChairID,cbStakeScore==CB_ADD_DOUBLE?"加倍":"不加倍");
    m_cbStakeInfo[wChairID] = cbStakeScore;
	m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(cbStakeScore), -1, opDouble, wChairID, wChairID);

    //构造变量
    CMD_S_Double DoubleInfo;
    DoubleInfo.set_dwcurrentuser( wChairID);
    DoubleInfo.set_cbdouble( cbStakeScore);

    //发送消息
    for (WORD i = 0; i<GAME_PLAYER; i++)
    {
        if (NULL == m_pITableFrame->GetTableUserItem(i)) continue;
        DWORD usertimes=1;
        //这个倍数是指除加倍以外的倍数
        if (i == m_wBankerUser)
        {
            for (WORD j = 0; j<GAME_PLAYER; j++)
            {
                if (j == m_wBankerUser)
                {
                    usertimes++;
                    continue;
                }
                if(m_cbStakeInfo[j] == CB_ADD_DOUBLE)
                    usertimes++;
            }
        }
        else
        {
            if(m_cbStakeInfo[i] == CB_ADD_DOUBLE)
                usertimes++;
        }
        DoubleInfo.set_wusertimes(usertimes) ;
        std::string data = DoubleInfo.SerializeAsString();
        m_pITableFrame->SendTableData(i, SUB_S_ADD_DOUBLE, (uint8_t*)data.c_str(),data.size());

        //tmp......
        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_ADD_DOUBLE, (uint8_t* )data.c_str(),data.size());
        }
    }
    //	m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_STAKE_SCORE, &StakeScore, sizeof(StakeScore));
    int nUser = 0;
    for (int i=0; i<GAME_PLAYER; ++i)
    {
        if (m_cbStakeInfo[i]>0)
        {
            nUser++;
        }
    }
    //加倍结束
    if (nUser == GAME_PLAYER-1)
    {
        m_cbGameStatus = GAME_SCENE_STAKEEnd;
        m_cbRemainTime = 3;
        m_dwTime = (DWORD)time(NULL);
		m_TimerLoopThread->getLoop()->cancel(m_TimerCallStakeId);
        m_TimerId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime*0.1,boost::bind(&CTableFrameSink::OnTimer, this));
        openLog("===加倍结束 GAME_SCENE_STAKEEnd m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
        AutoWork(); //加倍结束这里没进去
        return true;
    }

    return true;
}

//用户出牌
bool CTableFrameSink::OnUserOutCard(WORD wChairID, uint8_t cbCardData[], uint8_t cbCardCount,bool bSystem)
{
    //效验状态
    ASSERT(wChairID==m_wCurrentUser);
    if (wChairID!=m_wCurrentUser) return true;
    //获取类型
    uint8_t cbCardType=m_GameLogic.GetCardType(cbCardData,cbCardCount);

	char tmp[100] = { 0 };
	GetValue(cbCardData, cbCardCount, tmp);

    //类型判断
    if (cbCardType==CT_ERROR)
    {
        //ASSERT(FALSE);
        openLog("===OnUserOutCard cbCardType==CT_ERROR cbCardData=%s", tmp);
        return false;
    }

	openLog("===用户出牌 wChairID=%d %s", wChairID, tmp);
    //出牌判断
    if (m_cbTurnCardCount!=0)
    {
        //对比扑克
        if (m_GameLogic.CompareCard(m_cbTurnCardData,cbCardData,m_cbTurnCardCount,cbCardCount)==false)
        {
            openLog("===OnUserOutCard cbCardCount:%d ",cbCardCount);
            //ASSERT(FALSE);
            return false;
        }
    }
    char tmp1[100]={0};
    GetValue(m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID],tmp1);
    openLog("===删除扑克before wChairID=%d %s",wChairID,tmp1);
    //删除扑克
    if (m_GameLogic.RemoveCardList(cbCardData,cbCardCount,m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID])==false)
    {
        //ASSERT(FALSE);
        openLog("===m_GameLogic.RemoveCardList(cbCardData,cbCardCount,m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID])==false ");
        return false;
    }
    char tmp2[100]={0};
    GetValue(m_cbHandCardData[wChairID],m_cbHandCardCount[wChairID]-cbCardCount,tmp2);
    openLog("===删除扑克end wChairID=%d %s",wChairID,tmp2);

    m_TimerLoopThread->getLoop()->cancel(m_TimerTrustId);
    //记牌器记录
    for(WORD i = 0;i<cbCardCount;i++)
    {
        m_cbSurplusCard[m_cbSurplusCount++] = cbCardData[i];
    }

    //出牌变量
    uint8_t cbOutCardCount = m_cbOutCardCount[wChairID];
    m_cbOutCardCount[wChairID]++;
    m_cbOutCardListCount[wChairID][cbOutCardCount] = cbCardCount;
    memcpy(&m_cbOutCardList[wChairID][cbOutCardCount], cbCardData, cbCardCount);
	m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime, to_string(cbCardType) + ":" + GlobalFunc::converToHex(cbCardData, cbCardCount), -1, opOutCard, wChairID, wChairID);
    //设置变量
    m_cbTurnCardCount=cbCardCount;
    m_cbHandCardCount[wChairID]-=cbCardCount;
    memset(m_cbTurnCardData,0,sizeof(m_cbTurnCardData));
    CopyMemory(m_cbTurnCardData,cbCardData,sizeof(uint8_t)*cbCardCount);

    //炸弹判断
    if ((cbCardType==CT_BOMB_CARD)||(cbCardType==CT_MISSILE_CARD))
    {
        m_cbBombCount++;
        m_cbEachBombCount[wChairID]++;
    }

    //切换用户
    m_wTurnWiner=wChairID;
    if (m_cbHandCardCount[wChairID] != 0)
    {
		m_wCurrentUser = (m_wCurrentUser + 1) % GAME_PLAYER;
    }
    else
    {
        m_wCurrentUser = INVALID_CHAIR;
    }
    openLog("===下一位用户 m_wCurrentUser=%d",m_wCurrentUser);   
    //构造数据
    CMD_S_OutCard OutCard;
    OutCard.set_msgid(++m_MSGID);
    OutCard.set_cbtimeleft(TIME_OUT_CARD);
    OutCard.set_cbcardcount(cbCardCount);
    OutCard.set_dwcurrentuser(m_wCurrentUser);
    OutCard.set_dwoutcarduser(wChairID);
    for(int i=0;i<cbCardCount;i++)
        OutCard.add_cbcarddata(cbCardData[i]);

    for (WORD i = 0;i<GAME_PLAYER;i++)
    {
        DWORD userTime = 1;
        if (i == m_wBankerUser)
        {
            for (WORD j = 0; j<GAME_PLAYER; j++)
            {
                if (j == m_wBankerUser)
                {
                    userTime++;
                    continue;
                }
                if(m_cbStakeInfo[j] == CB_ADD_DOUBLE)
                    userTime++;
            }
            for (uint8_t i=0;i<m_cbBombCount;i++)
                userTime*=2;
        }
        else
        {
            if(m_cbStakeInfo[i] == CB_ADD_DOUBLE)
                userTime++;

            for (uint8_t i=0;i<m_cbBombCount;i++)
                userTime*=2;
        }
        OutCard.set_wusertimes(userTime ) ;
        std::string data = OutCard.SerializeAsString();
        m_pITableFrame->SendTableData(i, SUB_S_OUT_CARD, (uint8_t*)data.c_str(), data.size());

        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_OUT_CARD, (uint8_t* )data.c_str(),data.size());
        }
    }

    //结束判断
    if (m_wCurrentUser == INVALID_CHAIR)
    {
        OnEventGameConclude(wChairID, GER_NORMAL);
    }
    else
    {
        /*
        for (WORD i = 0;i<GAME_PLAYER;i++)
        {
            if(i==wChairID)
                continue;
            if(m_pITableFrame->GetTableUserItem(i)->IsAndroidUser()==true && m_bTrustee[i] == true)
                OnUserTrust(i, false);
        }
        */

        if(m_bTrustee[m_wCurrentUser])
            m_cbRemainTime = TIME_OUT_CARDTrust;
        else
            m_cbRemainTime = TIME_OUT_CARD;
        m_dwTime = (DWORD)time(NULL);
        m_TimerTrustId = m_TimerLoopThread->getLoop()->runAfter(m_cbRemainTime,boost::bind(&CTableFrameSink::OnUserTrustTimer, this));
        openLog("===设置托管出牌定时器 OnUserOutCard TIME_OUT_CARD m_cbRemainTime=%d timecount=%d",static_cast<int>(m_cbRemainTime),++timeCount);
        AutoWork();
    }

    return true;
}

bool CTableFrameSink::OnUserTrust(WORD wChairID, bool bTrust)
{
	openLog("===玩家托管操作 OnUserTrust wChairID=%d, bTrust = %s ", wChairID , bTrust? "true": "false");
    if (NULL == m_pITableFrame->GetTableUserItem(wChairID))
    {
        //ASSERT(0);
        return false;
    }
    m_bTrustee[wChairID] = bTrust;
    CMD_S_StatusTrustee StatusTrustee;
    for(uint8_t i = 0; i < GAME_PLAYER; i++)
        StatusTrustee.add_cbtrustee(m_bTrustee[i]);

    std::string data = StatusTrustee.SerializeAsString();
    //发送数据
    for (uint8_t i = 0; i < GAME_PLAYER; i++)
    {
        m_pITableFrame->SendTableData(i, SUB_S_TRUSTEE, (uint8_t*)data.c_str(), data.size());

        if(m_bReplayRecordStart)
        {
            RepayRecord(i,SUB_S_TRUSTEE, (uint8_t* )data.c_str(),data.size());
        }
    }

    //m_pITableFrame->SendLookonData(INVALID_CHAIR, SUB_S_TRUSTEE, &Trustee, sizeof(Trustee));
}

//获取剩余牌
bool CTableFrameSink::OnUserRemainCardNum(WORD wChairID)
{
    return true;
}

//牌转文字
CString CTableFrameSink::TransformCardInfo( uint8_t cbCardData )
{
    CString str = TEXT("");

    uint8_t cbCardValue=cbCardData&MASK_VALUE;
    switch(cbCardValue)
    {
    case 0x01:
    {
        str += TEXT("A");
        break;
    }
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
    {
        str.Format( TEXT("%d"),cbCardValue);
        break;
    }
    case 0x0A:
    {
        str += TEXT("10");
        break;
    }
    case 0x0B:
    {
        str += TEXT("J");
        break;
    }
    case 0x0C:
    {
        str += TEXT("Q");
        break;
    }
    case 0x0D:
    {
        str += TEXT("K");
        break;
    }
    case 0x0E:
    {
        str += TEXT("小");
        break;
    }
    case 0X0F:
    {
        str += TEXT("大");
        break;
    }
    default:
        ASSERT(FALSE);
    }

    uint8_t cbCardColor = (cbCardData&MASK_COLOR);
    switch( cbCardColor )
    {
    case 0x00:
        str += TEXT("方块");
        break;
    case 0x10:
        str += TEXT("梅花");
        break;
    case 0x20:
        str += TEXT("红桃");
        break;
    case 0x30:
        str += TEXT("黑桃");
        break;
    case 0x40:
        str += TEXT("王");
        break;
    default:
        ASSERT(FALSE);
    }

    return str;
}

//是否可以出掉最后一手牌，而无需等待
bool CTableFrameSink::IsCanOutAllCard(WORD wChairID)
{
    ASSERT(m_cbGameStatus == GAME_SCENE_PLAY && wChairID == m_wCurrentUser);
    if (m_cbGameStatus != GAME_SCENE_PLAY || wChairID != m_wCurrentUser)
        return false;
    tagSearchCardResult SearchCardResult;
    ZeroMemory(&SearchCardResult, sizeof(tagSearchCardResult));
	// openLog("===是否可以出掉最后一手牌 111  IsCanOutAllCard wChairID=%d", wChairID);
    //搜索提示
    try
    {
        m_GameLogic.SearchOutCard(m_cbHandCardData[m_wCurrentUser], m_cbHandCardCount[m_wCurrentUser], m_cbTurnCardData, m_cbTurnCardCount,
                                  &SearchCardResult);
    }
    catch (...)
    {
        ASSERT(FALSE);
        SearchCardResult.cbSearchCount = 0;
    }
	// openLog("===是否可以出掉最后一手牌 222  IsCanOutAllCard wChairID=%d", wChairID);
    uint8_t cbCardCount = m_cbHandCardCount[wChairID];

    if (m_cbHandCardCount[wChairID] > 0 && SearchCardResult.cbSearchCount > 0 && cbCardCount > 0)
    {
        bool bOutAll = false;
        bool bExistFourTake = false;

        for (int i = 0; i < SearchCardResult.cbSearchCount; ++i)
        {
            if (SearchCardResult.cbCardCount[i] == cbCardCount)
            {
                bOutAll = true;
                if (m_GameLogic.GetCardType(m_cbTurnCardData, m_cbTurnCardCount) != CT_ERROR)
                {
					openLog("===是否可以出掉最后一手牌 333 True  IsCanOutAllCard wChairID=%d", wChairID);
                    return true;
                }
                else
                {
                    uint8_t cbCardType = m_GameLogic.GetCardType(SearchCardResult.cbResultCard[i], SearchCardResult.cbCardCount[i]);
                    if (CT_FOUR_TAKE_ONE == cbCardType || CT_FOUR_TAKE_TWO == cbCardType)
                    {
                        bExistFourTake = true;
                        break;
                    }
                }
            }
        }
		if (bExistFourTake) {
			openLog("===是否可以出掉最后一手牌 444 bExistFourTake false  IsCanOutAllCard wChairID=%d", wChairID);
			return false;
		}
		openLog("===是否可以出掉最后一手牌 555 bOutAll=%s  IsCanOutAllCard wChairID=%d", bOutAll ? "True" : "False",wChairID);
        return bOutAll;
    }
    else
    {
		openLog("===是否可以出掉最后一手牌 666 false  IsCanOutAllCard wChairID=%d", wChairID);
        return false;
    }
}

//自动出牌
bool CTableFrameSink::OnAutoOutCard(WORD wChairID)
{
    ASSERT(m_cbGameStatus == GAME_SCENE_PLAY && wChairID == m_wCurrentUser);
    if (m_cbGameStatus!=GAME_SCENE_PLAY || wChairID != m_wCurrentUser )
        return false;
    openLog("===玩家自动出牌 start OnAutoOutCard wChairID=%d ", wChairID);
    //搜索提示
    try
    {
        memset(&m_SearchCardResult,0,sizeof(m_SearchCardResult));
        //test
        /*
		const uint8_t	CGameLogic::m_cbCardData[FULL_COUNT]=
		{
			0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
			0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
			0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
			0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
			0x4E,0x4F,
		[20181127 22:48:08]用户出牌 wChairID=1 方8
		[20181127 22:48:08]删除扑克before wChairID=1 黑Q 红Q 方Q 黑J 红J 梅J 黑9 方9 梅8
		};

        ZeroMemory(m_cbHandCardData[m_wCurrentUser],sizeof(m_cbHandCardData[m_wCurrentUser]));
        m_cbHandCardData[m_wCurrentUser][0] = 0x3C;
        m_cbHandCardData[m_wCurrentUser][1] = 0x2C;
        m_cbHandCardData[m_wCurrentUser][2] = 0x0C;
        m_cbHandCardData[m_wCurrentUser][3] = 0x3B;
        m_cbHandCardData[m_wCurrentUser][4] = 0x2B;
        m_cbHandCardData[m_wCurrentUser][5] = 0x1B;
        m_cbHandCardData[m_wCurrentUser][6] = 0x39;
        m_cbHandCardData[m_wCurrentUser][7] = 0x09;
        m_cbHandCardData[m_wCurrentUser][8] = 0x18;

        m_cbHandCardCount[m_wCurrentUser]=9;
        m_cbTurnCardData[0]=0x23;

        m_cbTurnCardCount = 1;

        char tmp1[100]={0};
        GetValue(m_cbHandCardData[m_wCurrentUser],m_cbHandCardCount[m_wCurrentUser],tmp1);
        openLog("m_cbHandCardCount %s",tmp1);

        ZeroMemory(tmp1,sizeof(tmp1));
        GetValue(m_cbTurnCardData,m_cbTurnCardCount,tmp1);
        openLog("m_cbTurnCardData %s",tmp1);
    */
        m_GameLogic.SearchOutCard(m_cbHandCardData[m_wCurrentUser],m_cbHandCardCount[m_wCurrentUser],m_cbTurnCardData,m_cbTurnCardCount,
                                  &m_SearchCardResult);
    }catch(...)
    {
        ASSERT(FALSE);
        m_SearchCardResult.cbSearchCount = 0;
    }

    uint8_t cbCardCount = m_cbHandCardCount[wChairID];

    if(m_cbHandCardCount[wChairID]>0 && m_SearchCardResult.cbSearchCount > 0 && cbCardCount>0)
    {
        bool bOutAll = false;
        bool bExistFourTake = false;
        int iOutIndex = 0;
        for (int i = 0; i < m_SearchCardResult.cbSearchCount; ++i)
        {
            if (m_SearchCardResult.cbCardCount[i] == cbCardCount)
            {
                bOutAll = true;
                iOutIndex = i;
                if (m_cbTurnCardCount != 0 && m_GameLogic.GetCardType(m_cbTurnCardData, m_cbTurnCardCount) != CT_ERROR)
                {
                    break;
                }
                else
                {
                    uint8_t cbCardType = m_GameLogic.GetCardType(m_SearchCardResult.cbResultCard[i], m_SearchCardResult.cbCardCount[i]);
                    if (CT_FOUR_TAKE_ONE == cbCardType || CT_FOUR_TAKE_TWO == cbCardType)
                    {
                        bExistFourTake = true;
                        break;
                    }
                }
            }
        }

        if (bExistFourTake)
        {
            bOutAll = false;
        }
        //最后一手牌
        if (bOutAll)
        {
            uint8_t cbCardData[MAX_COUNT];
            ZeroMemory(cbCardData, sizeof(cbCardData));
            CopyMemory(cbCardData, m_SearchCardResult.cbResultCard[iOutIndex], sizeof(uint8_t)*m_SearchCardResult.cbCardCount[iOutIndex]);
            m_GameLogic.SortCardList(cbCardData, m_SearchCardResult.cbCardCount[iOutIndex], ST_ORDER);
			openLog("===玩家自动出最后一手牌 OnAutoOutCard =>> OnUserOutCard wChairID=%d ", wChairID);
            return OnUserOutCard(wChairID, cbCardData, m_SearchCardResult.cbCardCount[iOutIndex], true);
        }
        else if (m_cbTurnCardCount == 0)
        {
            tagSearchCardResult SearchCardResult;
            m_GameLogic.SearchOutCard( m_cbHandCardData[m_wCurrentUser],m_cbHandCardCount[m_wCurrentUser],NULL,0,&SearchCardResult);
            if(SearchCardResult.cbSearchCount > 0)
            {
                bool end = OnUserOutCard(m_wCurrentUser, SearchCardResult.cbResultCard[0], SearchCardResult.cbCardCount[0],true);
                if (end == false)
                {
                    uint8_t cbOutCardData[MAX_COUNT];
                    ZeroMemory(cbOutCardData, sizeof(cbOutCardData));
                    cbOutCardData[0] = m_cbHandCardData[wChairID][cbCardCount-1];
					openLog("===OnAutoOutCard m_cbTurnCardCount 0 wChairID=%d ", wChairID);
                    return OnUserOutCard(wChairID, cbOutCardData, 1, false);
                }
            }
            else
            {
                uint8_t cbOutCardData[MAX_COUNT];
                ZeroMemory(cbOutCardData, sizeof(cbOutCardData));
                cbOutCardData[0] = m_cbHandCardData[wChairID][cbCardCount-1];
				openLog("===OnAutoOutCard m_cbTurnCardCount 0 clean wChairID=%d ", wChairID);
                return OnUserOutCard(wChairID, cbOutCardData, 1, false);
            }
			openLog("===OnAutoOutCard 111 OnUserOutCard Flase wChairID=%d ", wChairID);
            return true;
        }
        else
        {
            uint8_t cbCardData[MAX_COUNT];
            ZeroMemory(cbCardData, sizeof(cbCardData));
            CopyMemory(cbCardData, m_SearchCardResult.cbResultCard[0], sizeof(uint8_t)*m_SearchCardResult.cbCardCount[0]);
            m_GameLogic.SortCardList(cbCardData, m_SearchCardResult.cbCardCount[0], ST_ORDER);
            openLog("===OnAutoOutCard2 m_cbTurnCardCount %d  ",m_cbTurnCardCount);
            bool end = OnUserOutCard(wChairID, cbCardData, m_SearchCardResult.cbCardCount[0], false);
            if (end == false)
            {
                return OnUserPassCard(wChairID);
            }
			openLog("===OnAutoOutCard 222 OnUserOutCard Flase wChairID=%d ", wChairID);
        }
    }
    else
    {
		openLog("===玩家自动出牌 要不起 OnAutoOutCard OnUserPassCard wChairID=%d ", wChairID);
        return OnUserPassCard(wChairID);
    }
}
//自动叫分
bool CTableFrameSink::OnAutoCallScore(WORD wChairID)
{
    ASSERT(m_cbGameStatus == GAME_SCENE_CALL && wChairID == m_wCurrentUser);
    if (m_cbGameStatus != GAME_SCENE_CALL || wChairID != m_wCurrentUser)
        return false;

    return OnUserCallScore(wChairID, 0xFF);
}

bool CTableFrameSink::OnAutoStakeScore(WORD wChairID)
{
    ASSERT(m_cbGameStatus == GAME_SCENE_STAKE);

    if (m_cbGameStatus != GAME_SCENE_STAKE)
        return false;

    if (m_cbStakeInfo[wChairID] <= 0 && m_wBankerUser != wChairID)
    {
        OnUserStakeScore(wChairID, CB_NO_ADD_DOUBLE);
    }

    return true;
}

//黑白控制
void CTableFrameSink::ListControl()
{
    //黑白用户数目及级别
    WORD wListUserCount = GAME_PLAYER;
    uint8_t cbUserLever[GAME_PLAYER];
    uint8_t cbUserSecondLever=0;
    //过滤不做控制的情况
    WORD cbUnControlCount=0;
    for(WORD i=0; i<GAME_PLAYER;i++)
    {
        cbUserLever[i]= CONTROL_STATUS_NORMAL;//m_pIUserControlSink->GetUserControlStatus(i,cbUserSecondLever);
        switch (cbUserLever[i])
        {
			//普通用户
			case CONTROL_STATUS_NORMAL:
			{
				if(cbUserSecondLever == 0) cbUnControlCount++;

				cbUserLever[i] = 0x1f-cbUserSecondLever;
				break;
			}
			//黑名单用户
			case CONTROL_STATUS_BLACK:
			{
				cbUserLever[i] = cbUserSecondLever;
				break;
			}
			//白名单用户
			case CONTROL_STATUS_WHITE:
			{
				cbUserLever[i] = 0x2f-cbUserSecondLever;
				break;
			}
			default:
				ASSERT(FALSE);
				return;
        }
    }
    //过滤不做控制的情况
    if(cbUnControlCount >= GAME_PLAYER) return;

    //分析手牌
    tagAnalyseHandInfo HandCardInfo[3];
    ZeroMemory(HandCardInfo,sizeof(HandCardInfo));

    for(WORD i=0; i<wListUserCount; i++)
    {
        AnalysebHandCard(m_cbHandCardData[i],HandCardInfo[i]);
    }

    //手牌等级排列准备
    WORD wMaxHandIndex=0;
    WORD wMiddleHandIndex=1;
    WORD wMinHandIndex=2;
    WORD wTemp=0;

    //按大小排列，相等先不做变换
    if(HandCardInfo[wMaxHandIndex].wHandCardLever<HandCardInfo[wMiddleHandIndex].wHandCardLever)
    {
        wTemp=wMaxHandIndex;
        wMaxHandIndex=wMiddleHandIndex;
        wMiddleHandIndex=wTemp;
    }

    if(HandCardInfo[wMaxHandIndex].wHandCardLever<HandCardInfo[wMinHandIndex].wHandCardLever)
    {
        wTemp=wMaxHandIndex;
        wMaxHandIndex=wMinHandIndex;
        wMinHandIndex=wTemp;
    }

    if (HandCardInfo[wMiddleHandIndex].wHandCardLever<HandCardInfo[wMinHandIndex].wHandCardLever)
    {
        wTemp=wMiddleHandIndex;
        wMiddleHandIndex=wMinHandIndex;
        wMinHandIndex=wTemp;
    }

    //判断有没有其中两个等级是相等的，相等再做细致判断，
    //最大和最小不做判断，而且最多只判断其中一组
    if(HandCardInfo[wMaxHandIndex].wHandCardLever==HandCardInfo[wMiddleHandIndex].wHandCardLever)
    {
        CompareHandLever(HandCardInfo[wMaxHandIndex],HandCardInfo[wMiddleHandIndex]);
        if (HandCardInfo[wMaxHandIndex].wHandCardLever<HandCardInfo[wMiddleHandIndex].wHandCardLever)
        {
            wTemp=wMaxHandIndex;
            wMaxHandIndex=wMiddleHandIndex;
            wMiddleHandIndex=wTemp;
        }
    }
    else if(HandCardInfo[wMiddleHandIndex].wHandCardLever==HandCardInfo[wMinHandIndex].wHandCardLever)
    {
        CompareHandLever(HandCardInfo[wMiddleHandIndex],HandCardInfo[wMinHandIndex]);
        if (HandCardInfo[wMiddleHandIndex].wHandCardLever<HandCardInfo[wMinHandIndex].wHandCardLever)
        {
            wTemp=wMiddleHandIndex;
            wMiddleHandIndex=wMinHandIndex;
            wMinHandIndex=wTemp;
        }
    }//手牌等级排列完毕

    //玩家等级排列准备
    WORD wMaxLeverIndex=0;
    WORD wMiddleLeverIndex=1;
    WORD wMinLeverIndex=2;
    WORD wTempLever=0;

    if (cbUserLever[wMaxLeverIndex] < cbUserLever[wMiddleLeverIndex])
    {
        wTempLever=wMaxLeverIndex;
        wMaxLeverIndex=wMiddleLeverIndex;
        wMiddleLeverIndex=wTempLever;
    }
    if (cbUserLever[wMaxLeverIndex]<cbUserLever[wMinLeverIndex])
    {
        wTempLever=wMaxLeverIndex;
        wMaxLeverIndex=wMinLeverIndex;
        wMinLeverIndex=wTempLever;
    }
    if(cbUserLever[wMiddleLeverIndex]<cbUserLever[wMinLeverIndex])
    {
        wTempLever=wMiddleLeverIndex;
        wMiddleLeverIndex=wMinLeverIndex;
        wMinLeverIndex=wTempLever;
    }//玩家等级排列完毕

    //构造临时手牌
    uint8_t cbMaxLeverHandCard[NORMAL_COUNT];
    uint8_t cbMiddleLeverHandCard[NORMAL_COUNT];
    uint8_t cbMinLeverHandCard[NORMAL_COUNT];
    //先保存手牌，根据手牌等级保存
    CopyMemory(cbMaxLeverHandCard,m_cbHandCardData[wMaxHandIndex],sizeof(uint8_t)*NORMAL_COUNT);
    CopyMemory(cbMiddleLeverHandCard,m_cbHandCardData[wMiddleHandIndex],sizeof(uint8_t)*NORMAL_COUNT);
    CopyMemory(cbMinLeverHandCard,m_cbHandCardData[wMinHandIndex],sizeof(uint8_t)*NORMAL_COUNT);
    //手牌分配，根据用户级别分配
    CopyMemory(m_cbHandCardData[wMaxLeverIndex],cbMaxLeverHandCard,sizeof(uint8_t)*NORMAL_COUNT);
    CopyMemory(m_cbHandCardData[wMiddleLeverIndex],cbMiddleLeverHandCard,sizeof(uint8_t)*NORMAL_COUNT);
    CopyMemory(m_cbHandCardData[wMinLeverIndex],cbMinLeverHandCard,sizeof(uint8_t)*NORMAL_COUNT);
}
//分析扑克
void CTableFrameSink::AnalysebHandCard(uint8_t HandCardData[],tagAnalyseHandInfo &HandCardInfo)
{
    uint8_t cbTempCardData[NORMAL_COUNT];
    CopyMemory(cbTempCardData,HandCardData,sizeof(uint8_t)*NORMAL_COUNT);

    //牌型分布 下标0代表王 1代表A...13代表K
    WORD DistributCardCount[14] = {0};

    m_GameLogic.SortCardList(cbTempCardData,NORMAL_COUNT,ST_ORDER);

    //大牌数目计数
    for(WORD i=0; i<NORMAL_COUNT; i++)
    {
        if(0x4F == cbTempCardData[i])
        {
            HandCardInfo.bBigKing=true;
            DistributCardCount[0]++;
        }
        else if (0x4E == cbTempCardData[i])
        {
            HandCardInfo.bSmallKing=true;
            DistributCardCount[0]++;
        }
        else
        {
            uint8_t cbCardValue = m_GameLogic.GetCardValue(cbTempCardData[i]);
            if (cbCardValue>2)
            {
                DistributCardCount[m_GameLogic.GetCardLogicValue(cbTempCardData[i])]++;
            }
            else
            {
                DistributCardCount[m_GameLogic.GetCardValue(cbTempCardData[i])]++;
            }
        }
    }

    //火箭判断
    if(HandCardInfo.bBigKing && HandCardInfo.bSmallKing)
    {
        HandCardInfo.wBombCount++;
        HandCardInfo.wBiggestBomb=m_GameLogic.GetCardLogicValue(0x4F);
    }
    //A 2数目
    HandCardInfo.wAceCount = DistributCardCount[1];
    HandCardInfo.wTwoCount = DistributCardCount[2];

    //炸弹
    for (WORD i=1; i<14; i++)
    {
        if(4==DistributCardCount[i])
        {
            HandCardInfo.wBombCount++;
            if ((HandCardInfo.wBiggestBomb < (i<3))?(i+13):i) HandCardInfo.wBiggestBomb=(i<3)?(i+13):i;
        }
    }

    WORD wTempList[14];
    CopyMemory(wTempList,DistributCardCount,sizeof(DistributCardCount));
    WORD wTempTakeCount = HandCardInfo.wTakeCount;

    for (WORD i=1; i<14; i++)
    {
        if(3==wTempList[i]) HandCardInfo.wTakeCount++;
        if (2==wTempList[i] && i<9 && i>=3) HandCardInfo.wDoubleCardCount++;
        if (1==wTempList[i] && i<10 && i>=3) HandCardInfo.wSingleCardCount++;
    }

    //差集计算
    WORD wSingleDiffer = HandCardInfo.wTwoCount-HandCardInfo.wSingleCardCount+1;

    if(HandCardInfo.bBigKing && !HandCardInfo.bSmallKing) wSingleDiffer+=2;
    if(!HandCardInfo.bBigKing && HandCardInfo.bSmallKing) wSingleDiffer++;

    //级别计算
    if (wSingleDiffer+HandCardInfo.wTakeCount>0) HandCardInfo.wHandCardLever = 2;
    else
    {
        if (HandCardInfo.wTwoCount+HandCardInfo.wAceCount+HandCardInfo.wTakeCount+wSingleDiffer<0) HandCardInfo.wHandCardLever=0;
        else HandCardInfo.wHandCardLever=1;
    }

    //炸弹加入
    HandCardInfo.wHandCardLever+=HandCardInfo.wBombCount;
}

//比较扑克等级(当两者相等时才判断)
void CTableFrameSink::CompareHandLever(tagAnalyseHandInfo &HandCardInfo1,tagAnalyseHandInfo &HandCardInfo2)
{
    //手牌级别判断
    if(HandCardInfo1.wHandCardLever == HandCardInfo2.wHandCardLever)
    {
        //先比较炸弹
        if(HandCardInfo1.wBombCount !=0 || HandCardInfo2.wBombCount!=0)
        {
            //炸弹个数相等就比较炸弹大小
            if(HandCardInfo1.wHandCardLever == HandCardInfo2.wHandCardLever)
                HandCardInfo1.wHandCardLever += (HandCardInfo1.wBiggestBomb>HandCardInfo2.wBiggestBomb)?1:-1;
            else
                HandCardInfo1.wHandCardLever += HandCardInfo1.wBombCount-HandCardInfo2.wBombCount;
        }
        //2的个数
        else if((HandCardInfo1.wTwoCount!=0 || HandCardInfo2.wTwoCount!=0) && HandCardInfo1.wTwoCount!=HandCardInfo2.wTwoCount)
        {
            HandCardInfo1.wHandCardLever += (HandCardInfo1.wTwoCount>HandCardInfo2.wTwoCount)?1:-1;
        }
        //A的个数
        else if((HandCardInfo1.wAceCount!=0 || HandCardInfo2.wAceCount!=0) && HandCardInfo1.wAceCount!=HandCardInfo2.wAceCount)
        {
            HandCardInfo1.wHandCardLever += (HandCardInfo1.wAceCount>HandCardInfo2.wAceCount)?1:-1;
        }
    }

    return;
}

//断线托管
bool CTableFrameSink::IsOfflineTrustee(WORD wChairID)
{
    //非常规座位
    ASSERT(wChairID != INVALID_CHAIR);
    if(wChairID == INVALID_CHAIR) return false;

    //允许代打
    if (m_bOffLineTrustee)
    {
        shared_ptr<IServerUserItem>pServerUserItem=m_pITableFrame->GetTableUserItem(wChairID);
        ASSERT(pServerUserItem!=NULL);
        if (pServerUserItem == NULL) return false;
        return m_bTrustee[wChairID];
    }

    return false;
}

bool CTableFrameSink::isBankerDecided()
{
    for (int i = 0; i < GAME_PLAYER; i++)
    {
        if (m_cbHandCardCount[i] >= MAX_COUNT)
            return true;
    }
    return false;
}

bool CTableFrameSink::CalRemainCardNum()
{
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        for (int j = 0; j < MAX_TYPE1; ++j)
        {
            m_cbRemainCardNum[i][j] = 0;
        }
    }
    uint8_t cbSumOutCardNum[MAX_TYPE1];
    ZeroMemory(cbSumOutCardNum, sizeof(cbSumOutCardNum));
    for (int j = 0; j < m_cbSurplusCount; ++j)
    {
        uint8_t cbValue = m_GameLogic.GetCardValue(m_cbSurplusCard[j]) - 1;
        if (cbValue >= 0 && cbValue < MAX_TYPE1)
        {
            cbSumOutCardNum[cbValue] += 1;
        }
    }
    uint8_t cbSelfCardNum[GAME_PLAYER][MAX_TYPE1];	//玩家剩余扑克数目(依次是A - K,小王，大王)
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        ZeroMemory(cbSelfCardNum[i], sizeof(cbSelfCardNum[i]));
    }
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        for (int j = 0; j < m_cbHandCardCount[i]; ++j)
        {
            uint8_t cbValue = m_GameLogic.GetCardValue(m_cbHandCardData[i][j]) - 1;
            if (cbValue >= 0 && cbValue < MAX_TYPE1)
            {
                cbSelfCardNum[i][cbValue] += 1;
            }
        }
    }
    for (int i = 0; i < GAME_PLAYER; ++i)
    {
        for (int j = 0; j < MAX_TYPE1; ++j)
        {
            if (j == (MAX_TYPE1 - 1) || j == (MAX_TYPE1 - 2))
            {
                m_cbRemainCardNum[i][j] = 1 - (cbSelfCardNum[i][j] + cbSumOutCardNum[j]);
            }
            else
            {
                m_cbRemainCardNum[i][j] = 4 - (cbSelfCardNum[i][j] + cbSumOutCardNum[j]);
            }
        }
    }

    return true;
}
char * CTableFrameSink::GetValue(uint8_t itmp[],uint8_t count, char * name)
{
    for(int i=0 ;i<count;i++)
    {
        uint8_t cbValue = m_GameLogic.GetCardValue(itmp[i]);
        //获取花色
        uint8_t cbColor = m_GameLogic.GetCardColor(itmp[i])>>4;

        if(itmp[i] == 0x4E)
        {
            strcat(name, "0x4E,");
            continue;
        }
        else if(itmp[i] == 0x4F)
        {
            strcat(name, "0x4F,");
            continue;
        }

        if (0 == cbColor)
        {
            strcat(name, "0x0");
        }
        else if (1 == cbColor)
        {
            strcat(name, "0x1");
        }
        else if (2 == cbColor)
        {
            strcat(name, "0x2");
        }
        else if (3 == cbColor)
        {
            strcat(name, "0x3");
        }


        if (cbValue == 1)
        {
            strcat(name, "1,");
        }
        else if (cbValue == 2)
        {
            strcat(name, "2,");
        }
        else if (cbValue == 3)
        {
            strcat(name, "3,");
        }
        else if (cbValue == 4)
        {
            strcat(name, "4,");
        }
        else if (cbValue == 5)
        {
            strcat(name, "5,");
        }
        else if (cbValue == 6)
        {
            strcat(name, "6,");
        }
        else if (cbValue == 7)
        {
            strcat(name, "7,");
        }
        else if (cbValue == 8)
        {
            strcat(name, "8,");
        }
        else if (cbValue == 9)
        {
            strcat(name, "9,");
        }
        else if (cbValue == 10)
        {
            strcat(name, "A,");
        }
        else if (cbValue == 11)
        {
            strcat(name, "B,");
        }
        else if (cbValue == 12)
        {
            strcat(name, "C,");
        }
        else if (cbValue == 13)
        {
            strcat(name, "D,");
        }
    }

    return name;
}

void  CTableFrameSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);
    sprintf(Filename, "log//ddz//ddz_%d%d%d_%d_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
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

void CTableFrameSink::StoreageControl()
{
    //混乱扑克
	uint8_t cbRandCard[FULL_COUNT] = { 0 };
    m_GameLogic.RandCardList(cbRandCard,CountArray(cbRandCard));
    //获取随机数
    WORD Ratio = rand()%(100); //产生0~100的数

    WORD WinRatio = m_StorageRatio;         //定义当前输赢比例 //默认比例
    //计算是杀分还是送分
    //吃掉库存
    // m_lStockScore = 1000;
    tagStorageInfo StorageInfo ;
    ZeroMemory(&StorageInfo,sizeof(StorageInfo));
    m_pITableFrame->GetStorageScore(StorageInfo);
    m_lStockScore = StorageInfo.lEndStorage;
    m_lStorageMin = StorageInfo.lLowlimit;
    m_lStorageMax = StorageInfo.lUplimit;
	string str = "正常模式 ";
    if(m_lStockScore>=m_lStorageMax)
    {
        WinRatio= m_StorageMaxRatio;  //送分模式
		str = "送分模式 ";
    }
    if (m_lStockScore <= m_lStorageMin)
    {
        WinRatio = m_StorageMinRatio;	 //杀分模式
		str = "杀分模式 ";
    }
    bool ReturnValue=(WinRatio>Ratio && m_bTestGame == 0)?true:false; //计算机率

    //难度干涉值
    //m_difficulty
    m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
    LOG(ERROR)<<" system difficulty "<<m_difficulty;
    int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
    //难度值命中概率,全局难度,不管其它控制有什么黑名单，什么的，都必须先赚钱了
    if(randomRet<m_difficulty)
    {
        ReturnValue = true;
    }
    //发送好牌开始
    uint8_t cbAndroidUserCount = 0 ;
    //定义机器人
    WORD wAndroidUser[ GAME_PLAYER ] = {0};

    //机器人个数
    for ( WORD wChairID = 0; wChairID < GAME_PLAYER; ++wChairID )
    {
        //读取用户
        shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem( wChairID ) ;

        if (!pServerUserItem||pServerUserItem->IsAndroidUser() == false)
            continue;
        wAndroidUser[ cbAndroidUserCount ] = ( wChairID ) ;
        cbAndroidUserCount++;
    }
    //随机抽取一个机器人发送好牌
    WORD wHaveGoodCardAndroidUser = INVALID_CHAIR ;
    if (  cbAndroidUserCount >0 ) wHaveGoodCardAndroidUser = wAndroidUser[ rand() % cbAndroidUserCount] ;

    //发送好牌
    if (ReturnValue&&wHaveGoodCardAndroidUser != INVALID_CHAIR )
    {		
		openLog("===当前库存:%d,库存上限:%d,库存下限:%d,%s,ReturnValue=true,HaveGoodCard=%d;", m_lStockScore, m_lStorageMax, m_lStorageMin, str.c_str(), wHaveGoodCardAndroidUser);
        //混乱扑克
		ZeroMemory(cbRandCard, sizeof(cbRandCard));
        m_GameLogic.RandCardList(cbRandCard,CountArray(cbRandCard));

        //获取好牌扑克数据
		uint8_t cbGoodCard[NORMAL_COUNT] = { 0 };
        //获取好牌
        m_GameLogic.GetGoodCardData(cbGoodCard);

        //cbGoodCard现在这个已经是好牌数据了,把他交给机器人即可
        m_cbHandCardCount[ wHaveGoodCardAndroidUser ] = NORMAL_COUNT;
        CopyMemory(&m_cbHandCardData[ wHaveGoodCardAndroidUser ], cbGoodCard, NORMAL_COUNT ) ;

        //删除好牌数据
        m_GameLogic.RemoveGoodCardData( cbGoodCard, NORMAL_COUNT, cbRandCard, FULL_COUNT ) ;

        //其他用户扑克数据
        for ( WORD i = 0, j = 0; i < GAME_PLAYER; i++ )
        {
            //如果当前用户不是好牌机器人即把扑克数据给他
            if ( i != wHaveGoodCardAndroidUser )
            {
                //定义用户扑克数据长度
                m_cbHandCardCount[ i ] = NORMAL_COUNT;
                //复制扑克数据给用户
                CopyMemory( &m_cbHandCardData[ i ],&cbRandCard[j*m_cbHandCardCount[i]],sizeof(uint8_t)*m_cbHandCardCount[i]);
                ++j ;
            }
        }
        //因为好牌用户扑克已经删除,扑克数据中只有37张牌2*NORMAL_COUNT=34 取当前长度,3个字符
        //设置底牌
        CopyMemory(m_cbBankerCard,&cbRandCard[2*NORMAL_COUNT],sizeof(m_cbBankerCard));
        //排列底牌
        m_GameLogic.SortCardList(m_cbBankerCard,sizeof(m_cbBankerCard),ST_ORDER);
    }
    else
    {
        ////////////////////////////////不控制!这里应该是送分给真的才对的
		openLog("===当前库存:%d,库存上限:%d,库存下限:%d,%s,ReturnValue=false;", m_lStockScore, m_lStorageMax, m_lStorageMin, str.c_str());
		uint8_t TmpcbRandCard[GAME_PLAYER][MAX_COUNT] = { 0 };
		ZeroMemory(TmpcbRandCard, sizeof(TmpcbRandCard));
		uint8_t countIndex = 0;
		uint8_t UserIndex = 0;
		if (m_bTestGame && m_listTestCardArray.size() == 0) {
			ReadCardConfig();
		}
		if (m_bTestGame && m_listTestCardArray.size() > 0)
		{
			for (int i = 0; i < FULL_COUNT; i++) 
			{
				if (UserIndex<GAME_PLAYER)
				{
					TmpcbRandCard[UserIndex][countIndex++] = *(m_listTestCardArray.begin());
					if (countIndex >= NORMAL_COUNT)
					{
						countIndex = 0;
						UserIndex++;
					}
				}
				else if(countIndex < 3)
				{
					m_cbBankerCard[countIndex++] = *(m_listTestCardArray.begin());
				}								
				m_listTestCardArray.pop_front();
			}
			m_listTestCardArray.clear();
		} 
		if (m_bTestGame==1)
		{
			WORD dwTrueUser = 0;
			for (int i = 0;i < GAME_PLAYER;i++)
			{
				m_cbHandCardCount[i] = NORMAL_COUNT;
				shared_ptr<IServerUserItem>pServerUserItem = m_pITableFrame->GetTableUserItem(i);
				if (!pServerUserItem->IsAndroidUser())
				{
					dwTrueUser = i;
					//break;
				}
			}

			WORD upChairID = (dwTrueUser + GAME_PLAYER - 1) % GAME_PLAYER;   //上家
			WORD downChairID = (dwTrueUser + 1) % GAME_PLAYER; //下家
			WORD upHandCard = (m_cbTestUser + GAME_PLAYER - 1) % GAME_PLAYER;   //上家
			WORD downHandCard = (m_cbTestUser + 1) % GAME_PLAYER; //下家
			CopyMemory(m_cbHandCardData[upChairID], TmpcbRandCard[upHandCard], sizeof(uint8_t)*NORMAL_COUNT);
			CopyMemory(m_cbHandCardData[dwTrueUser], TmpcbRandCard[m_cbTestUser], sizeof(uint8_t)*NORMAL_COUNT);
			CopyMemory(m_cbHandCardData[downChairID], TmpcbRandCard[downHandCard], sizeof(uint8_t)*NORMAL_COUNT);
		} 
		else
		{
			for (WORD i = 0;i < GAME_PLAYER;i++)
			{
				m_cbHandCardCount[i] = NORMAL_COUNT;
				CopyMemory(&m_cbHandCardData[i], &cbRandCard[i*m_cbHandCardCount[i]], sizeof(uint8_t)*m_cbHandCardCount[i]);
			}
			//设置底牌
			CopyMemory(m_cbBankerCard, &cbRandCard[DISPATCH_COUNT], sizeof(m_cbBankerCard));
		}
		
        //排列底牌
        m_GameLogic.SortCardList(m_cbBankerCard,sizeof(m_cbBankerCard),ST_ORDER);
        /////结束发送扑克
    }
}

void CTableFrameSink::SendWinSysMessage()
{
    for(int i=0;i<GAME_PLAYER;i++)
    {
        shared_ptr<IServerUserItem>  pUserItem=nullptr;
        pUserItem = m_pITableFrame->GetTableUserItem(i);
        if(pUserItem==nullptr)
            continue;

        if(m_EndWinScore[i] >= m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
            openLog("===恭喜玩家 UserId=%d,%s,在斗地主中一把赢得[%d]元!", pUserItem->GetUserId(), pUserItem->GetNickName().c_str(), m_EndWinScore[i]);
            m_pITableFrame->SendGameMessage(i,"",SMT_GLOBAL|SMT_SCROLL,m_EndWinScore[i]);
        }
    }
}

bool CTableFrameSink::CleanUser()
{
    bool isRealUser = false;
    for(uint8_t j = 0; j < GAME_PLAYER; j++)
    {
        m_pUserItem=m_pITableFrame->GetTableUserItem(j);
        if(m_pUserItem )
        {
            if(m_pUserItem->IsAndroidUser()== false)
                isRealUser= true;
            break;
        }
        else
            continue;
    }
    if(!isRealUser)
    {
        //m_TimerLoopThread->getLoop()->cancel(m_TimerId);
        for(uint8_t j = 0; j < GAME_PLAYER; j++)
        {
            m_pUserItem=m_pITableFrame->GetTableUserItem(j);
            if(m_pUserItem && m_pUserItem->IsAndroidUser()== true)
            {
                openLog("===IsAndroidUser isRealUser ClearTableUser");
                m_pITableFrame->ClearTableUser(j,true,true);
                openLog("===IsAndroidUser isRealUser ClearTableUser end");
            }
        }
    }
    return true;
}

bool CTableFrameSink::RepayRecorduserinfo(int wChairID)
{
    return true;
}

bool CTableFrameSink::RepayRecorduserinfoStatu(int wChairID,int status)
{
    return true;
}

bool CTableFrameSink::RepayRecordStart()
{
    return true;
}
bool CTableFrameSink::RepayRecordEnd()
{
    return true;
}

bool CTableFrameSink::RepayRecord(int wChairID,WORD wSubCmdId, uint8_t* data, WORD wSize)
{
    return true;
}
bool CTableFrameSink::bHaveTheSameCard()
{
	bool bHaveSameCard = false;
	//检查牌值是否重复
	uint8_t color = 0;
	uint8_t value = 0;
	uint8_t kingcount = 0;
	uint8_t handCardColorIndex[4][13] = { 0 };
	for (int i = 0;i < GAME_PLAYER;++i)
	{
		shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(i);
		if (!pIServerUserItem) continue;

		for (int j = 0; j < MAX_COUNT; ++j)
		{
			if (m_cbHandCardData[i][j] == 0)
			{
				continue;
			}
			color = m_GameLogic.GetCardColor(m_cbHandCardData[i][j]) >> 4;
			if (color < 4)
			{
				value = m_GameLogic.GetCardValue(m_cbHandCardData[i][j]);
				handCardColorIndex[color][value - 1]++;
				if (handCardColorIndex[color][value - 1] > 1)
				{
					bHaveSameCard = true;
					break;
				}
			}
			else
			{
				kingcount++;
				if (kingcount > 2)
				{
					bHaveSameCard = true;
					break;
				}
			}
		}
		if (bHaveSameCard)
			break;
	}
	return bHaveSameCard;
}

extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
	shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
	shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
	return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
	pSink.reset();
}

//////////////////////////////////////////////////////////////////////////////////
