#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

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
#include "proto/Erqs.Message.pb.h"

#include "CTableMgr.h"
#include "CPart.h"
#include "GamePlayer.h"
#include "MjAlgorithm.h"
#include "CMD_Game.h"

using namespace Erqs;
#include "GameProcess.h"

#define NONE         "\033[m"
#define RED          "\033[0;32;31m"
#define GREEN        "\033[0;32;32m"
#define YELLOW       "\033[0;32;33m"

void asyncOutput(const char* msg, int len)
{
	string out = msg;
	int pos = out.find("ERROR");
	if (pos >= 0) {
		out = RED + out + NONE;
	}

	// dump the warning now.
	pos = out.find("WARN");
	if (pos >= 0) {
		out = YELLOW + out + NONE;
	}

	// dump the special content for write the output window now.
	size_t n = std::fwrite(out.c_str(), 1, out.length(), stdout);
	(void)n;
}

vector<string> s_split(const string& in, const string& delim) {
	boost::regex re{ delim };
	return vector<string> {
		boost::sregex_token_iterator(in.begin(), in.end(), re, -1),
			boost::sregex_token_iterator()
	};
}

void intListParse(string str, vector<int32_t> & des)
{
	vector<string> strs = s_split(str, ",");
	for (auto strItem : strs)
	{
		des.push_back(stoi(strItem));
	}
}

string CardsToString(vector<int32_t>& cards)
{
	stringstream ss;
	ss << std::setfill('0') << hex << std::setw(2);
	for (uint32_t i=0; i<cards.size(); ++i)
	{
		ss << cards[i];
		if (i != cards.size() -1)
		{
			ss << "-";
		}
	}
	return ss.str();
}

string CardToString(int32_t card)
{
	stringstream ss;
	ss << std::setfill('0') << hex << std::setw(2);
	ss << card;
	return ss.str();
}

CGameProcess::CGameProcess(void)
	:m_pGameRoomInfo(nullptr)
	, m_pTableFrame(nullptr)
	, m_pLoopThread(nullptr)
{
	ClearGameData();
	m_lMarqueeMinScore = 1000;
	muduo::Logger::setOutput(asyncOutput);
	if (!Init())
	{
		WARNLOG("游戏初始化失败");
	}
}

void Test(vector<int32_t> & handCards, vector<int32_t> & outCards)
{
	/////////////////以下添加生成代码////////////////

	//////////////以上添加生成代码//////////////////
	array<int32_t, TABLE_SIZE> leftTable;
	leftTable.fill(4);
	array<int32_t, TABLE_SIZE> handTable = { 0 };
	array<int32_t, TABLE_SIZE> outTable = { 0 };
	cardsToTable(handCards, handTable);
	cardsToTable(outCards, outTable);
	for (int32_t i = 1; i < TABLE_SIZE; ++i)
	{
		leftTable[i] -= handTable[i];
		leftTable[i] -= outTable[i];
	}
	PrInfo prInfo;
	vector<int32_t> outCardsExclude;
	findPrInfo(handCards, outCardsExclude, leftTable, prInfo);
	stringstream ss;
	ss << "测试需要的牌 ";
	ss << "[ ";
	for (auto card : prInfo.needCards)
	{
		ss << getCardStr(card) << " ";
	}
	ss << "]";
	WARNLOG(ss.str());
}

void Test()
{
	/////////////////以下添加生成代码////////////////
	vector<int32_t> handCards;
	vector<int32_t> outCards;
	int32_t leftNum;
	leftNum = 46;
	handCards.push_back(2);
	handCards.push_back(3);
	handCards.push_back(4);
	handCards.push_back(8);
	handCards.push_back(8);
	handCards.push_back(9);
	handCards.push_back(51);
	handCards.push_back(51);
	handCards.push_back(54);
	handCards.push_back(54);
	handCards.push_back(68);
	handCards.push_back(69);
	handCards.push_back(70);
	outCards.push_back(1);
	outCards.push_back(3);
	outCards.push_back(4);
	outCards.push_back(5);
	outCards.push_back(9);
	outCards.push_back(49);
	outCards.push_back(49);
	outCards.push_back(50);
	outCards.push_back(50);
	outCards.push_back(50);
	outCards.push_back(53);
	outCards.push_back(53);
	outCards.push_back(55);


	//////////////以上添加生成代码//////////////////
	Test(handCards, outCards);
	array<int32_t, TABLE_SIZE> leftTable;
	leftTable.fill(4);
	array<int32_t, TABLE_SIZE> handTable = { 0 };
	array<int32_t, TABLE_SIZE> outTable = { 0 };
	cardsToTable(handCards, handTable);
	cardsToTable(outCards, outTable);
	for (int32_t i = 1; i < TABLE_SIZE; ++i)
	{
		leftTable[i] -= handTable[i];
		leftTable[i] -= outTable[i];
	}
	PrInfo prInfo;
	vector<int32_t> outCardsExclude;
	findPrInfo(handCards, outCardsExclude, leftTable, prInfo);
	stringstream ss;
	ss << "测试需要的牌 ";
	ss << "[ ";
	for (auto card : prInfo.needCards)
	{
		ss << getCardStr(card) << " ";
	}
	ss << "]";
	WARNLOG(ss.str());
}

bool CGameProcess::Init()
{
	CTableMgr::Instance()->LoadTable();
	CPart::Instance()->Init();
	//测试读取是否成功
	int32_t len = CTableMgr::Instance()->GetTable(2, false)->GetItems().size();
	if (len <= 0)
	{
		WARNLOG("麻将table加载失败");
		return false;
	}
	if (!boost::filesystem::exists(INI_FILENAME))
	{
		WARNLOG("游戏配置文件不存在");
		return false;
	}
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(INI_FILENAME, pt);
	intListParse(pt.get<string>("DEAL_GOOD.WIN", ""), m_dealGoodPrsWin);
	intListParse(pt.get<string>("DEAL_GOOD.LOSE", ""), m_dealGoodPrsLose);
	m_startAniTime = pt.get<float>("TIME.START_ANI_TIME", 0.0f);
	m_roundTime = pt.get<float>("TIME.ROUND_TIME", 0.0f);
	m_optionTime = pt.get<float>("TIME.OPTION_TIME", 0.0f);
	m_hostingTime = pt.get<float>("TIME.HOSTING_TIME", 0.0f);
	m_roundAfterTingTime = pt.get<float>("TIME.ROUND_AFTER_TING_TIME", 0.0f);
	//测试配置文件读取
	m_testEnable = pt.get<int>("TEST.ENABLE", 0) == 1;
	if (m_testEnable)
	{
		if (!boost::filesystem::exists(TEST_INI_FILENAME))
		{
			WARNLOG("测试配置文件不存在");
			return false;
		}
		boost::property_tree::ptree pt;
		boost::property_tree::read_ini(TEST_INI_FILENAME, pt);
		m_handCardsCfgEnable = pt.get<int>("HANDCARDS.ENABLE", 0) == 1;
		if (m_handCardsCfgEnable)
		{
			WARNLOG("启动手牌配置");
			vector<int32_t> handCardsCfg;
			intListParse(pt.get<string>("HANDCARDS.CHAIR_0", ""), handCardsCfg);
			m_handCardsCfg.push_back(handCardsCfg);
			handCardsCfg.clear();
			intListParse(pt.get<string>("HANDCARDS.CHAIR_1", ""), handCardsCfg);
			m_handCardsCfg.push_back(handCardsCfg);
		}
		m_ctrlType = (ControlType)pt.get<int>("RESULT.TYPE", 0);
		if (m_ctrlType != Ctrl_None)
		{
			string des[] = { "", "正常输赢", "系统赢", "系统输" };
			WARNLOG(des[m_ctrlType]);
		}
	}
	return true;
}

CGameProcess::~CGameProcess(void)
{

}

void CGameProcess::OnGameStart()
{
	if (m_ctrlType == Ctrl_None)
	{
		UpdateCtrlTypeByStorage();
	}
	m_startTime = chrono::system_clock::now();
	m_startTimeForReplay = (uint32_t)time(NULL);
	m_replay.clear();
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
		pServerUser->SetUserStatus(sPlaying);
		m_pTableFrame->BroadcastUserInfoToOther(pServerUser);
		m_pTableFrame->SendAllOtherUserInfoToUser(pServerUser);
		m_pTableFrame->BroadcastUserStatus(pServerUser, true);
		m_replay.addPlayer(pServerUser->GetUserId(), pServerUser->GetAccount(), pServerUser->GetUserScore(), pServerUser->GetChairId());
	}
	m_pTableFrame->SetGameStatus(GAME_STATUS_START);
	//洗牌
	for (uint32_t i = 0; i < 42; ++i)
	{
		if (i >= 9 && i < 27)
		{
			continue;
		}
		uint32_t num = i < 34 ? 4 : 1;
		for (uint32_t j = 0; j < num; ++j)
		{
			m_cards.push_back(CARDS[i]);
		}
	}
	std::random_shuffle(m_cards.begin(), m_cards.end());
	m_nBankerChairId = rand() % PLAYER_NUM;
	m_dices[0] = (rand() % 6) + 1;
	m_dices[1] = (rand() % 6) + 1;
	uint32_t startDir = (m_dices[0] + m_dices[1] - 1) % 4;
	uint32_t minDice = std::min(m_dices[0], m_dices[1]);
	m_cardWall[0] = 18 * startDir + minDice * 2;
	m_cardWall[1] = m_cardWall[0] - 1;
	//构造游戏玩家数据
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
		assert(pServerUser != nullptr);
		m_players[pServerUser->GetChairId()] = std::make_shared<CGamePlayer>(pServerUser);
		m_players[pServerUser->GetChairId()]->SetBanker(pServerUser->GetChairId() == m_nBankerChairId);
	}
	if (m_handCardsCfgEnable)
	{
		DealHandCardsByCfg();
	}
	else
	{
		DealHandCards();
	}
	for (int32_t i = 0; i < 26; ++i)
	{
		ReduceCardWallFront();
	}
	//庄家多发一张
	DealCardToUser(true, m_nBankerChairId);
	m_strRoundId = m_pTableFrame->GetNewRoundId();
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardsToString(m_players[i]->GetHandCards()), -1, ReplayOpType_HandCards, i, i);
	}
	//发送游戏开始消息
	CMD_S_GameStart sMsgGameStart;
	sMsgGameStart.set_roundid(m_strRoundId);
	sMsgGameStart.set_cellscore(m_pGameRoomInfo->floorScore);
	sMsgGameStart.set_leftcardnum(m_cards.size());
	sMsgGameStart.set_bankerchairid(m_nBankerChairId);
	for (uint32_t i = 0; i < 2; ++i)
	{
		sMsgGameStart.add_dices(m_dices[i]);
		sMsgGameStart.add_cardwall(m_cardWall[i]);
	}
	auto addPlayersToMsgGameStart = [&](uint32_t chairId)
	{
		sMsgGameStart.clear_players();
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
			Erqs::Player* pMjPlayer = sMsgGameStart.add_players();
			pMjPlayer->set_chairid(i);
			pMjPlayer->set_userid(m_players[i]->GetServerUser()->GetUserId());
			pMjPlayer->set_score(m_players[i]->GetServerUser()->GetUserScore());
			for (auto card : m_players[i]->GetHandCards())
			{
				if (m_players[chairId]->GetServerUser()->IsAndroidUser())
				{
					pMjPlayer->add_handcards(card);
				}
				else
				{
					pMjPlayer->add_handcards(i == chairId ? card : 0);
				}
			}
			pMjPlayer->set_hosting(m_players[i]->IsHosting());
			pMjPlayer->set_ting(m_players[i]->IsTing());
		}
	};
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		addPlayersToMsgGameStart(i);
		string content = sMsgGameStart.SerializeAsString();
		m_pTableFrame->SendTableData(i, SUB_S_GAME_START, (uint8_t*)content.data(), content.size());
	}
	//补花
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		ReplaceHua(i);
	}
	m_startRound = true;
	//更新游戏回合
	UpdateGameRound(m_nBankerChairId, false);
	//发送操作选项给其他玩家[闲家天听] 此处需要在更新回合之后，否则闲家天听选项值会被重置
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i != m_nBankerChairId)
		{
			if (SendOption(i, mj_ting))
			{
				WARNLOG("start tian ting timer");
				m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
				m_TimerTianTing = m_pLoopThread->getLoop()->runAfter(m_optionTime + m_startAniTime, boost::bind(&CGameProcess::OnTimerTianTing, this));
				m_tianTinging = true;
			}
		}
	}
}

void CGameProcess::UpdateCtrlTypeByStorage()
{
	bool androidExist = false;
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
		if (pServerUser->IsAndroidUser())
		{
			androidExist = true;
		}
	}
	if (!androidExist)
	{
		m_ctrlType = Ctrl_Normal;
	}
	else
	{
		tagStorageInfo storageInfo = { 0 };
		m_pTableFrame->GetStorageScore(storageInfo);
		if (storageInfo.lEndStorage < storageInfo.lLowlimit)
		{
			WARNLOG("当前库存:" + to_string(storageInfo.lEndStorage) + "低于下限，输赢控制:系统赢");
			m_ctrlType = Ctrl_SystemWin;
		}
		else if (storageInfo.lEndStorage > storageInfo.lUplimit)
		{
			WARNLOG("当前库存:" + to_string(storageInfo.lEndStorage) + "高于上限，输赢控制:系统输");
			m_ctrlType = Ctrl_SystemLose;
		}
		else
		{
			WARNLOG("当前库存:" + to_string(storageInfo.lEndStorage) + "正常范围，输赢控制:正常");
			m_ctrlType = Ctrl_Normal;
		}
		CMD_S_Android_AiLv sMsgAndroidAiLv;
		sMsgAndroidAiLv.set_lv(m_ctrlType);
		for (int i = 0; i < PLAYER_NUM; ++i)
		{
			shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
			if (pServerUser->IsAndroidUser())
			{
				string content = sMsgAndroidAiLv.SerializeAsString();
				m_pTableFrame->SendTableData(i, SUB_S_ANDROID_AI_LV, (uint8_t*)content.data(), content.size());
			}
		}
	}

}
void CGameProcess::WriteUserScore(uint32_t chairid)
{

    tagScoreInfo scoreInfo ;

    //scoreInfo.addScore=
    //写入玩家积分
    //m_pTableFrame->WriteUserScore(&scoreInfo, 1, strRoundID_);
}
//游戏结束
bool CGameProcess::OnEventGameConclude(uint32_t dwChairID, uint8_t GETag)
{
	//发送结算消息
	CMD_S_GameEnd sMsgGameEnd;
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		Erqs::Player* pSMsgPlayer = sMsgGameEnd.add_players();
		pSMsgPlayer->set_chairid(i);
		pSMsgPlayer->set_userid(m_players[i]->GetServerUser()->GetUserId());
		if (m_players[i]->GetServerUser()->IsAndroidUser())
		{
			int64_t androidScore = m_players[i]->GetServerUser()->GetUserScore();
			if (i == dwChairID)
			{
				int64_t tax = m_pTableFrame->CalculateRevenue(m_winScore);
				androidScore += (m_winScore - tax);
				m_pTableFrame->UpdateStorageScore(m_winScore - tax);
			}
			else
			{
				androidScore -= m_winScore;
				m_pTableFrame->UpdateStorageScore(-m_winScore);
			}
			pSMsgPlayer->set_score(androidScore);
		}
		else
		{
			pSMsgPlayer->set_score(m_players[i]->GetServerUser()->GetUserScore());
		}
		for (auto card : m_players[i]->GetHandCards())
		{
			pSMsgPlayer->add_handcards(card);
		}
		for (auto card : m_players[i]->GetOutCards())
		{
			pSMsgPlayer->add_outcards(card);
		}
		for (auto cpg : m_players[i]->GetCpgs())
		{
			Erqs::ChiPengGang* pSMsgCpg = pSMsgPlayer->add_cpgs();
			pSMsgCpg->set_type(cpg.type);
			pSMsgCpg->set_targetcard(cpg.targetCard);
			pSMsgCpg->set_targetchair(cpg.targetChair);
			for (auto card : cpg.cards)
			{
				pSMsgCpg->add_cards(card);
			}
		}
		for (auto card : m_players[i]->GetHuaCards())
		{
			pSMsgPlayer->add_huacards(card);
		}
		pSMsgPlayer->set_hosting(m_players[i]->IsHosting());
		pSMsgPlayer->set_ting(m_players[i]->IsTing());
		for (auto tingInfo : m_players[i]->GetTingInfos())
		{
			Erqs::MsgHuCard* pSMsgHuCard = pSMsgPlayer->add_hucards();
			pSMsgHuCard->set_card(tingInfo.card);
			pSMsgHuCard->set_times(tingInfo.compose.times);
		}
	}
	if (dwChairID != GAME_INVALID_CHAIR)
	{
		for (auto composeType : m_players[dwChairID]->GetHuCompose().types)
		{
			sMsgGameEnd.add_composetypes(composeType);
		}
		uint32_t times = m_players[dwChairID]->GetHuCompose().times;
		times = times > 200 ? 200 : times;
		sMsgGameEnd.set_times(times);
		sMsgGameEnd.set_iszimo(m_targetInfo.card == GAME_INVALID_CARD);
	}
	sMsgGameEnd.set_winchairid(dwChairID);
	string content = sMsgGameEnd.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, SUB_S_GAME_END, (uint8_t*)content.data(), content.size());



	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, "", -1, dwChairID == GAME_INVALID_CHAIR ? ReplayOpType_NoHu : ReplayOpType_Hu, dwChairID, dwChairID);
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i == dwChairID)
		{
			int64_t tax = m_pTableFrame->CalculateRevenue(m_winScore);
			m_replay.addResult(i, i, m_winScore, m_winScore - tax, CardsToString(m_players[i]->GetHandCards()), i == m_nBankerChairId);
		}
		else
		{
			m_replay.addResult(i, i, m_winScore, -m_winScore, CardsToString(m_players[i]->GetHandCards()), i == m_nBankerChairId);
		}
	}
	m_replay.gameinfoid = m_pTableFrame->GetNewRoundId();
	m_replay.roomname = m_pGameRoomInfo->roomName;
	m_replay.cellscore = m_pGameRoomInfo->floorScore;
	m_pTableFrame->SaveReplay(m_replay);
	m_pTableFrame->ConcludeGame(GAME_STATUS_END);
	m_pTableFrame->SetGameStatus(GAME_STATUS_END);
	ClearGameData();
	return true;
}

bool CGameProcess::OnEventGameScene(uint32_t dwChairID, bool bIsLookUser)
{
	bool oldHosting = m_players[dwChairID]->IsHosting();
	m_players[dwChairID]->SetHosting(false);
	//重连后,系统自动取消托管
	if (oldHosting)
	{
		OnHosting(dwChairID);
	}
	//广播玩家消息
	shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(dwChairID);
	assert(pServerUser);
	pServerUser->SetUserStatus(sPlaying);
	m_pTableFrame->SendAllOtherUserInfoToUser(pServerUser);
	m_pTableFrame->BroadcastUserStatus(pServerUser, true);
	//发送游戏场景消息
	CMD_S_GameScenePlay sMsgGameScenePlay;
	sMsgGameScenePlay.set_roundid(m_strRoundId);
	sMsgGameScenePlay.set_cellscore(m_pGameRoomInfo->floorScore);
	sMsgGameScenePlay.set_leftcardnum(m_cards.size());
	sMsgGameScenePlay.set_bankerchairid(m_nBankerChairId);
	for (uint32_t i = 0; i < 2; ++i)
	{
		sMsgGameScenePlay.add_dices(m_dices[i]);
		sMsgGameScenePlay.add_cardwall(m_cardWall[i]);
	}
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		Erqs::Player* pSMsgPlayer = sMsgGameScenePlay.add_players();
		pSMsgPlayer->set_chairid(i);
		pSMsgPlayer->set_userid(m_players[i]->GetServerUser()->GetUserId());
		pSMsgPlayer->set_score(m_players[i]->GetServerUser()->GetUserScore());
		for (auto card : m_players[i]->GetHandCards())
		{
			if (pServerUser->IsAndroidUser())
			{
				pSMsgPlayer->add_handcards(card);
			}
			else
			{
				pSMsgPlayer->add_handcards(i == dwChairID ? card : 0);
			}
		}
		for (auto card : m_players[i]->GetOutCards())
		{
			pSMsgPlayer->add_outcards(card);
		}
		for (auto cpg : m_players[i]->GetCpgs())
		{
			Erqs::ChiPengGang* pSMsgCpg = pSMsgPlayer->add_cpgs();
			pSMsgCpg->set_type(cpg.type);
			pSMsgCpg->set_targetcard(cpg.targetCard);
			pSMsgCpg->set_targetchair(cpg.targetChair);
			for (auto card : cpg.cards)
			{
				if (pServerUser->IsAndroidUser())
				{
					pSMsgCpg->add_cards(card);
				}
				else
				{
					if (cpg.type == mj_an_gang && i != dwChairID)
					{
						pSMsgCpg->add_cards(INVISIBLE_CARD);
					}
					else
					{
						pSMsgCpg->add_cards(card);
					}
				}
			}
		}
		for (auto card : m_players[i]->GetHuaCards())
		{
			pSMsgPlayer->add_huacards(card);
		}
		pSMsgPlayer->set_hosting(m_players[i]->IsHosting());
		pSMsgPlayer->set_ting(m_players[i]->IsTing());
		if (i == dwChairID)
		{
			for (auto tingInfo : m_players[i]->GetTingInfos())
			{
				Erqs::MsgHuCard* pSMsgHuCard = pSMsgPlayer->add_hucards();
				pSMsgHuCard->set_card(tingInfo.card);
				pSMsgHuCard->set_times(tingInfo.compose.times);
			}
		}
	}
	string content = sMsgGameScenePlay.SerializeAsString();
	m_pTableFrame->SendTableData(dwChairID, SUB_SC_GAMESCENE_PLAY, (uint8_t*)content.data(), content.size());
	//发送回合消息
	CMD_S_GameRound sMsgGameRound;
	sMsgGameRound.set_chairid(m_nCurRoundChairId);
	sMsgGameRound.set_roundtime(m_roundTime);
	for (uint32_t i = 0; i < 2; ++i)
	{
		sMsgGameRound.add_cardwall(m_cardWall[i]);
	}
	sMsgGameRound.set_card(GAME_INVALID_CARD);
	content = sMsgGameRound.SerializeAsString();
	m_pTableFrame->SendTableData(dwChairID, SUB_S_GAME_ROUND, (uint8_t*)content.data(), content.size());
	return true;
}

bool CGameProcess::OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t * data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> pServerUserItem = m_pTableFrame->GetTableUserItem(chairid);
    if(!pServerUserItem)
    {
        return false;
    }
	CancelHosting(chairid);
	try
	{
		switch (subid)
		{
		case SUB_C_OUT_CARD:
			return onMsgOutCard(chairid, data, datasize);
		case SUB_C_CHI:
			return onMsgChi(chairid, data, datasize);
		case SUB_C_PENG:
			return onMsgPeng(chairid, data, datasize);
		case SUB_C_GANG:
			return onMsgGang(chairid, data, datasize);
		case SUB_C_PASS:
			return onMsgPass(chairid, data, datasize);
		case SUB_C_TING:
			return onMsgTing(chairid, data, datasize);
		case SUB_C_HU:
			return onMsgHu(chairid, data, datasize);
		case SUB_C_HOSTING:
			return onMsgHosting(chairid, data, datasize);
			//测试功能
		case SUB_C_LETF_CARDS:
			return onMsgLeftCards(chairid, data, datasize);
		case SUB_C_REPLACE_CARD:
			return onMsgReplaceCard(chairid, data, datasize);
		default:
			LOG_ERROR << "unknow subid " << subid;
		}
	}
	catch (exception & exp)
	{
		//        openLog("err subid:%d",subid);
	}

	return false;
}

bool CGameProcess::onMsgOutCard(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (m_tianTinging)
	{
		if (chairid == m_nCurRoundChairId && !IsGameMsgCacheExist(chairid))
		{
			uint8_t* newData = new uint8_t[datasize];
			memcpy(newData, data, datasize);
			m_gameMsgCache.push_back({ chairid , SUB_C_OUT_CARD , newData , datasize });
			m_pLoopThread->getLoop()->cancel(m_TimerRound);
			return true;
		}
		else
		{
			WARNLOG("");
			return false;
		}
	}
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	CMD_C_OutCard cMsgOutCard;
	if (!cMsgOutCard.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
    //判断牌是否有效
	if (!isValidCard(cMsgOutCard.card()))
	{
		WARNLOG(cMsgOutCard.card());
		return false;
	}
	if (!OnOutCard(chairid, cMsgOutCard.card()))
	{
		WARNLOG("");
		return false;
	}
	return true;
}

bool CGameProcess::onMsgChi(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	if (NEXT_CHAIR(m_nCurRoundChairId) != chairid)
	{
		WARNLOG("");
		return false;
	}
	CMD_C_Chi cMsgChi;
	if (!cMsgChi.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
	//玩家吃牌
	vector<int32_t> cards;
	for (int i = 0; i < cMsgChi.cards_size(); ++i)
	{
		if (!isValidCard(cMsgChi.cards(i)))
		{
			WARNLOG(cMsgChi.cards(i));
			return false;
		}
		cards.push_back(cMsgChi.cards(i));
	}
	if (!m_players[chairid]->Chi(cards))
	{
		WARNLOG("");
		return false;
	}
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_gameOpCnt++;
	m_lastAction = action_chi;
	//被吃玩家删除被吃牌
	CPG cpg = m_players[chairid]->GetCpgs().back();
	m_players[m_nCurRoundChairId]->RemoveOutCard(cpg.targetCard);
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardsToString(cards), -1, ReplayOpType_Chi, chairid, chairid);
	//广播吃牌
	CMD_S_Chi sMsgChi;
	sMsgChi.set_chairid(chairid);
	Erqs::ChiPengGang* pSMsgCpg = sMsgChi.mutable_cpg();
	pSMsgCpg->set_type(cpg.type);
	pSMsgCpg->set_targetcard(cpg.targetCard);
	pSMsgCpg->set_targetchair(cpg.targetChair);
	for (auto card : cpg.cards)
	{
		pSMsgCpg->add_cards(card);
	}
	string content = sMsgChi.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, SUB_S_CHI, (uint8_t*)content.data(), content.size());
	//更新游戏回合
	UpdateGameRound(chairid, false);
	return true;
}

bool CGameProcess::onMsgPeng(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	if (m_nCurRoundChairId == chairid)
	{
		WARNLOG("");
		return false;
	}
	//玩家碰牌
	if (!m_players[chairid]->Peng())
	{
		WARNLOG("");
		return false;
	}
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_gameOpCnt++;
	m_lastAction = action_peng;
	//被碰玩家删除被碰牌
	CPG cpg = m_players[chairid]->GetCpgs().back();
	m_players[m_nCurRoundChairId]->RemoveOutCard(cpg.targetCard);
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardsToString(cpg.cards), -1, ReplayOpType_Peng, chairid, chairid);
	//广播碰牌
	CMD_S_Peng sMsgPeng;
	sMsgPeng.set_chairid(chairid);
	Erqs::ChiPengGang* pSMsgCpg = sMsgPeng.mutable_cpg();
	pSMsgCpg->set_type(cpg.type);
	pSMsgCpg->set_targetcard(cpg.targetCard);
	pSMsgCpg->set_targetchair(cpg.targetChair);
	for (auto card : cpg.cards)
	{
		pSMsgCpg->add_cards(card);
	}
	string content = sMsgPeng.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, SUB_S_PENG, (uint8_t*)content.data(), content.size());
	//更新游戏回合
	UpdateGameRound(chairid, false);
	return true;
}

bool CGameProcess::onMsgGang(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (m_cards.size() <= 0)
	{
		WARNLOG("");
		return false;
	}
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	CMD_C_Gang cMsgGang;
	if (!cMsgGang.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
	//玩家杠牌
	vector<int32_t> cards;
	for (int i = 0; i < cMsgGang.cards_size(); ++i)
	{
		if (!isValidCard(cMsgGang.cards(i)))
		{
			WARNLOG(cMsgGang.cards(i));
			return false;
		}
		cards.push_back(cMsgGang.cards(i));
	}
	if (!m_players[chairid]->Gang(cards))
	{
		WARNLOG("");
		return false;
	}
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_gameOpCnt++;
	m_lastAction = action_gang;
	int32_t cpgIdx = m_players[chairid]->GetCpgIdx(cards);
	if (cpgIdx == -1)
	{
		WARNLOG("");
		return false;
	}
	auto cpg = m_players[chairid]->GetCpgs().begin() + cpgIdx;
	//被杠玩家删除被杠牌
	if (cpg->type == mj_dian_gang)
	{
		m_players[m_nCurRoundChairId]->RemoveOutCard(cpg->targetCard);
	}
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardsToString(cpg->cards), -1, cpg->type == mj_an_gang ? ReplayOpType_AnGang : ReplayOpType_MingGang, chairid, chairid);
	//广播杠牌
	CMD_S_Gang sMsgGang;
	sMsgGang.set_chairid(chairid);
	Erqs::ChiPengGang* pSMsgCpg = sMsgGang.mutable_cpg();
	pSMsgCpg->set_type(cpg->type);
	pSMsgCpg->set_targetcard(cpg->targetCard);
	pSMsgCpg->set_targetchair(cpg->targetChair);
	auto addCardToMsgCpg = [&](uint32_t playerChairId)
	{
		pSMsgCpg->clear_cards();
		for (auto card : cpg->cards)
		{
			if (m_players[playerChairId]->GetServerUser()->IsAndroidUser())
			{
				pSMsgCpg->add_cards(card);
			}
			else
			{
				if (cpg->type == mj_an_gang && playerChairId != chairid)
				{
					pSMsgCpg->add_cards(INVISIBLE_CARD);
				}
				else
				{
					pSMsgCpg->add_cards(card);
				}
			}
		}
	};
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		addCardToMsgCpg(i);
		string content = sMsgGang.SerializeAsString();
		m_pTableFrame->SendTableData(i, SUB_S_GANG, (uint8_t*)content.data(), content.size());
	}
	if (cpg->type == mj_jia_gang)
	{
		SetTargetInfo(cpg->targetCard, target_jia_gang, chairid);
		//其他玩家发送操作选项
		bool hasOption = false;
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
			if (i != chairid)
			{
				if (SendOption(i, mj_hu))
				{
					hasOption = true;
				}
			}
			else
			{
				m_players[i]->ResetOption();
			}
		}
		//无操作选项，则更新游戏回合
		if (!hasOption)
		{
			//更新游戏回合
			UpdateGameRound(chairid, true);
		}
		else
		{
			m_nOptionStartTime = (uint32_t)time(nullptr);
			m_pLoopThread->getLoop()->cancel(m_TimerOption);
			int32_t optionChair = NEXT_CHAIR(chairid);
			int32_t time = m_players[optionChair]->IsHosting() ? m_hostingTime : m_optionTime;
			m_TimerOption = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CGameProcess::OnTimerOption, this));
		}
	}
	else
	{
		//更新游戏回合
		UpdateGameRound(chairid, true);
	}
	return true;
}

bool CGameProcess::onMsgPass(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	//是否有选项
	if (!m_players[chairid]->HasOption())
	{
		WARNLOG("");
		return false;
	}
	//玩家过
	m_players[chairid]->ResetOption();
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	//过类型
	if (chairid != m_nCurRoundChairId)
	{
		//闲家天听过
		if (m_tianTinging)
		{
			OnTimerTianTing();
		}
		else
		{
			UpdateGameRound(NEXT_CHAIR(m_nCurRoundChairId), true);
		}
	}
	else
	{
		//当前回合者过，不处理
	}
	return true;
}

bool CGameProcess::onMsgTing(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	CMD_C_Ting cMsgTing;
	if (!cMsgTing.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
	if (!m_players[chairid]->Ting(cMsgTing.card()))
	{
		WARNLOG("");
		return false;
	}
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	bool isTianTing = m_gameOpCnt == 0;
	m_players[chairid]->AddHuType(isTianTing ? hu_tianting : hu_baoting);
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, "", -1, ReplayOpType_Ting, chairid, chairid);
	//广播听
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		CMD_S_Ting sMsgTing;
		sMsgTing.set_chairid(chairid);
		sMsgTing.set_card(cMsgTing.card());
		sMsgTing.set_tianting(isTianTing);
		if (i == chairid)
		{
			for (auto tingInfo : m_players[i]->GetTingInfos())
			{
				Erqs::MsgHuCard* pSMsgHuCard = sMsgTing.add_hucards();
				pSMsgHuCard->set_card(tingInfo.card);
				pSMsgHuCard->set_times(tingInfo.compose.times);
			}
		}
		string content = sMsgTing.SerializeAsString();
		m_pTableFrame->SendTableData(i, SUB_S_TING, (uint8_t*)content.data(), content.size());
	}
	//非闲家天听
	if (cMsgTing.card() != GAME_INVALID_CARD)
	{
		if (!isValidCard(cMsgTing.card()))
		{
			WARNLOG(cMsgTing.card());
			return false;
		}
		if (m_nCurRoundChairId != chairid)
		{
			WARNLOG("");
			return false;
		}
		m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardToString(cMsgTing.card()), -1, ReplayOpType_OutCard, chairid, chairid);
		m_gameOpCnt++; //出牌操作
		m_lastAction = action_out_card;
		//更新目标信息
		SetTargetInfo(cMsgTing.card(), target_out, chairid);
		//其他玩家发送操作选项
		bool hasOption = false;
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
			if (i != chairid)
			{
				if (SendOption(i, mj_left_chi | mj_peng | mj_dian_gang | mj_hu))
				{
					hasOption = true;
				}
			}
			else
			{
				m_players[i]->ResetOption();
			}
		}
		//无操作选项，则更新游戏回合
		if (!hasOption)
		{
			UpdateGameRound(NEXT_CHAIR(chairid), true);
		}
		else
		{
			m_nOptionStartTime = (uint32_t)time(nullptr);
			m_pLoopThread->getLoop()->cancel(m_TimerOption);
			int32_t optionChair = NEXT_CHAIR(chairid);
			int32_t time = m_players[optionChair]->IsHosting() ? m_hostingTime : m_optionTime;
			m_TimerOption = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CGameProcess::OnTimerOption, this));
		}
	}
	//闲家天听
	else if (chairid != m_nBankerChairId && isTianTing && cMsgTing.card() == GAME_INVALID_CARD)
	{
		OnTimerTianTing();
	}
	else
	{
		WARNLOG("");
		return false;
	}
	return true;
}

bool CGameProcess::onMsgHu(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	if (!OnHu(chairid))
	{
		WARNLOG("");
		return false;
	}
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	return true;
}

bool CGameProcess::onMsgHosting(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	CMD_C_Hosting cMsgHosting;
	if (!cMsgHosting.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
	m_players[chairid]->SetHosting(cMsgHosting.hosting());
	OnHosting(chairid);
	return true;
}

bool CGameProcess::OnUserEnter(int64_t dwUserID, bool bIsLookUser)
{
	shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetUserItemByUserId(dwUserID);
	if (!pServerUser)
	{
		WARNLOG("");
		return false;
	}
	uint32_t chairId = pServerUser->GetChairId();
	if (chairId >= PLAYER_NUM)
	{
		LOG_ERROR << "chairId=" << chairId << " player_num=" << PLAYER_NUM;
		return false;
	}
	//断线重连
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START && m_players[chairId] != nullptr)
	{
		return OnEventGameScene(chairId, bIsLookUser);
	}
	//正常进入
	if (m_pTableFrame->GetPlayerCount(true) == 1)
	{
		//第一个进入房间的也必须是真人
		assert(m_pTableFrame->GetPlayerCount(false) == 1);
	}
	pServerUser->SetUserStatus(sReady);
	//清理不满足条件玩家
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
		if (pServerUser)
		{
			if (pServerUser->GetUserStatus() == sOffline)
			{
				m_pTableFrame->ClearTableUser(i, true, true);
			}
			else if (pServerUser->GetUserScore() < m_pGameRoomInfo->enterMinScore)
			{
				pServerUser->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_SCORENOENOUGH);
			}
		}
	}
	if (m_pTableFrame->GetPlayerCount(true) == PLAYER_NUM)
	{
		OnGameStart();
	}

	return true;
}

bool CGameProcess::OnUserReady(int64_t dwUserID, bool bIsLookUser)
{
	return true;
}

bool CGameProcess::OnUserLeft(int64_t dwUserID, bool bIsLookUser)
{
	shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetUserItemByUserId(dwUserID);
	if (!pServerUser)
	{
		LOG_INFO << "OnUserLeft of userid:" << dwUserID << ", GetUserItemByUserId failed, pServerUser==NULL";
		return false;
	}
	//游戏开始了不能离开游戏
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START)
	{
		return false;
	}
	pServerUser->SetUserStatus(sOffline);
	m_pTableFrame->ClearTableUser(pServerUser->GetChairId(), true, true);
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT || m_pTableFrame->GetGameStatus() == GAME_STATUS_FREE)
	{
		m_pTableFrame->BroadcastUserStatus(pServerUser, true);
	}
	return true;
}

bool CGameProcess::CanJoinTable(shared_ptr<CServerUserItem> & pUser)
{
	if (m_pTableFrame->GetGameStatus() < GAME_STATUS_START)
	{
		//达到游戏人数要求禁入游戏
		if (m_pTableFrame->GetPlayerCount(true) >= PLAYER_NUM) {
			return false;
		}
		if (pUser->IsAndroidUser()) //机器人
		{

		}
		else
		{
			//真实玩家防止作弊检查
//            for (int i = 0; i < PLAYER_NUM; ++i) {
//                shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetTableUserItem(i);
//                if(pServerUser && !pServerUser->IsAndroidUser())
//                {
//                    if (CheckSubnetInetIp(userIp, pServerUser->GetIp())) {
//                        printf("\n\n--- *** &&&&&&&&&&&&&&&&&&& tableID[%d]检测到同一个网段I玩家IP，不能进入房间\n",
//                            m_pTableFrame->GetTableId());
//                        return false;
//                    }
//                }
//            }
		}
		return true;
	}
	else if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START)
	{
		//游戏进行状态，处理断线重连，玩家信息必须存在
		shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetUserItemByUserId(pUser->GetUserId());
		return pServerUser != nullptr;
	}
	else
	{
		return false;
	}
}

bool CGameProcess::CanLeftTable(int64_t userId)
{
	shared_ptr<CServerUserItem> pServerUser = m_pTableFrame->GetUserItemByUserId(userId);
	if (!pServerUser)
	{
		LOG_INFO << "OnUserLeft of userid:" << userId << ", GetUserItemByUserId failed, pServerUser==NULL";
		return true;
	}
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_START)
	{
		return false;
	}
	return true;
}

bool CGameProcess::SetTableFrame(shared_ptr<ITableFrame> & pTableFrame)
{
	if (!pTableFrame)
	{
		return false;
	}
	m_pTableFrame = pTableFrame;
	m_pGameRoomInfo = m_pTableFrame->GetGameRoomInfo();
	m_pLoopThread = m_pTableFrame->GetLoopThread();
	return true;
}

void CGameProcess::ClearGameData()
{
	if (m_pLoopThread != nullptr)
	{
		m_pLoopThread->getLoop()->cancel(m_TimerOption);
		m_pLoopThread->getLoop()->cancel(m_TimerRound);
		m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
	}
	if (m_pTableFrame != nullptr)
	{
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
            shared_ptr<CServerUserItem> pServerUserItem = m_pTableFrame->GetTableUserItem(i);
			if (pServerUserItem)
			{

				pServerUserItem->SetUserStatus(sOffline);
				m_pTableFrame->ClearTableUser(i, true, true, ERROR_ENTERROOM_LONGTIME_NOOP);
			}
			m_players[i] = nullptr;
		}
		m_pTableFrame->SetGameStatus(GAME_STATUS_INIT);
	}
	else
	{
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
			m_players[i] = nullptr;
		}
	}
	m_cards.clear();
	m_strRoundId = "";
	m_nBankerChairId = 0;
	m_nCurRoundChairId = 0;
	m_nRoundStartTime = 0;
	m_nOptionStartTime = 0;
	m_gameOpCnt = 0;
	m_tianTinging = false;
	m_lastAction = action_invalid;
	m_matchTime = 0;
	m_dices[0] = m_dices[1] = 0;
	m_cardWall[0] = m_cardWall[1] = 0;
	m_isGameFlow = false;
	m_winScore = 0;
	m_startRound = false;
	m_ctrlType = Ctrl_None;
	m_handCardsCfgEnable = false;
	m_startTimeForReplay = 0;
	SetTargetInfo(GAME_INVALID_CARD, target_invalid, GAME_INVALID_CHAIR);
}

void CGameProcess::UpdateGameRound(uint32_t nChairId, bool bAddHandCard)
{
	m_nCurRoundChairId = nChairId;
	int32_t handCardToAdd = 0;
	if (bAddHandCard)
	{
		handCardToAdd = DealCardToUser(m_lastAction != action_gang, nChairId);
		//流局
		if (handCardToAdd == GAME_INVALID_CARD)
		{
			m_isGameFlow = true;
			OnEventGameConclude(GAME_INVALID_CHAIR, GER_NORMAL_NO_HU);
			return;
		}
		m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardToString(handCardToAdd), -1, ReplayOpType_AddCard, nChairId, nChairId);
		m_gameOpCnt++;
		m_players[m_nCurRoundChairId]->IncreaseAddHandCnt();
	}
	m_nRoundStartTime = (uint32_t)time(nullptr);
	CMD_S_GameRound sMsgGameRound;
	sMsgGameRound.set_chairid(m_nCurRoundChairId);
	sMsgGameRound.set_roundtime(m_roundTime);
	for (uint32_t i = 0; i < 2; ++i)
	{
		sMsgGameRound.add_cardwall(m_cardWall[i]);
	}
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		//GAME_INVALID_CARD:未加手牌, INVISIBLE_CARD:其他玩家加手牌 正常牌值:自己加手牌
		if (bAddHandCard)
		{
			if (m_players[i]->GetServerUser()->IsAndroidUser())
			{
				sMsgGameRound.set_card(handCardToAdd);
			}
			else
			{
				sMsgGameRound.set_card(i == m_nCurRoundChairId ? handCardToAdd : INVISIBLE_CARD);
			}
		}
		else
		{
			sMsgGameRound.set_card(GAME_INVALID_CARD);
		}
		string content = sMsgGameRound.SerializeAsString();
		m_pTableFrame->SendTableData(i, SUB_S_GAME_ROUND, (uint8_t*)content.data(), content.size());
	}
	//补花
	bool replaceResult = ReplaceHua(nChairId);
	if (m_isGameFlow)
	{
		OnEventGameConclude(GAME_INVALID_CHAIR, GER_NORMAL_NO_HU);
		return;
	}
	if (replaceResult)
	{
		UpdateGameRound(nChairId, false);
		return;
	}
	SetTargetInfo(GAME_INVALID_CARD, target_invalid, GAME_INVALID_CHAIR);
	//发送操作选项给当前回合玩家
	bool hasOption = false;
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i == m_nCurRoundChairId)
		{
			int32_t updateOption = mj_jia_gang | mj_an_gang | mj_ting | mj_hu;
			if (m_lastAction == action_chi || m_lastAction == action_peng)
			{
				updateOption = mj_ting;
			}
			if (SendOption(i, updateOption))
			{
				hasOption = true;
			}
		}
		else
		{
			m_players[i]->ResetOption();
		}
	}
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	int32_t time = m_roundTime;
	if (m_players[m_nCurRoundChairId]->IsTing())
	{
		time = m_roundAfterTingTime;
	}
	else if (m_players[m_nCurRoundChairId]->IsHosting())
	{
		time = m_hostingTime;
	}
	if (hasOption)
	{
		time += m_optionTime;
	}
	if (m_startRound)
	{
		time += m_startAniTime;
	}
	m_TimerRound = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CGameProcess::OnTimerRound, this));
	m_startRound = false;
}

void CGameProcess::OnTimerRound()
{
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT)
	{
		WARNLOG("game end, but timer is not canceled");
		return;
	}
	//回合超时操作:胡牌,出牌
	if (m_players[m_nCurRoundChairId]->HasOptionType(mj_hu))
	{
		OnHu(m_nCurRoundChairId);
	}
	else
	{
		if (m_players[m_nCurRoundChairId]->IsTing())
		{
			int32_t card = m_players[m_nCurRoundChairId]->GetHandCards().back();
			OnOutCard(m_nCurRoundChairId, card);
		}
		else
		{
			bool oldHosting = m_players[m_nCurRoundChairId]->IsHosting();
			m_players[m_nCurRoundChairId]->Timeout();
			int32_t card = m_players[m_nCurRoundChairId]->GetHandCards().back();
			OnOutCard(m_nCurRoundChairId, card);
			//超时后系统自动托管，广播消息
			if (!oldHosting && m_players[m_nCurRoundChairId]->IsHosting())
			{
				OnHosting(m_nCurRoundChairId);
			}
		}
	}
}

void CGameProcess::OnTimerOption()
{
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	if (m_pTableFrame->GetGameStatus() == GAME_STATUS_INIT)
	{
		WARNLOG("game end, but timer is not canceled");
		return;
	}
	//选项超时操作:胡牌,更新游戏回合
	for (int i = 0; i < PLAYER_NUM; ++i)
	{
		if (i != m_nCurRoundChairId)
		{
			if (m_players[i]->HasOptionType(mj_hu))
			{
				OnHu(i);
				//2人麻将 不处理多人胡牌，直接返回
				return;
			}
		}
	}
	UpdateGameRound(NEXT_CHAIR(m_nCurRoundChairId), true);
}

void CGameProcess::OnTimerTianTing()
{
	m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
	m_tianTinging = false;
	for (auto msg : m_gameMsgCache)
	{
		OnGameMessage(msg.chairid, msg.subid, msg.data, msg.datasize);
		delete msg.data;
		msg.data = nullptr;
	}
	m_gameMsgCache.clear();
}

bool CGameProcess::OnOutCard(uint32_t chairid, int32_t card)
{
    //玩家是否是当前操作玩家
	if (chairid != m_nCurRoundChairId)
	{
		WARNLOG(chairid);
		WARNLOG(card);
		return false;
	}
    //玩家出牌,把牌放到出牌列表
	if (!m_players[chairid]->OutCard(card))
	{
		WARNLOG(card);
		return false;
	}
	m_gameOpCnt++;
	m_lastAction = action_out_card;
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardToString(card), -1, ReplayOpType_OutCard, chairid, chairid);
	//广播出牌
	CMD_S_OutCard sMsgOutCard;
	sMsgOutCard.set_chairid(chairid);
	sMsgOutCard.set_card(card);
	string content = sMsgOutCard.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, SUB_S_OUT_CARD, (uint8_t*)content.data(), content.size());
	//更新目标信息
	SetTargetInfo(card, target_out, chairid);
	//其他玩家发送操作选项
	bool hasOption = false;
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i != chairid)
		{
			if (SendOption(i, mj_left_chi | mj_peng | mj_dian_gang | mj_hu))
			{
				hasOption = true;
			}
		}
		else
		{
			m_players[i]->ResetOption();
		}
	}
	//无操作选项，则更新游戏回合
	if (!hasOption)
	{
		UpdateGameRound(NEXT_CHAIR(m_nCurRoundChairId), true);
	}
	else
	{
		m_nOptionStartTime = (uint32_t)time(nullptr);
		m_pLoopThread->getLoop()->cancel(m_TimerOption);
		int32_t optionChair = NEXT_CHAIR(m_nCurRoundChairId);
		int32_t time = m_players[optionChair]->IsHosting() ? m_hostingTime : m_optionTime;
		m_TimerOption = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CGameProcess::OnTimerOption, this));
	}
	return true;
}

bool CGameProcess::OnHu(uint32_t nChairId)
{
	if (!m_players[nChairId]->CanHu(m_targetInfo.card))
	{
		WARNLOG("");
		return false;
	}
	uint32_t huCard = m_targetInfo.card;
	uint32_t gameCpgs = 0;
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		gameCpgs += m_players[i]->GetCpgs().size();
	}
	//天胡=[庄家 && 发完牌 && 庄家无暗杠]
	if (m_gameOpCnt == 0 && nChairId == m_nBankerChairId)
	{
		m_players[nChairId]->AddHuType(hu_tianhu);
	}
	//地胡=[闲家 && 第一次摸牌 && 游戏无吃碰杠 && 自摸]
	else if (nChairId != m_nBankerChairId && m_players[nChairId]->GetAddCardCnt() == 1 && gameCpgs == 0 && m_targetInfo.card == GAME_INVALID_CARD)
	{
		m_players[nChairId]->AddHuType(hu_dihu);
	}
	//人胡=[闲家 && 庄家第一次打牌 && 庄家无暗杠 && 接炮]
	else if (m_gameOpCnt == 1 && nChairId != m_nBankerChairId && m_targetInfo.card != GAME_INVALID_CARD)
	{
		m_players[nChairId]->AddHuType(hu_renhu);
	}
	//自摸
	else if (m_targetInfo.card == GAME_INVALID_CARD)
	{
		m_players[nChairId]->AddHuType(hu_zimo);
	}
	if (m_targetInfo.card == GAME_INVALID_CARD)
	{
		huCard = m_players[nChairId]->GetHandCards().back();
		//胡绝张
		if (GetCardNumInTable(huCard) == 3)
		{
			m_players[nChairId]->AddHuType(hu_juezhang);
		}
		//妙手回春
		if (m_cards.size() == 0)
		{
			m_players[nChairId]->AddHuType(hu_miaoshouhuichun);
		}
		//杠上开花
		if (m_lastAction == action_gang)
		{
			m_players[nChairId]->AddHuType(hu_gangshangkaihua);
		}
	}
	else
	{
		//胡绝张
		if (GetCardNumInTable(huCard) == 4)
		{
			m_players[nChairId]->AddHuType(hu_juezhang);
		}
		//海底捞月
		if (m_cards.size() == 0)
		{
			m_players[nChairId]->AddHuType(hu_haidilaoyue);
		}
		//抢杠胡
		if (m_targetInfo.type == target_jia_gang)
		{
			m_players[nChairId]->AddHuType(hu_qiangganghu);
		}
	}
	m_players[nChairId]->Hu(m_targetInfo.card);
	//被胡玩家删除被胡牌
	if (m_targetInfo.card != GAME_INVALID_CARD)
	{
		if (m_targetInfo.type == target_jia_gang)
		{
			m_players[nChairId]->ChangeJiaGangToPeng(m_targetInfo.card);
		}
		else
		{
			m_players[m_targetInfo.chair]->RemoveOutCard(m_targetInfo.card);
		}
		m_players[nChairId]->AddHandCard(m_targetInfo.card);
	}
	//计算分数
	CalculateScore(nChairId);
	if (!OnEventGameConclude(nChairId, GER_NORMAL_HU))
	{
		WARNLOG("");
		return false;
	}
	return true;
}

void CGameProcess::OnHosting(uint32_t chairId)
{
	CMD_S_Hosting sMsgHosting;
	sMsgHosting.set_chairid(chairId);
	sMsgHosting.set_hosting(m_players[chairId]->IsHosting());
	string content = sMsgHosting.SerializeAsString();
	m_pTableFrame->SendTableData(INVALID_CHAIR, SUB_S_HOSTING, (uint8_t*)content.data(), content.size());
}

void CGameProcess::CancelHosting(uint32_t chairId)
{
	if (chairId < 0 || chairId >= PLAYER_NUM)
	{
		WARNLOG("");
		return;
	}

	m_players[chairId]->SetHosting(false);
}

bool CGameProcess::ReplaceHua(uint32_t nChairId)
{
	vector<int32_t> huaCards;
	m_players[nChairId]->SearchHuaCardsInHandCards(huaCards);
	if (huaCards.size() == 0)
	{
		return false;
	}
	m_lastAction = action_replace_hua;
	vector<int32_t> replaceCards;
	for (auto huaCard : huaCards)
	{
		int32_t replaceCard = DealCardToUser(true, nChairId);
		//流局
		if (replaceCard == GAME_INVALID_CARD)
		{
			m_isGameFlow = true;
			return false;
		}
		m_players[nChairId]->AddHuaCard(huaCard);
		replaceCards.push_back(replaceCard);
	}
	m_replay.addStep((int32_t)time(NULL) - m_startTimeForReplay, CardsToString(replaceCards), -1, ReplayOpType_ReplaceHua, nChairId, nChairId);
	CMD_S_ReplaceHua sMsgReplaceHua;
	sMsgReplaceHua.set_chairid(nChairId);
	for (auto huaCard : huaCards)
	{
		sMsgReplaceHua.add_huacards(huaCard);
	}
	auto addReplaceCardsToMsg = [&](uint32_t playerChair)
	{
		sMsgReplaceHua.clear_replacecards();
		for (auto replaceCard : replaceCards)
		{
			if (m_players[playerChair]->GetServerUser()->IsAndroidUser())
			{
				sMsgReplaceHua.add_replacecards(replaceCard);
			}
			else
			{
				sMsgReplaceHua.add_replacecards(playerChair == nChairId ? replaceCard : INVISIBLE_CARD);
			}
		}
	};
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		addReplaceCardsToMsg(i);
		string content = sMsgReplaceHua.SerializeAsString();
		m_pTableFrame->SendTableData(i, SUB_S_REPLACE_HUA, (uint8_t*)content.data(), content.size());
	}
	//递归补花
	ReplaceHua(nChairId);
	return true;
}

void CGameProcess::SetTargetInfo(int32_t card, TargetType type, int32_t chair)
{
	m_targetInfo.card = card;
	m_targetInfo.type = type;
	m_targetInfo.chair = chair;
}

//发送选项时，必须更新目标信息
bool CGameProcess::SendOption(uint32_t chair, int32_t updateOption)
{
	m_players[chair]->UpdateOption(m_targetInfo.card, m_targetInfo.chair, m_cards.size() > 0, updateOption);
	if (m_players[chair]->GetOption() == 0)
	{
		return false;
	}
	CMD_S_Options sMsgOptions;
	sMsgOptions.set_chairid(chair);
	sMsgOptions.set_option(m_players[chair]->GetOption());
	sMsgOptions.set_optiontime(m_roundTime); //[0=无效值，其他=有效值]
	for (auto cpg : m_players[chair]->GetOptionalCpgs())
	{
		Erqs::ChiPengGang* pSMsgCpg = sMsgOptions.add_optionalcpgs();
		pSMsgCpg->set_type(cpg.type);
		pSMsgCpg->set_targetcard(cpg.targetCard);
		pSMsgCpg->set_targetchair(cpg.targetChair);
		for (auto card : cpg.cards)
		{
			pSMsgCpg->add_cards(card);
		}
	}
	for (TingOptInfo tingOptInfo : m_players[chair]->GetTingOptInfos())
	{
		Erqs::MsgTingInfo* pSMsgTingInfos = sMsgOptions.add_tinginfos();
		pSMsgTingInfos->set_outcard(tingOptInfo.outCard);
		for (TingInfo tingInfo : tingOptInfo.tingInfos)
		{
			Erqs::MsgHuCard* pSMsgHuCard = pSMsgTingInfos->add_hucards();
			pSMsgHuCard->set_card(tingInfo.card);
			pSMsgHuCard->set_num(tingInfo.num);
			pSMsgHuCard->set_times(tingInfo.compose.times);
		}
	}
	string content = sMsgOptions.SerializeAsString();
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (m_players[i]->GetServerUser()->IsAndroidUser())
		{
			m_pTableFrame->SendTableData(i, SUB_S_OPTIONS, (uint8_t*)content.data(), content.size());
		}
		else if (i == chair)
		{
			m_pTableFrame->SendTableData(chair, SUB_S_OPTIONS, (uint8_t*)content.data(), content.size());
		}
	}
	return true;
}

uint32_t CGameProcess::GetCardNumInTable(uint32_t card)
{
	uint32_t num = 0;
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		for (auto playerCard : m_players[i]->GetOutCards())
		{
			if (playerCard == card)
			{
				num++;
			}
		}
		for (auto cpg : m_players[i]->GetCpgs())
		{
			if (!isGangType(cpg.type))
			{
				for (auto playerCard : cpg.cards)
				{
					if (playerCard == card)
					{
						num++;
					}
				}
			}
		}
	}
	return num;
}

void CGameProcess::CalculateScore(uint32_t winChair)
{
	uint32_t times = m_players[winChair]->GetHuCompose().times;
	times = times > 200 ? 200 : times;
	m_winScore = times * m_pGameRoomInfo->floorScore;
	uint32_t loseChair = NEXT_CHAIR(winChair);
	m_winScore = m_winScore > m_players[loseChair]->GetServerUser()->GetUserScore() ? m_players[loseChair]->GetServerUser()->GetUserScore() : m_winScore;
	//跑马灯
//	if ((m_lMarqueeMinScore != 0) && (m_winScore > m_lMarqueeMinScore))
//	{
//		int msgType = SMT_GLOBAL | SMT_SCROLL;
//		LOG_INFO << " >>> marquee SendGameMessage, wWinnerChairID:" << winChair << ", winScore:" << m_winScore;
//		m_pTableFrame->SendGameMessage(winChair, (uint8_t*)"", msgType, m_winScore);
//	}
	int64_t tax = m_pTableFrame->CalculateRevenue(m_winScore);
	vector<tagSpecialScoreInfo> scoreInfos;
	for (uint32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (!m_pTableFrame->IsAndroidUser(i))
		{
			shared_ptr<CServerUserItem> userItem = m_pTableFrame->GetTableUserItem(i);
			tagSpecialScoreInfo scoreInfo;
			scoreInfo.beforeScore = userItem->GetUserScore();
			scoreInfo.agentId = userItem->GetUserBaseInfo().agentId;
			scoreInfo.account = userItem->GetAccount();
			scoreInfo.userId = userItem->GetUserId();
			scoreInfo.chairId = i;
			scoreInfo.addScore = i == winChair ? m_winScore - tax : -m_winScore;
			scoreInfo.revenue = i == winChair ? tax : 0;
			scoreInfo.betScore = m_winScore;
			scoreInfo.rWinScore = m_winScore;
			scoreInfo.cellScore.push_back(m_winScore);
			scoreInfo.cardValue = CardsToString(m_players[i]->GetHandCards());
			scoreInfo.startTime = m_startTime;
			scoreInfo.bWriteScore = true;
			scoreInfo.bWriteRecord = true;
			scoreInfos.push_back(scoreInfo);
		}
	}
	m_pTableFrame->WriteSpecialUserScore(&scoreInfos[0], scoreInfos.size(), m_strRoundId);
}

bool CGameProcess::IsGameMsgCacheExist(uint32_t chair)
{
	for (auto msg : m_gameMsgCache)
	{
		if (msg.chairid == chair)
		{
			return true;
		}
	}
	return false;
}

void CGameProcess::DealHandCardsByCfg()
{
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		for (auto card : m_handCardsCfg[i])
		{
			for (auto itr = m_cards.begin(); itr != m_cards.end(); ++itr)
			{
				if (card == *itr)
				{
					m_cards.erase(itr);
					break;
				}
			}
			m_players[i]->AddHandCard(card);
		}
	}
}

void CGameProcess::DealHandCards()
{
	vector<vector<int32_t>> handCardsList;
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		vector<int32_t> handCards(m_cards.begin(), m_cards.begin() + 13);
		m_cards.erase(m_cards.begin(), m_cards.begin() + 13);
		handCardsList.push_back(handCards);
	}
	if (m_ctrlType == Ctrl_Normal)
	{
		for (int32_t i = 0; i < PLAYER_NUM; ++i)
		{
			for (auto card : handCardsList[i])
			{
				m_players[i]->AddHandCard(card);
			}
		}
	}
	else
	{
		int32_t goodIdx = 0;
		int32_t minNeed = 14;
		vector<PrInfo> prInfos;
		for (int32_t i = 0; i < handCardsList.size(); ++i)
		{
			PrInfo prInfo;
			CalculatePrInfo(handCardsList[i], prInfo);
			prInfos.push_back(prInfo);
			int32_t huaNum = 0;
			for (auto card : handCardsList[i])
			{
				if (getColor(card) == 4)
				{
					huaNum++;
				}
			}
			if ((prInfo.need - huaNum) < minNeed)
			{
				minNeed = prInfo.need;
				goodIdx = i;
			}
		}
		int badIdx = NEXT_CHAIR(goodIdx);
		for (int32_t i = 0; i < PLAYER_NUM; ++i)
		{
			bool good = m_ctrlType == Ctrl_SystemWin ? m_players[i]->GetServerUser()->IsAndroidUser() : !m_players[i]->GetServerUser()->IsAndroidUser();
			int32_t idx = good ? goodIdx : badIdx;
			for (auto card : handCardsList[idx])
			{
				m_players[i]->AddHandCard(card);
			}
		}
	}
}

int32_t CGameProcess::DealCardToUser(bool isFront, uint32_t toChair)
{
	if (m_ctrlType == Ctrl_Normal)
	{
		return DealCardToUserNormal(isFront, toChair);
	}
	else
	{
		bool isWinner = m_ctrlType == Ctrl_SystemWin ? m_players[toChair]->GetServerUser()->IsAndroidUser() : !m_players[toChair]->GetServerUser()->IsAndroidUser();
		return DealCardToUserControl(isFront, toChair, isWinner);
	}
}

int32_t CGameProcess::DealCardToUserNormal(bool isFront, uint32_t toChair)
{
	if (m_cards.size() == 0)
	{
		return GAME_INVALID_CARD;
	}
	int32_t card = m_cards.back();
	m_cards.pop_back();
	if (!isValidCard(card))
	{
		WARNLOG(card);
	}
	if (isFront)
	{
		ReduceCardWallFront();
	}
	else
	{
		ReduceCardWallTail();
	}
	m_players[toChair]->AddHandCard(card);
	return card;
}

int32_t CGameProcess::DealCardToUserControl(bool isFront, uint32_t toChair, bool isWinner)
{
	if (m_cards.size() == 0)
	{
		return GAME_INVALID_CARD;
	}
	int32_t card = GAME_INVALID_CARD;
	uint32_t otherChair = NEXT_CHAIR(toChair);
	PrInfo toPrInfo;
	CalculatePrInfo(m_players[toChair]->GetHandCards(), toPrInfo);
	m_players[toChair]->SetPrInfo(toPrInfo);
	//PrInfo otherPrInfo;
	//CalculatePrInfo(m_players[otherChair]->GetHandCards(), otherPrInfo);
	//m_players[otherChair]->SetPrInfo(otherPrInfo);
	srand(time(nullptr) + m_cards.size());
	int32_t prValue = rand() % 100;
	bool dealGoodWin = prValue < m_dealGoodPrsWin[toPrInfo.need - 1];
	bool dealGoodLose = prValue < m_dealGoodPrsLose[toPrInfo.need - 1];
	bool dealGood = isWinner ? dealGoodWin : dealGoodLose;
	stringstream ss;
	ss << (isWinner ? "赢家发牌" : "输家发牌");
	ss << " 发好牌概率:" << (isWinner ? m_dealGoodPrsWin[toPrInfo.need - 1] : m_dealGoodPrsLose[toPrInfo.need - 1]);
	ss << " 当前概率值:" << prValue;
	ss << (dealGood ? " 发好牌" : " 发差牌");
	WARNLOG(ss.str());
	for (auto itr = m_cards.begin(); itr != m_cards.end(); ++itr)
	{
		if (getColor(card) == 4)
		{
			card = *itr;
			m_cards.erase(itr);
		}
		if (dealGood && m_players[toChair]->NeedCard(*itr))
		{
			card = *itr;
			m_cards.erase(itr);
		}
		else if (!dealGood && !m_players[toChair]->NeedCard(*itr))
		{
			card = *itr;
			m_cards.erase(itr);
		}

		if (card != GAME_INVALID_CARD)
		{
			//log test
			vector<int32_t> handCards(m_players[toChair]->GetHandCards().begin(), m_players[toChair]->GetHandCards().end());
			sort(handCards.begin(), handCards.end());
			stringstream ss;
			ss << "手牌 ";
			ss << "[ ";
			for (auto card : handCards)
			{
				ss << getCardStr(card) << " ";
			}
			ss << "]";
			WARNLOG(ss.str());
			ss.str("");
			ss << "需要的张数 " << toPrInfo.need;
			ss << " 需要的牌 ";
			ss << " [ ";
			for (auto card : toPrInfo.needCards)
			{
				ss << getCardStr(card) << card << " ";
			}
			ss << "]";
			WARNLOG(ss.str());
			string cardDes = dealGood ? "发好牌 " : "发差牌 ";
			WARNLOG(cardDes + getCardStr(card));
			WARNLOG("\n\n");
			break;
		}
	}
	//无法控制牌
	if (card == GAME_INVALID_CARD)
	{
		card = m_cards.back();
		m_cards.pop_back();
		//log test
		WARNLOG("已经无法控制发牌");
		stringstream ss;
		ss << "需要的牌 ";
		ss << " [ ";
		for (auto card : toPrInfo.needCards)
		{
			ss << getCardStr(card) << " ";
		}
		ss << "]";
		WARNLOG(ss.str());
		ss.str("");
		ss << "牌库";
		ss << " [ ";
		for (auto card : m_cards)
		{
			ss << getCardStr(card) << " ";
		}
		ss << "]";
		WARNLOG("\n\n");
	}
	if (!isValidCard(card))
	{
		WARNLOG(card);
	}
	if (isFront)
	{
		ReduceCardWallFront();
	}
	else
	{
		ReduceCardWallTail();
	}
	m_players[toChair]->AddHandCard(card);
	return card;
}

void CGameProcess::ReduceCardWallFront()
{
	m_cardWall[0]++;
	if (m_cardWall[0] == 72)
	{
		m_cardWall[0] = 0;
	}
}

void CGameProcess::ReduceCardWallTail()
{
	m_cardWall[1]--;
	if (m_cardWall[1] == -1)
	{
		m_cardWall[1] = 71;
	}
}

void CGameProcess::CalculatePrInfo(vector<int32_t> & handCards, PrInfo & prInfo)
{
	vector<int32_t> leftCards;
	leftCards.insert(leftCards.begin(), m_cards.begin(), m_cards.end());
	array<int32_t, TABLE_SIZE> leftTable = { 0 };
	cardsToTable(leftCards, leftTable);
	vector<int32_t> outCardsExclude;
	findPrInfo(handCards, outCardsExclude, leftTable, prInfo);
}

//得到桌子实例
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink()
{
	shared_ptr<CGameProcess> pGameProcess(new CGameProcess());
	shared_ptr<ITableFrameSink> pITableFrameSink = dynamic_pointer_cast<ITableFrameSink>(pGameProcess);
	return pITableFrameSink;
}

//删除桌子实例
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink> & pITableFrameSink)
{
	pITableFrameSink.reset();
}


//----------------------------------------测试功能-----------------------------------------//
bool CGameProcess::onMsgLeftCards(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (!m_testEnable)
	{
		WARNLOG("");
		return false;
	}
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	//if (chairid != m_nCurRoundChairId)
	//{
	//	WARNLOG("");
	//	return false;
	//}
	//if (m_players[chairid]->GetHandCards().size()%3 != 2)
	//{
	//	WARNLOG("");
	//	return false;
	//}
	m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	CMD_S_LeftCards sMsgLeftCards;
	for (auto card : m_cards)
	{
		sMsgLeftCards.add_cards(card);
	}
	string serverInfo = "";
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		serverInfo += m_players[i]->DebugString();
		serverInfo += "\n";
		WARNLOG(m_players[i]->DebugString());
	}
	sMsgLeftCards.set_serverinfo(serverInfo);
	string content = sMsgLeftCards.SerializeAsString();
	m_pTableFrame->SendTableData(chairid, SUB_S_LEFT_CARDS, (uint8_t*)content.data(), content.size());
	return true;
}

bool CGameProcess::onMsgReplaceCard(uint32_t chairid, const uint8_t * data, uint32_t datasize)
{
	if (!m_testEnable)
	{
		WARNLOG("");
		return false;
	}
	if (chairid < 0 || chairid >= PLAYER_NUM)
	{
		WARNLOG("");
		return false;
	}
	//	if (chairid != m_nCurRoundChairId)
	//	{
	//		WARNLOG("");
	//		return false;
	//	}
	//	if (m_players[chairid]->GetHandCards().size() % 3 != 2)
	//	{
	//		WARNLOG("");
	//		return false;
	//	}
	CMD_C_ReplaceCard cMsgReplaceCard;
	if (!cMsgReplaceCard.ParseFromArray(data, datasize))
	{
		WARNLOG("");
		return false;
	}
	if (cMsgReplaceCard.origincard_size() < 1)
	{
		WARNLOG("");
		return false;
	}
	if (cMsgReplaceCard.origincard_size() != cMsgReplaceCard.targetcard_size())
	{
		WARNLOG("");
		return false;
	}
	auto replaceOneCard = [&](int32_t originCard, int32_t targetCard)
	{
		if (!m_players[chairid]->RemoveHandCards(originCard))
		{
			WARNLOG("");
			return false;
		}
		bool replaced = false;
		for (auto itr = m_cards.begin(); itr != m_cards.end(); ++itr)
		{
			if (*itr == targetCard)
			{
				m_cards.erase(itr);
				m_players[chairid]->AddHandCard(targetCard);
				replaced = true;
				break;
			}
		}
		if (!replaced)
		{
			m_players[chairid]->AddHandCard(originCard);
			return false;
		}
		else
		{
			m_cards.insert(m_cards.begin(), originCard);
			return true;
		}
	};
	for (uint32_t i = 0; i < cMsgReplaceCard.origincard_size(); ++i)
	{
		if (!replaceOneCard(cMsgReplaceCard.origincard(i), cMsgReplaceCard.targetcard(i)))
		{
			WARNLOG("");
			return false;
		}
	}

	CMD_S_ReplaceCard sMsgReplaceCard;
	for (uint32_t i = 0; i < cMsgReplaceCard.origincard_size(); ++i)
	{
		sMsgReplaceCard.add_origincard(cMsgReplaceCard.origincard(i));
		sMsgReplaceCard.add_targetcard(cMsgReplaceCard.targetcard(i));
	}
	string content = sMsgReplaceCard.SerializeAsString();
	m_pTableFrame->SendTableData(chairid, SUB_S_REPLACE_CARD, (uint8_t*)content.data(), content.size());
	UpdateGameRound(m_nCurRoundChairId, false);
	if (m_gameOpCnt == 0)
	{
		//发送操作选项给其他玩家[闲家天听] 此处需要在更新回合之后，否则闲家天听选项值会被重置
		for (uint32_t i = 0; i < PLAYER_NUM; ++i)
		{
			if (i != m_nBankerChairId)
			{
				if (SendOption(i, mj_ting))
				{
					m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
					m_TimerTianTing = m_pLoopThread->getLoop()->runAfter(m_optionTime, boost::bind(&CGameProcess::OnTimerTianTing, this));
					m_tianTinging = true;
				}
			}
		}
	}
	m_pLoopThread->getLoop()->cancel(m_TimerTianTing);
	m_pLoopThread->getLoop()->cancel(m_TimerOption);
	m_pLoopThread->getLoop()->cancel(m_TimerRound);
	return true;
}
