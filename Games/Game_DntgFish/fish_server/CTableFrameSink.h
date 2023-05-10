
#ifndef TABLE_FRAME_SINK_H_
#define TABLE_FRAME_SINK_H_

#include <vector>
#include <deque>
#include <glog/logging.h>

using namespace std;

#include "../inc/aics/IAicsEngine.h"
#include <xml/XmlParser.h>
#include <iunknown.h>
#include "SceneManager.h"
#include "FishProduceManager.h"
#include "ISendTableData.h"
#include "gameplay/collect/CollectUtility.h"
#include "ITimerEvent.h"
#include "UserPrefs.h"
#include "../FishProduceUtils/FishProduceDataManager.h"
#include "../FishProduceUtils/FishArrayManager.h"
#include "PlayLog.h"
#ifdef __USE_LIFE2__
#include "ArithmLog.h"
#endif

#include "../FishProduceUtils/NetworkDataUtility.h"
#include "../FishProduceUtils/FishProduceCommon.h"

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>


#include "public/StdRandom.h"

#define MAX_WASH_COUNT 10
#define FIRECOUNTTIME  15  //准备发射倒计时

struct WashData
{
	DWORD amountMin;
	DWORD amountMax;
};
struct WashXmlConfiguration
{
	WashData washData[MAX_WASH_COUNT]; // 洗码量目标
	short washDataCount;
	WashPercent percent; // 奖金池抽取比例
	WORD minimumAwardMin, minimumAwardMax; // 保底奖励
};

struct RumtimeWashData
{
	DWORD currentAmount; // 当前洗码量
	DWORD arrivalAmount; // 目标洗码量
};

struct PlayerScoreInfo
{
	long long winScore;
	long long loseScore;
	long long exchangeAddScore;
	long long exchangeSubScore;
};

class CTableFrameSink : public ITableFrameSink
//                     , public ITableUserAction
                     , public ISendTableData
//                     , public ITimerEvent
{
public:
	CTableFrameSink();
	virtual ~CTableFrameSink();
public:
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    //virtual double QueryConsumeQuota(shared_ptr<CServerUserItem> server_user_item) { return 0; }
    //virtual double QueryLessEnterScore(WORD chair_id, shared_ptr<CServerUserItem> server_user_item) { return 0; }
    //virtual bool QueryBuckleServiceCharge(WORD chair_id) { return true; }
    //virtual void SetGameBaseScore(LONG base_score) {}
    //virtual bool OnDataBaseMessage(WORD request_id, void* data, WORD data_size) { return false; }
    //virtual bool OnUserScroeNotify(WORD chair_id, shared_ptr<CServerUserItem> server_user_item, BYTE reason) { return false; }
	virtual bool OnEventGameStart();
public://modified by James interface.
    //virtual bool OnEventGameConclude(WORD chair_id, shared_ptr<CServerUserItem> server_user_item, BYTE reason)
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag);
    //virtual bool OnEventSendGameScene(WORD chair_id, shared_ptr<CServerUserItem> server_user_item, BYTE game_status, bool send_secret);
    virtual bool OnEventGameScene(DWORD chairid, bool bisLookon);
    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser)
    {
        return true;
    }
    virtual bool CanLeftTable(int64_t userId)
    {
        return true;
    }
public:
	virtual bool OnTimerMessage(DWORD dwTimerID, WPARAM dwBindParameter);
 	//{
 	//	return true;
 	//}
    //记录水果机免费游戏剩余次数
    virtual bool WriteSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord)
    {
        return true;
    }
    virtual bool GetSgjFreeGameRecord(tagSgjFreeGameRecord* pSgjFreeGameRecord,int64_t userId)
    {
         return true;
    }
public://modified by James.
    virtual bool OnGameMessage( uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);
public:
    virtual bool OnActionUserOffLine(WORD chair_id, shared_ptr<ITableFrame> pIServerUserItem)
	{
 		//TRACE_INFO(_T("CTableFrameSink::OnActionUserOffLine start. [table_id=%d chair_id=%d]"), pIServerUserItem->GetTableID(), chair_id);
		m_pServerUserItemForGamescene[chair_id] = NULL;
		m_bulletCount[chair_id] = 0;
		memset(m_bullets[chair_id], 0, sizeof(m_bullets[chair_id]));
 		//TRACE_INFO(_T("CTableFrameSink::OnActionUserOffLine End."));
		return true;
	}
    virtual bool OnActionUserConnect(WORD chair_id, shared_ptr<ITableFrame> pIServerUserItem)
	{
#ifdef __GAMEPLAY_PAIPAILE__
		if (game_config.pplData[chair_id].isPlaying) {
			resetPplData(chair_id);
			DWORD score = 0;
			sendGameplayData(chair_id, INVALID_CHAIR, gptPpl, gctEndSync, chair_id, &score, sizeof(score));
		}
#endif
		return true;
	}
public://modified by James
    /*
    virtual bool OnActionUserSitDown(WORD chair_id, shared_ptr<CServerUserItem> server_user_item, bool lookon_user);
    virtual bool OnActionUserStandUp(WORD chair_id, shared_ptr<CServerUserItem> server_user_item, bool lookon_user);
	virtual bool OnActionUserOnReady(WORD chair_id, IServerUserItem * server_user_item, void* data, WORD data_size)
	{
		return true;
    } */
    virtual bool OnUserEnter(int64_t userId, bool isLookon);
    virtual bool OnUserReady(int64_t userId, bool isLookon)
    {
        return true;
    }
    virtual bool OnUserLeft(int64_t userId, bool isLookon);
public:
	virtual bool send_table_data(WORD me_chair_id, WORD chair_id, WORD sub_cmdid, void* data, WORD data_size, bool lookon, WORD exclude_chair_id = INVALID_CHAIR);
    virtual WORD get_table_id() {return m_pITableFrame->GetTableId();}
 //protected:
 //	virtual void OnTimerEvent(DWORD dwTimerID, WPARAM dwBindParameter);

private:
    void KickOutUser(DWORD chairid);
	void killAllGameTimer();
	void setGameTimer(DWORD dwTimerID, DWORD dwElapse, DWORD dwRepeat, WPARAM dwBindParameter, bool noSync = false);
	void killGameTimer(DWORD dwTimerID, bool noSync = false);
	void SendRoomMessage(WORD type, LPCTSTR format, ...);
	void SendGlobalRoomMessage(WORD type, LPCTSTR format, ...);
    void SendGlobalRoomMessage(shared_ptr<CServerUserItem> server_user_item, WORD type, LPCTSTR format, ...);
	void OnGameFree();
    void OnGameStart();
	void StartGameTimer();
	bool LoadConfig();
 	//bool SendGameConfig(shared_ptr<CServerUserItem> server_user_item);
	int  GetBulletID(WORD chairid);
	void readGameplayDataFromXml(CXmlParser& xml);
	bool OnTimerSwitchScene(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize);
	bool OnTimerSceneEnd(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize);
	bool OnTimerFishProduce(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize);
	bool OnTimerPauseBoom(WPARAM bind_param, unsigned char* buffer, unsigned short bufferSize);

    bool OnSubExchangeFishScore(shared_ptr<CServerUserItem> server_user_item, BOOL inc);
    bool OnSubUserFire(shared_ptr<CServerUserItem> server_user_item, const CMD_C_UserFire& user_fire);
	bool OnSubSpecialRotate(shared_ptr<CServerUserItem> server_user_item, const CMD_C_SpecialRotate& specialRotate);
    bool SendGameScene(shared_ptr<CServerUserItem> server_user_item);
    bool SendGameScene1(shared_ptr<CServerUserItem> server_user_item);
    bool OnSubHitFish(shared_ptr<CServerUserItem> server_user_item, const CMD_C_HitFish& hit_fish, unsigned short fish_count);
	bool OnSubHitFish_Normal(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_HitFish& hit_fish, unsigned short fish_count, BulletScoreT fire_score, const FishKind* fish_kind, short &catch_fish_count, bool &isBoss, bool &bBossOrGlobalBomb, LONG &score);
	bool OnSubHitFish_Special(std::shared_ptr<CServerUserItem> pIServerUserItem, const CMD_C_HitFish& hit_fish, unsigned short fish_count, BulletScoreT fire_score, const FishKind* fish_kind, short &catch_fish_count, bool &isBoss, bool &bBossOrGlobalBomb, bool &is_pause_boom, LONG &score);
    bool OnSubLockFish(shared_ptr<CServerUserItem> server_user_item, const CMD_C_LockFish& lock_fish);
    bool OnSubUnlockFish(shared_ptr<CServerUserItem> server_user_item);
    bool OnSubFrozenFish(shared_ptr<CServerUserItem> server_user_item);
    bool OnSubAddLevel(shared_ptr<CServerUserItem> server_user_item,const CMD_C_AddLevel& addlevle,bool Sendother=false);
    bool OnSubGameplay(shared_ptr<CServerUserItem> server_user_item, const CMD_C_Gameplay& gamePlay, const unsigned char* data, WORD dataSize);
    float GetOverallProbability(int64_t currentstock,int64_t minstock,int64_t maxstock);
    float GetPersonalProbability(int64_t currentpersomstock);

    float GetSmallPersonalProbability(int64_t currentpersomstock);
    bool GetResult(PONEBKINFO pList ,tagPersonalProfit currentper,int64_t currentstock,int64_t minstock,int64_t maxstock,int32_t ncount);

    bool OnSendInfoToOther(shared_ptr<CServerUserItem> server_user_item);
    // bSynchorOnly : 飘海浪时同步分数.
    bool OnTimerSynchorScore();
    void CalcScore(shared_ptr<CServerUserItem> server_user_item, bool bSynchorOnly=false);
    void writeUserScore(WORD chair_id, double score);
    void writeUserInfo(int chairid);
    void writeUserInfo1(int chairid);
	void OnAddScore(const char* user_id, WORD table_id, ULONG64 score);
	void OnSubScore(const char* user_id, WORD table_id, ULONG64 score);

    void SendBossMessage(int chairid,string fishname,int64_t score);//跑马灯信息
	BulletScoreT getDivNumber()
	{
		const short FIGURES = 9;
		BulletScoreT value = 1;
		for (auto i = FIGURES; i > 0; --i) {
			BulletScoreT divNum = pow(10, i);
			if (game_config.cannonPowerMin / divNum) {
				value = divNum;
				break;
			}
		}
		return value;
	}


    double getExchangeUserScore(double fishScore) {
        return (fishScore * game_config.exchangeRatioFishScore) / (game_config.ticketRatio * game_config.exchangeRatioUserScore*PALYER_SCORE_SCALE);
 		//return fishScore;
	}
#ifdef __USE_LIFE2__
    void writeUserScore(WORD chair_id, double score, double ingot);
    double getExchangeUserIngot(long long score2) {
        return (double)score2 * game_config.exchangeRatioIngot / game_config.exchangeRatioScore2;
	}
#endif
    long long getExchangeFishScore(double userScore)
    {
        if (!game_config.exchangeRatioFishScore) {
             game_config.exchangeRatioFishScore = 1;
            LOG(ERROR) << "getExchangeFishScore game_config.exchangeRatioFishScore = 0!";
        }
        return (game_config.ticketRatio * game_config.exchangeRatioUserScore * userScore) / (game_config.exchangeRatioFishScore*PALYER_SCORE_SCALE);
 		//return userScore;
	}
	long long convertToFishCoin(long long fishScore) {
		return fishScore/* * m_coinRatioCoin*/ / (/*m_coinRatioScore * */game_config.ticketRatio);
	}
	long long convertToFishScore(long long fishCoin) {
		return fishCoin /** m_coinRatioScore */* game_config.ticketRatio/* / m_coinRatioCoin*/;
	}
    long convertToAicsScore(long fishScore)
    {
        if (!m_coinRatioCoin) {
            LOG(ERROR) << "convertToAicsScore m_coinRatioCoin=0,reset to 1.";
            m_coinRatioCoin=1;
        }
    //Cleanup:
        return ((fishScore * m_coinRatioScore) / m_coinRatioCoin);
	}
	void writeLog(const char* format, ...);
	WORD getRealChairID(WORD srcChairID) {
		WORD chairID = srcChairID;
#ifdef __DYNAMIC_CHAIR__
		WORD replacedChairID = game_config.dynamicChair[srcChairID];
		if (replacedChairID != INVALID_CHAIR) {
			chairID = replacedChairID;
		}
#endif
		return chairID;
	}

#ifdef __DYNAMIC_CHAIR__
private:
    bool OnSubDynamicChair(shared_ptr<CServerUserItem> server_user_item, const CMD_C_DynamicChair& dynamicChair) {
        WORD chair_id = server_user_item->GetChairId();
		WORD oldChairID = game_config.dynamicChair[chair_id];
		WORD newChairID = genDynamicChairID(chair_id, dynamicChair.newChairID);
        if (newChairID != INVALID_CHAIR)
        {
#ifdef AICS_ENGINE
            //char info[1024]={0};
            //snprintf(info,sizeof(info),"玩家昵称=%s,玩家ID=%d,房间号=%d,椅子号=%d)玩家起立!!!",
            //    server_user_item->GetNickName(), server_user_item->GetUserId(), m_pITableFrame->GetTableId(), oldChairID);
            //GetAicsEngine()->OnUserStandup(server_user_item->GetUserId(), m_pITableFrame->GetTableId(), oldChairID, FALSE, FALSE);
            //LOG(INFO) << info;

            //snprintf(info,sizeof(info),"玩家昵称=%s,玩家ID=%d,房间号=%d,椅子号=%d)玩家坐下!!!",
            //    server_user_item->GetNickName(), server_user_item->GetUserId(), m_pITableFrame->GetTableId(), newChairID);
            //GetAicsEngine()->OnUserSitdown(server_user_item->GetUserId(), m_pITableFrame->GetTableId(), newChairID, FALSE, FALSE);
            //LOG(INFO) << info;
#endif
			CMD_S_DynamicChair cmdData = {0};
			cmdData.newChairID = newChairID;
			cmdData.playerID = chair_id;
			unsigned short dataSize = NetworkUtility::encodeDynamicChair(m_send_buffer, CMD_BUFFER_SIZE_MAX, cmdData);
			send_table_data(INVALID_CHAIR, INVALID_CHAIR, SUB_S_DYNAMIC_CHAIR, m_send_buffer, dataSize, true);
			doReturnBulletScore(chair_id);
		}
		return true;
	}
	void resetDynamicChair(WORD chairID = INVALID_CHAIR) {
		if (chairID == INVALID_CHAIR) {
			for (int i = 0; i < MAX_PLAYERS; ++i) {
				game_config.dynamicChair[i] = INVALID_CHAIR;
				m_usedChairIDs[i] = FALSE;
			}
		} else {
			m_usedChairIDs[game_config.dynamicChair[chairID]] = FALSE;
			game_config.dynamicChair[chairID] = INVALID_CHAIR;
		}
	}
	WORD genDynamicChairID(WORD chairID, WORD newChairID = INVALID_CHAIR) {
		WORD tempChairID = INVALID_CHAIR;
		WORD dynamicChairID = game_config.dynamicChair[chairID];
		if (newChairID == INVALID_CHAIR) {
            if (dynamicChairID == INVALID_CHAIR)
            {
                if (m_usedChairIDs[chairID])
                {
                    for (int i = 0; i < MAX_PLAYERS; ++i)
                    {
						if (m_usedChairIDs[i]) {
							tempChairID = i;
							break;
						}
					}
				} else {
					tempChairID = chairID;
				}
			} else {
				return dynamicChairID;
			}
		} else {
			if ((dynamicChairID == INVALID_CHAIR && chairID != newChairID) ||
				dynamicChairID != newChairID) {
				if (!m_usedChairIDs[newChairID]) {
					tempChairID = newChairID;
				}
			}
		}
		if (tempChairID != INVALID_CHAIR) {
			resetDynamicChair(chairID);
			m_usedChairIDs[tempChairID] = TRUE;
			game_config.dynamicChair[chairID] = tempChairID;
		}
		return tempChairID;
	}
	BOOL m_usedChairIDs[MAX_PLAYERS];
#endif

private:
	void resetRumtimeWashData(WORD chairId);
	void sendWashPercentData(WORD chairId, WashPercent percent);
	LONG getAwardFromArithPool();
	void onPlayerFired(WORD tableID, WORD chairId, int fireScore);
	void onFishCatched(WORD tableID, WORD chairId, LONG score);
	void onWashUpdated(WORD chairId, WashPercent percent);
	void onWashArrived(WORD chairId);
	void readXmlWashData(CXmlParser& xml);
	void doReturnBulletScore(WORD chair_id);
    bool ReadConfigInformation();



    template<typename T>
    std::vector<T> to_array(const std::string& s)
    {
        std::vector<T> result;
        std::stringstream ss(s);
        std::string item;
        while (std::getline(ss, item, ','))
        {
            result.push_back(boost::lexical_cast<T>(item));
        }
        return result;
    }
#ifdef __USE_LIFE2__
	void writeArithmLog1(WORD chair_id) {
		long long winScore = game_config.fishScore[chair_id] + game_config.fishScore2[chair_id] - game_config.exchangeFishScore[chair_id];
		m_traceArithm->updateData(game_service_option_->wServerID, m_pITableFrame->GetTableID(), chair_id, winScore, 0, 1);
	}
	void writeArithmLog2(WORD chair_id, long long exchangeProfit) {
		m_traceArithm->updateData(game_service_option_->wServerID, m_pITableFrame->GetTableID(), chair_id, 0, exchangeProfit, 2);
	}
#endif
private:
	void sendGameplayData(WORD me_chair_id, WORD chairId, BYTE gameplayId, BYTE commandType,
    WORD excludeChairId = INVALID_CHAIR, void* addiData = NULL, WORD addiDataSize = 0);
    LPCTSTR getNickName(shared_ptr<CServerUserItem> server_user_item);
    LPCTSTR getServerName()
    {
        std::string name = "游戏中";
 //     if (m_pGameRoomInfo) {
 //         name = ("m_pITableFrame->()");----
 //     }
 // //Cleanup:
        return name.c_str();
	}

	void AddFishScoreForPlayer(WORD chairId, ScoreUpType type, LONG score, WORD exclude_chair_id = INVALID_CHAIR);
    inline int64_t GetTickCount()
    {
        timeval tv;
        gettimeofday(&tv,NULL);
        int64_t tick = (int64_t)(((tv.tv_sec*1000)+(tv.tv_usec/1000)));
        return (tick);
    }
#ifdef __USE_LIFE2__
	void AddFishScoreForPlayer2(WORD chairId, ScoreUpType type, LONG score, LONG score2);
#endif

    // get the exchange score text content.
	LPCTSTR getExchangeScoreText(LONG score)
	{
#ifdef __DFW__
		wsprintf(m_scoreText, _T("%d"), score);
#else
		wsprintf(m_scoreText, m_exchangeScoreFormat, getExchangeUserScore(score));
#endif
		return m_scoreText;
	}
	void setExchangeScoreFormat(unsigned char decimalPlaces)
	{
		wsprintf(m_exchangeScoreFormat, _T("%%.0%df"), decimalPlaces);
	}
	DWORD getXimaliangForNewOldUser()
	{
		static int currentIndex = 0;
		if (m_gainForNewOldUser.size()) {
			int idx = currentIndex;
			currentIndex = ++currentIndex % m_gainForNewOldUser.size();
			return m_gainForNewOldUser[idx];
		}
		return 0;
	}
	DWORD getWinCoinForNewOldUser()
	{
		static int currentIndex = 0;
		if (m_winCoinForNewOldUser.size()) {
			int idx = currentIndex;
			currentIndex = ++currentIndex % m_winCoinForNewOldUser.size();
			return m_winCoinForNewOldUser[idx];
		}
		return 0;
	}
	void frozenAllFish(WORD chair_id);

private:
	void SendAllRoomMessage(LPCTSTR format, ...);
	//设置当局游戏详情
	void SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, int64_t winScore, PlayLog playlog);
private:
    std::shared_ptr<ITableFrame>    m_pITableFrame;
    tagGameRoomInfo*				m_pGameRoomInfo;   // chaircount = startmaxplayer.

#ifdef __MULTI_THREAD__
	CRITICAL_SECTION				m_cs;
	CRITICAL_SECTION				m_csSend;
#endif

	short							m_bulletCount[MAX_PLAYERS];
	BulletScoreT					m_bullets[MAX_PLAYERS][MAX_BULLETS];

	DECLARE_FISH_ARRAY(FISH_ID, m_fishIDs);
	DECLARE_FISH_ARRAY(FISH_ID, m_usedFishIDs);
	DECLARE_FISH_ARRAY(FishKind, m_catchFishKindList);
	DECLARE_CMD_BUFFER(m_send_buffer);
	DECLARE_CMD_BUFFER(m_send_buffer_for_timer_thread);

	int								exchange_count_;
	int								exchangeAmountForExperienceRoom;
	CMD_S_GameConfig				game_config;
	bool							game_started;
    int								aics_kindid;
    int64_t							sceneTimeLeft;
    bool							isGroupScene;
	CSceneManager					scene_mgr;
	WORD							wSwitchSceneTimerElasped;
	FishProduceManager				fish_mgr;
	FishProduceDataManager*			m_fishProduceDataManager;
	FishArrayBase*					m_currentFishArray;
	FishArrayManager				m_fishArrayManager;
    shared_ptr<CServerUserItem>		m_pServerUserItemForGamescene[MAX_PLAYERS];
	bool							m_is_switching_scene;
    bool							m_is_nestScene;
	ONEBKILLINFO					m_kill_info[500];
 	//ILogdbEngine*					m_i_log_db_engine;
    double							m_exp_user_score[MAX_PLAYERS];
	WashXmlConfiguration			m_washXmlConfig;
	RumtimeWashData					m_rumtimeWashData[MAX_PLAYERS];
	WashPercent						m_washPercent[MAX_PLAYERS];
	TCHAR							m_scoreText[1024];
	TCHAR							m_exchangeScoreFormat[20];
    double							lostScore[MAX_PLAYERS];
    double							sumScore[MAX_PLAYERS];
    long long						playerScore[MAX_PLAYERS];

    double							playerAddScore[MAX_PLAYERS];
	//PlayerScoreInfo					m_playerScoreInfo[MAX_PLAYERS];

    static vector<std::shared_ptr<ITableFrame>> tableFrameList;
	static WORD						instanceCount;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
    std::vector<int>				deathfishs;
#ifdef __GAMEPLAY_COLLECT__
	DWORD getAdditionalCollectFishLife(WORD chairId, BYTE collectStats, int fireScore)
	{
		DWORD fishLife = 0;
		if (collectStats) {
			collectStats &= ~game_config.collectData.collectStats[chairId];
			BYTE collectStatsMerge = game_config.collectData.collectStats[chairId] | collectStats;
			if (IsCollectComplete(collectStatsMerge, game_config.collectData.collectCount)) {
				fishLife = static_cast<DWORD>((m_collectTotalFishScore[chairId] + 0.0f) / fireScore + 0.501f) +
					m_collectGiftingMultiple[chairId] * m_collectUtility.getCollectFishCount(collectStats);
			}
		}
		return fishLife;
	}
	void setPlayerCollectStats(WORD chairId, short collectIndex, int fishScore)
	{
		if (IsCollectFish(collectIndex) && !GetCollectStats(game_config.collectData.collectStats[chairId], collectIndex)) {
			SetCollectStats(game_config.collectData.collectStats[chairId], collectIndex);
			m_collectTotalFishScore[chairId] += m_collectGiftingMultiple[chairId] * fishScore;
// 			TRACE_INFO(_T("CTableFrameSink::setCollectStats[table_id=%d chair_id=%d stats=0x%02x]"), m_pITableFrame->GetTableID(), chairId, game_config.collectData.collectStats[chairId]);
		}
	}
	void resetPlayerCollectStats(WORD chairId)
	{
		m_collectTotalFishScore[chairId] = 0;
		game_config.collectData.collectStats[chairId] = 0;
		m_collectGiftingMultiple[chairId] = Random::getInstance()->RandomInt(m_collectGiftingMultipleMin, m_collectGiftingMultipleMax);
	}
	BYTE							m_collectGiftingMultipleMin, m_collectGiftingMultipleMax; // 每收集一项送的倍率区间
	BYTE							m_collectGiftingMultiple[MAX_PLAYERS];
	DWORD							m_collectTotalFishScore[MAX_PLAYERS]; // 累积赠送分数
	CCollectUtility					m_collectUtility;
#endif
#ifdef __GAMEPLAY_MSYQ__
	WORD							m_msyqCopies;
	LONG							m_msyqAward[MAX_PLAYERS];
	DWORD							m_msyqSumAward[MAX_PLAYERS];
	LONG							m_msyqAverageAward[MAX_PLAYERS];
#endif
#ifdef __GAMEPLAY_PAIPAILE__
	PplRuntimeData					m_pplRuntimeData[MAX_PLAYERS];
	void resetPplData(WORD chairId)
	{
		memset(&game_config.pplData[chairId], 0, sizeof(game_config.pplData[chairId]));
		memset(&m_pplRuntimeData[chairId], 0, sizeof(m_pplRuntimeData[chairId]));
	}
#endif


protected:
    vector<int32_t> controlledProxy;
    float           controllPoint;
    vector<int64_t>  personalProfitLevel;
    vector<float>  personalPribirity;

    int64_t     smallOrBigLevelbet;
    vector<int64_t>  smallPersonalLevel;
    vector<float>    smallPersonalPribirity;

    vector<int64_t> overallProfit;
    vector<float> overallProbirity;

    float fairProbability;

    int32_t fishRake;
    int32_t Resurrection;
 	//DWORD							m_debugTickProduce;
 	//DWORD							m_debugTickQuit[MAX_PLAYERS];
 	//static UserPrefs*				m_userPrefs;
	vector<DWORD>					m_gainForNewOldUser;
	vector<DWORD>					m_winCoinForNewOldUser;
	DWORD							m_beginUserID;
	WORD							m_coinRatioCoin, m_coinRatioScore;
	DWORD							m_currentSN[MAX_PLAYERS];
    //qidong dingshiqi
    muduo::net::TimerId             m_TimeridFISH_PRODUCE;                     //create fish
    muduo::net::TimerId             m_TimerIdSCENE_SWITCH;                     // scene change.
    muduo::net::TimerId             m_TimerIdFISH_ARRAY;                      //fish special array
    muduo::net::TimerId             m_TimerIdPauseBoom;                  //fish pause bomb
    muduo::net::TimerId             m_TimerIdWriteBgData;                  // write date
    muduo::net::TimerId             m_TimerIdSynchorScore;                  //synchor score
    muduo::net::TimerId             m_TimerIdClearUser;                  //synchor score
    muduo::net::TimerId             m_TimerIdClearNotOperateUser;                  //synchor score

	muduo::net::TimerId             m_TimeridFireLaser;                       //fire laser
	muduo::net::TimerId             m_TimeridBorebitStartRotate;               //钻头炮开始旋转
	muduo::net::TimerId             m_TimeridFireBorebit;                     //create fish
	muduo::net::TimerId             m_TimeridCountDownFire;                     //create fish
	muduo::net::TimerId             m_TimeridFireBorebitEnd;                     //钻头炮爆炸结束

    void OnTimerProduceFish();
    void OnTimerSwitchFish();
    void OnTimerFishArray();
    void OnTimerPauseBomb();
    void OnTimerBgData();
    void OnTimerSynchor();
    void OnTimerClearUser();
    void OnTimerClearNotOperateUser();
	void OnTimerFireLaser(WORD chair_id);
	void OnTimerBorebitStartRotate(WORD chair_id); //钻头炮开始旋转
	void OnTimerFireBorebit(WORD chair_id);
	void OnTimerCountDownFire(WORD chair_id);
	void OnTimerFireBorebitEnd(WORD chair_id);

    int32_t fishCal;
    int32_t fishWinScore;


    chrono::system_clock::time_point			m_startTime;           
    float										fpalyerReventPercent;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;
    string										strRoundID[MAX_PLAYERS];
    string										OldstrRoundID[MAX_PLAYERS];
    int											bulletlevel[MAX_PLAYERS];
    bool										isNotOutRoom[MAX_PLAYERS];
    tagGameReplay								m_replay;
    int											notOperate[MAX_PLAYERS];
    PlayLog										playerLog[MAX_PLAYERS];

	int32_t										m_startChangSceneTime;				//切换场景的开始时间

	bool 										m_bHaveLaser[MAX_PLAYERS];			//是否获得了激光炮
	bool 										m_bHaveFireLaser[MAX_PLAYERS];		//是否发射了激光炮
	int32_t										m_nBaseLaserOdds;					//激光炮的基础倍数
	int32_t										m_nLessLaserOdds;					//激光炮除了基础倍数外剩余的倍数
	
	bool 										m_bHaveBorebit[MAX_PLAYERS];		//是否获得了钻头炮
	bool 										m_bHaveFireBorebit[MAX_PLAYERS];	//是否发射了钻头炮
	int32_t										m_nBaseBorebitOdds;					//钻头炮的基础倍数
	int32_t										m_nLessBorebitOdds;					//激光炮除了基础倍数外剩余的倍数

	int32_t										m_startFireCountTime;				//准备发射倒计时
	muduo::Timestamp							m_startSwitchSceneTime;				//切换场景的开始时间
	muduo::Timestamp							m_startFireTime;					//发射激光炮钻头炮的开始时间
	muduo::Timestamp							m_startExtendSwitchSceneTime;		//获得钻头炮时开始延长切换场景的时间
	short										m_startFireAngle;					//发射的角度
	BulletScoreT								m_fireScore[MAX_PLAYERS];			//开炮炮值
	LONG										m_fireAllWinScore[MAX_PLAYERS];		//累计总赢的分值
	int32_t										m_nfireMaxOdds;						//激光炮钻头炮最多赢的倍数
	int32_t										m_nAddHitFishOdds;					//激光炮钻头炮的累计倍数

	int32_t										m_nHitFishBoomOdds;					//钻头炮爆炸预留的倍数
	double										m_nBorebitMinFireTime;				//钻头炮最小飞行时间 10s
	double										m_nBorebitMaxFireTime;				//钻头炮最大飞行时间 30s
	double										m_nBorebitFireAllTime;				//钻头炮本次飞行的总时间
	bool										m_bBorebitFireBoom;					//钻头炮是否爆炸
	bool										m_bBorebitStartRotate;				//钻头炮是否开始旋转
	bool										m_isNeedExtendSwitchScene;			//是否需要延长切换场景
	DWORD										m_dwNeedAddTick;					//需要延长切换场景的时间
	DWORD										m_dwSceneLessTime;					//剩余切换场景的时间
	int32_t										m_SpecialKillFishProbili_JG[4];		//激光炮 大>=100,中>=50,小>=25 倍鱼
	int32_t										m_SpecialKillFishProbili_ZT[4];		//钻头炮 大>=100,中>=50,小>=25 倍鱼
	bool										m_bCatchSpecialFireInGlobalBomb[MAX_PLAYERS];	//捕获全屏炸弹，是否包含特殊炮   
    //thread.
#ifdef __USE_LIFE2__
	static ArithmLog*							m_traceArithm;
#endif
};

#endif // TABLE_FRAME_SINK_H_
