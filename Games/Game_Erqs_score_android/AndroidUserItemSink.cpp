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

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
//#include "public/pb2Json.h"

#include "proto/Erqs.Message.pb.h"

#include "../Game_Erqs/CTableMgr.h"
#include "../Game_Erqs/CPart.h"
#include "../Game_Erqs/CTable.h"
#include "../Game_Erqs/GamePlayer.h"
#include "../Game_Erqs/MjAlgorithm.h"
#include "../Game_Erqs/CMD_Game.h"

using namespace Erqs;

#include "AndroidUserItemSink.h"

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


CAndroidUserItemSink::CAndroidUserItemSink()
{
	m_pGameRoomInfo = nullptr;
	m_pTableFrame = nullptr;
	m_pLoopThread = nullptr;
	m_selfChair = 0;
	m_startRound = false;
	m_curRoundChair = 0;
	m_startAniTime = 0.0f;
	m_ctrlType = Ctrl_None;
	muduo::Logger::setOutput(asyncOutput);
	if (!Init())
	{
		WARNLOG("机器人初始化失败");
	}
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{

}

bool CAndroidUserItemSink::Init()
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
	if (!boost::filesystem::exists(ROBOT_INI_FILENAME))
	{
		WARNLOG("机器人配置文件不存在");
		return false;
	}
	boost::property_tree::ptree pt;
	boost::property_tree::read_ini(ROBOT_INI_FILENAME, pt);
	vector<int32_t> roundTimeWeights;
	intListParse(pt.get<string>("TIME.ROUND_TIME", ""), roundTimeWeights);
	int roundTimeWeightArr[100] = { 0 };
	for (int32_t i = 0; i < roundTimeWeights.size(); ++i)
	{
		roundTimeWeightArr[i] = roundTimeWeights[i];
	}
	m_roundTimeWeight.init(roundTimeWeightArr, roundTimeWeights.size());
	m_roundTimeWeight.shuffleSeq();
	vector<int32_t> optionTimeWeights;
	intListParse(pt.get<string>("TIME.OPTION_TIME", ""), optionTimeWeights);
	int optionTimeWeightArr[100] = { 0 };
	for (int32_t i = 0; i < optionTimeWeights.size(); ++i)
	{
		optionTimeWeightArr[i] = optionTimeWeights[i];
	}
	m_optionTimeWeight.init(optionTimeWeightArr, optionTimeWeights.size());
	m_optionTimeWeight.shuffleSeq();

	if (!boost::filesystem::exists(INI_FILENAME))
	{
		WARNLOG("游戏配置文件不存在");
		return false;
	}
	boost::property_tree::ptree gamePt;
	boost::property_tree::read_ini(INI_FILENAME, gamePt);
	m_startAniTime = gamePt.get<float>("TIME.START_ANI_TIME", 0.0f);
	return true;
}

bool CAndroidUserItemSink::RepositionSink()
{
	return true;
}

bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame> & pTableFrame, shared_ptr<IServerUserItem> & pAndroidUserItem)
{
	return true;
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem> & pUserItem)
{
	if (nullptr == pUserItem)
	{
		return false;
	}
	m_selfUser = pUserItem;
	return true;
}

void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame> & pTableFrame)
{
	if (!pTableFrame)
	{
		return;
	}
	m_pTableFrame = pTableFrame;
	m_pGameRoomInfo = m_pTableFrame->GetGameRoomInfo();
	m_pLoopThread = m_pTableFrame->GetLoopThread();
}

bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t * pData, uint32_t wDataSize)
{
	switch (wSubCmdID)
	{
	case SUB_S_ANDROID_AI_LV:
		return onAiLv(pData, wDataSize);
	case SUB_S_GAME_START:
		return onGameStart(pData, wDataSize);
	case SUB_S_REPLACE_HUA:
		return onReplaceHua(pData, wDataSize);
	case SUB_S_OUT_CARD:
		return onOutCard(pData, wDataSize);
	case SUB_S_CHI:
		return onChi(pData, wDataSize);
	case SUB_S_PENG:
		return onPeng(pData, wDataSize);
	case SUB_S_GANG:
		return onGang(pData, wDataSize);
	case SUB_S_TING:
		return onTing(pData, wDataSize);
	case SUB_S_PASS:
		break;
	case SUB_S_OPTIONS:
		return onGameOptions(pData, wDataSize);
	case SUB_S_GAME_ROUND:
		return onGameRound(pData, wDataSize);
	case SUB_S_GAME_END:
		return onGameEnd(pData, wDataSize);
	default:
		break;
	}
	return true;
}

void CAndroidUserItemSink::SetAndroidStrategy(tagAndroidStrategyParam * strategy)
{

}

tagAndroidStrategyParam* CAndroidUserItemSink::GetAndroidStrategy()
{
	return nullptr;
}

bool CAndroidUserItemSink::onAiLv(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Android_AiLv sMsgAndroidAiLv;
	if (!sMsgAndroidAiLv.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	m_ctrlType = (ControlType)sMsgAndroidAiLv.lv();
	string des[] = { "", "正常输赢", "系统赢", "系统输" };
	WARNLOG("机器人AI级别：" + des[m_ctrlType]);
	return true;
}

bool CAndroidUserItemSink::onGameStart(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_GameStart sMsgGameStart;
	if (!sMsgGameStart.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	for (uint32_t i = 0; i < sMsgGameStart.players_size(); ++i)
	{
		Erqs::Player sMsgPlayer = sMsgGameStart.players(i);
		int32_t chair = sMsgPlayer.chairid();
		if (sMsgPlayer.userid() == m_selfUser->GetUserId())
		{
			m_selfChair = chair;
		}
		m_players[chair] = make_shared<CGamePlayer>();
		for (uint32_t j = 0; j < sMsgPlayer.handcards_size(); ++j)
		{
			m_players[chair]->AddHandCard(sMsgPlayer.handcards(j));
		}
	}
	m_startRound = true;
	return true;
}

bool CAndroidUserItemSink::onReplaceHua(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_ReplaceHua sMsgReplaceHua;
	if (!sMsgReplaceHua.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	int32_t chair = sMsgReplaceHua.chairid();
	for (int i = 0; i < sMsgReplaceHua.huacards_size(); ++i)
	{
		m_players[chair]->AddHuaCard(sMsgReplaceHua.huacards(i));
	}
	for (int i = 0; i < sMsgReplaceHua.replacecards_size(); ++i)
	{
		m_players[chair]->AddHandCard(sMsgReplaceHua.replacecards(i));
	}
	if (chair == m_selfChair)
	{
		m_pLoopThread->getLoop()->cancel(m_timerOutCard);
	}
	return true;
}

bool CAndroidUserItemSink::onOutCard(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_OutCard sMsgOutCard;
	if (!sMsgOutCard.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	int32_t chair = sMsgOutCard.chairid();
	m_players[chair]->OutCard(sMsgOutCard.card());
	if (chair == m_selfChair)
	{
		m_pLoopThread->getLoop()->cancel(m_timerOutCard);
	}
	return true;
}

bool CAndroidUserItemSink::onChi(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Chi sMsgChi;
	if (!sMsgChi.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	//玩家吃牌
	int32_t chair = sMsgChi.chairid();
	vector<int32_t> cards;
	for (int32_t i = 0; i < sMsgChi.cpg().cards_size(); ++i)
	{
		cards.push_back(sMsgChi.cpg().cards(i));
	}
	if (!m_players[chair]->Chi(cards))
	{
		WARNLOG("");
		return false;
	}
	//被吃玩家删除被吃牌
	m_players[sMsgChi.cpg().targetchair()]->RemoveOutCard(sMsgChi.cpg().targetcard());
	return true;
}

bool CAndroidUserItemSink::onPeng(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Peng sMsgPeng;
	if (!sMsgPeng.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	//玩家碰牌
	int32_t chair = sMsgPeng.chairid();
	if (!m_players[chair]->Peng())
	{
		WARNLOG("");
		return false;
	}
	//被碰玩家删除被碰牌
	m_players[sMsgPeng.cpg().targetchair()]->RemoveOutCard(sMsgPeng.cpg().targetcard());
	return true;
}

bool CAndroidUserItemSink::onGang(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Gang sMsgGang;
	if (!sMsgGang.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	//玩家杠牌
	int32_t chair = sMsgGang.chairid();
	vector<int32_t> cards;
	for (int32_t i = 0; i < sMsgGang.cpg().cards_size(); ++i)
	{
		cards.push_back(sMsgGang.cpg().cards(i));
	}
	if (!m_players[chair]->Gang(cards))
	{
		WARNLOG("");
		return false;
	}
	//被杠玩家删除被杠牌
	if ((MjType)sMsgGang.cpg().type() == mj_dian_gang)
	{
		m_players[sMsgGang.cpg().targetchair()]->RemoveOutCard(sMsgGang.cpg().targetcard());
	}
	return true;
}

bool CAndroidUserItemSink::onTing(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Ting sMsgTing;
	if (!sMsgTing.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	int32_t chair = sMsgTing.chairid();
	if (!m_players[chair]->Ting(sMsgTing.card()))
	{
		WARNLOG("");
		return false;
	}
	return true;
}

bool CAndroidUserItemSink::onGameOptions(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_Options sMsgOptions;
	if (!sMsgOptions.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	vector<CPG> optionCpgs;
	for (int32_t i = 0; i < sMsgOptions.optionalcpgs_size(); ++i)
	{
		const Erqs::ChiPengGang& sMsgCpg = sMsgOptions.optionalcpgs(i);
		CPG cpg;
		cpg.type = (MjType)sMsgCpg.type();
		cpg.targetCard = sMsgCpg.targetcard();
		cpg.targetChair = sMsgCpg.targetchair();
		for (int32_t j = 0; j < sMsgCpg.cards_size(); ++j)
		{
			cpg.cards.push_back(sMsgCpg.cards(j));
		}
		optionCpgs.push_back(cpg);
	}
	vector<TingOptInfo> tingOptInfos;
	for (int32_t i = 0; i < sMsgOptions.tinginfos_size(); ++i)
	{
		const Erqs::MsgTingInfo& msgTingInfo = sMsgOptions.tinginfos(i);
		TingOptInfo tingOptInfo;
		for (int32_t j = 0; j < msgTingInfo.hucards_size(); ++j)
		{
			const Erqs::MsgHuCard& msgHuCard = msgTingInfo.hucards(j);
			TingInfo tingInfo;
			tingInfo.card = msgHuCard.card();
			tingOptInfo.tingInfos.push_back(tingInfo);
		}
		tingOptInfo.outCard = msgTingInfo.outcard();
		tingOptInfos.push_back(tingOptInfo);
	}
	int32_t chair = sMsgOptions.chairid();
	m_players[chair]->SetOption(sMsgOptions.option());
	m_players[chair]->SetOptionCpgs(optionCpgs);
	m_players[chair]->SetTingOptInfos(tingOptInfos);
	if (chair == m_selfChair)
	{
		m_pLoopThread->getLoop()->cancel(m_timerOption);
		int32_t time = m_optionTimeWeight.getResultByWeight() + 1;
		if (sMsgOptions.option() & mj_hu != 0)
		{
			time = 2;
		}
		m_timerOption = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CAndroidUserItemSink::OnTimerOption, this));
		if (m_curRoundChair == m_selfChair)
		{
			m_pLoopThread->getLoop()->cancel(m_timerOutCard);
			time += 1;
			m_timerOutCard = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CAndroidUserItemSink::OnTimerOutCard, this));
		}
	}
	return true;
}

bool CAndroidUserItemSink::onGameRound(const uint8_t * pData, uint32_t wDataSize)
{
	CMD_S_GameRound sMsgGameRound;
	if (!sMsgGameRound.ParseFromArray(pData, wDataSize)) {
		return false;
	}
	int32_t chair = sMsgGameRound.chairid();
	if (sMsgGameRound.card() != GAME_INVALID_CARD)
	{
		m_players[chair]->AddHandCard(sMsgGameRound.card());
	}
	m_curRoundChair = sMsgGameRound.chairid();
	if (m_curRoundChair == m_selfChair)
	{
		int32_t time = m_roundTimeWeight.getResultByWeight() + 1;
		if (m_startRound)
		{
			time += m_startAniTime;
		}
		m_pLoopThread->getLoop()->cancel(m_timerOutCard);
		m_timerOutCard = m_pLoopThread->getLoop()->runAfter(time, boost::bind(&CAndroidUserItemSink::OnTimerOutCard, this));
	}
	m_startRound = false;
	return true;
}

bool CAndroidUserItemSink::onGameEnd(const uint8_t * pData, uint32_t wDataSize)
{
	m_pLoopThread->getLoop()->cancel(m_timerOutCard);
	m_pLoopThread->getLoop()->cancel(m_timerOption);
}

void CAndroidUserItemSink::OnTimerOutCard()
{
	m_pLoopThread->getLoop()->cancel(m_timerOutCard);
	int32_t outCard = GAME_INVALID_CARD;
	if (m_players[m_selfChair]->IsTing())
	{
		outCard = m_players[m_selfChair]->GetHandCards().back();
	}
	else
	{
		PrInfo fastHuPrInfo;
		calculatePrInfo(m_players[m_selfChair]->GetHandCards(), fastHuPrInfo);
		if (fastHuPrInfo.outCard == GAME_INVALID_CARD)
		{
			WARNLOG("无牌可出，出手牌最后一张");
			fastHuPrInfo.outCard = m_players[m_selfChair]->GetHandCards().back();
		}
		outCard = fastHuPrInfo.outCard;
	}
	CMD_C_OutCard cMsgOutCard;
	cMsgOutCard.set_card(outCard);
	string content = cMsgOutCard.SerializeAsString();
	m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_OUT_CARD, (uint8_t*)content.data(), content.size());
}

void CAndroidUserItemSink::OnTimerOption()
{
	m_pLoopThread->getLoop()->cancel(m_timerOption);
	if (aiActionHu()) return;
	if (aiActionTing()) return;
	if (aiActionGang()) return;
	if (aiActionPeng()) return;
	if (aiActionChi()) return;
	if (aiActionPass()) return;
}

bool CAndroidUserItemSink::aiActionHu()
{
	if (m_players[m_selfChair]->GetOption() & mj_hu)
	{
		m_pLoopThread->getLoop()->cancel(m_timerOutCard);
		CMD_C_NULL cMsgNull;
		string content = cMsgNull.SerializeAsString();
		m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_HU, (uint8_t*)content.data(), content.size());
		return true;
	}
	return false;
}

bool CAndroidUserItemSink::aiActionTing()
{
	if (m_players[m_selfChair]->GetOption() & mj_ting)
	{
		PrInfo fastHuPrInfo;
		calculatePrInfo(m_players[m_selfChair]->GetHandCards(), fastHuPrInfo);
		//听牌胡牌概率大于0，则听牌
		if (fastHuPrInfo.need == 1 && fastHuPrInfo.probability > 0)
		{
			//听牌包含出牌功能
			m_pLoopThread->getLoop()->cancel(m_timerOutCard);
			CMD_C_Ting cMsgTing;
			cMsgTing.set_card(fastHuPrInfo.outCard);
			string content = cMsgTing.SerializeAsString();
			m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_TING, (uint8_t*)content.data(), content.size());
			return true;
		}
	}
	return false;
}

bool CAndroidUserItemSink::aiActionGang()
{
	if (m_players[m_selfChair]->GetOption() & mj_an_gang || m_players[m_selfChair]->GetOption() & mj_jia_gang || m_players[m_selfChair]->GetOption() & mj_dian_gang)
	{
		for (auto optionCgp : m_players[m_selfChair]->GetOptionalCpgs())
		{
			if (isGangType(optionCgp.type))
			{
				if (!m_players[m_selfChair]->IsTing())
				{
					vector<int32_t> handCardsAfterAction(m_players[m_selfChair]->GetHandCards().begin(), m_players[m_selfChair]->GetHandCards().end());
					vector<int32_t> removeCards;
					if (optionCgp.type == mj_an_gang)
					{
						removeCards = { optionCgp.targetCard, optionCgp.targetCard, optionCgp.targetCard, optionCgp.targetCard };
					}
					else if (optionCgp.type == mj_dian_gang)
					{
						removeCards = { optionCgp.targetCard, optionCgp.targetCard, optionCgp.targetCard };
					}
					else if (optionCgp.type == mj_jia_gang)
					{
						removeCards = { optionCgp.targetCard };
					}
					for (auto removeCard : removeCards)
					{
						for (auto itr = handCardsAfterAction.begin(); itr != handCardsAfterAction.end(); ++itr)
						{
							if (*itr == removeCard)
							{
								handCardsAfterAction.erase(itr);
								break;
							}
						}
					}
					if (!aiActionExecutable(handCardsAfterAction, optionCgp.type))
					{
						continue;
					}
				}
				CMD_C_Gang cMsgGang;
				for (auto card : optionCgp.cards)
				{
					cMsgGang.add_cards(card);
				}
				string content = cMsgGang.SerializeAsString();
				m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_GANG, (uint8_t*)content.data(), content.size());
				return true;
			}
		}
	}
	return false;
}

bool CAndroidUserItemSink::aiActionPeng()
{
	if (m_players[m_selfChair]->GetOption() & mj_peng)
	{
		for (auto optionCgp : m_players[m_selfChair]->GetOptionalCpgs())
		{
			if (optionCgp.type == mj_peng)
			{
				vector<int32_t> handCardsAfterAction(m_players[m_selfChair]->GetHandCards().begin(), m_players[m_selfChair]->GetHandCards().end());
				vector<int32_t> removeCards = { optionCgp.targetCard, optionCgp.targetCard };
				for (auto removeCard : removeCards)
				{
					for (auto itr = handCardsAfterAction.begin(); itr != handCardsAfterAction.end(); ++itr)
					{
						if (*itr == removeCard)
						{
							handCardsAfterAction.erase(itr);
							break;
						}
					}
				}
				if (!aiActionExecutable(handCardsAfterAction, optionCgp.type))
				{
					break;
				}
				CMD_C_NULL cMsgNull;
				string content = cMsgNull.SerializeAsString();
				m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_PENG, (uint8_t*)content.data(), content.size());
				return true;
			}
		}
	}
	return false;
}

bool CAndroidUserItemSink::aiActionChi()
{
	if (m_players[m_selfChair]->GetOption() & mj_left_chi || m_players[m_selfChair]->GetOption() & mj_center_chi || m_players[m_selfChair]->GetOption() & mj_right_chi)
	{
		for (auto optionCgp : m_players[m_selfChair]->GetOptionalCpgs())
		{
			if (isChiType(optionCgp.type))
			{
				vector<int32_t> handCardsAfterAction(m_players[m_selfChair]->GetHandCards().begin(), m_players[m_selfChair]->GetHandCards().end());
				vector<int32_t> removeCards;
				if (optionCgp.type == mj_left_chi)
				{
					removeCards = { optionCgp.cards[1], optionCgp.cards[2] };
				}
				else if (optionCgp.type == mj_center_chi)
				{
					removeCards = { optionCgp.cards[0], optionCgp.cards[2] };
				}
				else if (optionCgp.type == mj_right_chi)
				{
					removeCards = { optionCgp.cards[0], optionCgp.cards[1] };
				}
				for (auto removeCard : removeCards)
				{
					for (auto itr = handCardsAfterAction.begin(); itr != handCardsAfterAction.end(); ++itr)
					{
						if (*itr == removeCard)
						{
							handCardsAfterAction.erase(itr);
							break;
						}
					}
				}
				if (!aiActionExecutable(handCardsAfterAction, optionCgp.type))
				{
					continue;
				}
				CMD_C_Chi cMsgChi;
				for (auto card : optionCgp.cards)
				{
					cMsgChi.add_cards(card);
				}
				string content = cMsgChi.SerializeAsString();
				m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_CHI, (uint8_t*)content.data(), content.size());
				return true;
			}
		}
	}
	return false;
}

bool CAndroidUserItemSink::aiActionPass()
{
	CMD_C_NULL cMsgNull;
	string content = cMsgNull.SerializeAsString();
	m_pTableFrame->OnGameEvent(m_selfChair, SUB_C_PASS, (uint8_t*)content.data(), content.size());
	return true;
}

bool CAndroidUserItemSink::aiActionExecutable(vector<int32_t> & handCardsAfterAction, MjType mjType)
{
	PrInfo prInfo;
	calculatePrInfo(m_players[m_selfChair]->GetHandCards(), prInfo);
	PrInfo prInfoAfterAction;
	calculatePrInfo(handCardsAfterAction, prInfoAfterAction);
	//胡牌概率变为0，则不执行action
	if (prInfo.probability > 0 && prInfoAfterAction.probability <= 0)
	{
		return false;
	}
	if (isGangType(mjType))
	{
		//胡牌需要的张数变大，并超过接受范围，则不执行action
		if (prInfoAfterAction.need >= (prInfo.need + 2))
		{
			return false;
		}
	}
	else if (isChiType(mjType))
	{
		//吃完，胡牌张数没变小，则不执行action
		if (prInfoAfterAction.need >= prInfo.need)
		{
			return false;
		}
	}
	else
	{
		//碰完，胡牌张数变大，则不执行action
		if (prInfoAfterAction.need >= (prInfo.need + 1))
		{
			return false;
		}
	}

	return true;
}

void CAndroidUserItemSink::calculatePrInfo(vector<int32_t> & handCards, PrInfo & fastHuPrInfo)
{
	array<int32_t, TABLE_SIZE> leftTable;
	leftTable.fill(4);
	if (m_ctrlType == Ctrl_SystemWin)
	{
		calculateLeftTableSysWin(leftTable);
	}
	else if (m_ctrlType == Ctrl_SystemLose)
	{
		calculateLeftTableSysLose(leftTable);
	}
	else
	{
		calculateLeftTableNormal(leftTable);
	}
	vector<int32_t> outCardsExclude;
	uint32_t otherChair = NEXT_CHAIR(m_selfChair);
	if (m_players[otherChair]->IsTing())
	{
		for (auto tingInfo : m_players[otherChair]->GetTingInfos())
		{
			outCardsExclude.push_back(tingInfo.card);
		}
	}
	findPrInfo(handCards, outCardsExclude, leftTable, fastHuPrInfo);
}

void CAndroidUserItemSink::calculateLeftTableNormal(array<int32_t, TABLE_SIZE> & leftTable)
{
	vector<int32_t> outCards;
	//自己手牌，出牌，吃碰杠，其他玩家出牌，吃碰明杠
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i == m_selfChair)
		{
			outCards.insert(outCards.begin(), m_players[i]->GetHandCards().begin(), m_players[i]->GetHandCards().end());
		}
		outCards.insert(outCards.begin(), m_players[i]->GetOutCards().begin(), m_players[i]->GetOutCards().end());
		for (auto cpg : m_players[i]->GetCpgs())
		{
			if (cpg.type == mj_an_gang && i != m_selfChair)
			{
				continue;
			}
			outCards.insert(outCards.begin(), cpg.cards.begin(), cpg.cards.end());
		}
	}
	array<int32_t, TABLE_SIZE> outTable = { 0 };
	cardsToTable(outCards, outTable);
	for (int32_t i = 1; i < TABLE_SIZE; ++i)
	{
		leftTable[i] -= outTable[i];
	}
}

void CAndroidUserItemSink::calculateLeftTableSysWin(array<int32_t, TABLE_SIZE> & leftTable)
{
	vector<int32_t> outCards;
	//自己手牌，出牌，吃碰杠，其他玩家手牌，出牌，吃碰杠
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		outCards.insert(outCards.begin(), m_players[i]->GetHandCards().begin(), m_players[i]->GetHandCards().end());
		outCards.insert(outCards.begin(), m_players[i]->GetOutCards().begin(), m_players[i]->GetOutCards().end());
		for (auto cpg : m_players[i]->GetCpgs())
		{
			outCards.insert(outCards.begin(), cpg.cards.begin(), cpg.cards.end());
		}
	}
	array<int32_t, TABLE_SIZE> outTable = { 0 };
	cardsToTable(outCards, outTable);
	for (int32_t i = 1; i < TABLE_SIZE; ++i)
	{
		leftTable[i] -= outTable[i];
	}
}

void CAndroidUserItemSink::calculateLeftTableSysLose(array<int32_t, TABLE_SIZE> & leftTable)
{
	vector<int32_t> outCards;
	//只计算自己手牌
	for (int32_t i = 0; i < PLAYER_NUM; ++i)
	{
		if (i == m_selfChair)
		{
			outCards.insert(outCards.begin(), m_players[i]->GetHandCards().begin(), m_players[i]->GetHandCards().end());
		}
	}
	array<int32_t, TABLE_SIZE> outTable = { 0 };
	cardsToTable(outCards, outTable);
	for (int32_t i = 1; i < TABLE_SIZE; ++i)
	{
		leftTable[i] -= outTable[i];
	}
}

//CreateAndroidSink 创建实例
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
	shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
	shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
	return pIAndroidSink;
}

//DeleteAndroidSink 删除实例
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink> & pSink)
{
	if (pSink)
	{
		pSink->RepositionSink();
	}
	pSink.reset();
}
