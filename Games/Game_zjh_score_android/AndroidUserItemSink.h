#pragma once

#ifndef isZero
#define isZero(a)        (((a)>-0.000001) && ((a)<0.000001))
#endif//isZero

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)//底注
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)//顶注
#define CellScore		(FloorScore)//底注
#define JettionList     (m_pTableFrame->GetGameRoomInfo()->jettons)//筹码表

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
//#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

#define ThisChairId		(m_pAndroidUserItem->GetChairId())
#define ThisUserId		(m_pAndroidUserItem->GetUserId())
#define ThisUserScore	(m_pAndroidUserItem->GetUserScore())

#define ByChairId(chairId)	(m_pTableFrame->GetTableUserItem(chairId))
#define ByUserId(userId)	(m_pTableFrame->GetUserItemByUserId(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

#define StockScore (storageInfo_.lEndStorage) //系统当前库存
#define StockLowLimit (storageInfo_.lLowlimit)//系统输分不得低于库存下限，否则赢分
#define StockHighLimit (storageInfo_.lUplimit)//系统赢分不得大于库存上限，否则输分

//配置文件
#define INI_FILENAME "./conf/zjh_config.ini"

//操作类型
enum Wait_Operate {
	OP_INVALID,				//无效
	OP_LOOKOVER,            //看牌
	OP_GIVEUP,              //弃牌
	OP_FOLLOW,              //跟注
	OP_ADD,                 //加注
	OP_COMPARE,             //比牌
	OP_ALLIN,				//孤注一掷
	OP_RUSH,                //火拼
	OP_MAX,
};

enum EstimateTy {
	EstimateAll,	//当前最大牌用户
	EstimateAndroid,//机器人中最大牌用户
	EstimatePlayer,	//真实玩家中最大牌用户
};

class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
	virtual ~CAndroidUserItemSink();
public:
	//桌子重置
	virtual bool RepositionSink();
	//桌子初始化
	virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
	//用户指针
	virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);
	//桌子指针
	virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
	//消息处理
	virtual bool OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize);
	//机器人策略
	virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy) { strategy_ = strategy; }
	virtual tagAndroidStrategyParam* GetAndroidStrategy() { return strategy_; }
protected:
	//随机等待时间
	double randWaitSeconds(int chairId, int32_t delay = 0, int32_t wTimeLeft = -1);
	//等待操作定时器
	void onTimerWaitingOver();
	//清理所有定时器
	void ClearAllTimer();
	//推断当前最大牌用户
	void EstimateWinner(EstimateTy ty = EstimateAll);
	//返回决策(0,1,2)
	int GetSPIndex();
	//推断当前操作
	void EstimateOperate(int SPIndex);	
	//等待机器人操作
	void waitingOperate(int32_t delay = 0, int32_t wTimeLeft = -1);
	//读取机器人配置
	void ReadConfig();
	//读取操作配置概率
	void ReadOperateConfig();
	//牌型获取胜率和权值
	std::pair<double, double>* GetWinRatioAndWeight();
	//反序列化机器人配置
	void UnserialConfig(std::vector<std::pair<double, double>> const& robot);
	//剩余游戏人数
	int leftPlayerCount(bool includeAndroid = true, uint32_t* currentUser = NULL);
	//返回有效用户
	uint32_t GetNextUser(uint32_t startUser, bool ignoreCurrentUser = true);

	//推断当前操作
	void EstimateOperate_New(int SPIndex);
	//是否大散牌
	bool IsBigSanCard(uint8_t cards[]);
	//是否大对子
	bool IsBigDuizi(uint8_t cards[]);
	//获取当前操作
	void calculateCurrentOperate(uint32_t chairId);
	//获取可加注的Index
	int getAddIndex(int start);
protected:
	//累计等待时长
	//double totalWaitSeconds_;
	//分片等待时长
	double sliceWaitSeconds_;
	//随机等待时长
	double randWaitSeconds_;
	//重置机器人等待时长
	//double resetWaitSeconds_;
	//是否已重置等待时长
	//bool randWaitResetFlag_;
protected:
	//机器人牌 - 服务端广播
	bool onAndroidCard(const void * pBuffer, int32_t wDataSize);
	//游戏开始 - 服务端广播
	bool onGameStart(const void * pBuffer, int32_t wDataSize);
	//看牌结果 - 服务端回复
	bool resultLookCard(const void * pBuffer, int32_t wDataSize);
	//弃牌结果 - 服务端回复
	bool resultGiveup(const void * pBuffer, int32_t wDataSize);
	//跟注/加注结果 - 服务端回复
	bool resultFollowAdd(const void * pBuffer, int32_t wDataSize);
	//比牌结果 - 服务端回复
	bool resultCompareCard(const void * pBuffer, int32_t wDataSize);
	//孤注一掷结果 - 服务端回复
	bool resultAllIn(const void * pBuffer, int32_t wDataSize);
	//游戏结束 - 服务端广播
	bool onGameEnd(const void * pBuffer, int32_t wDataSize);
public:
	//请求操作
	bool sendOperateReq();
	//请求看牌
	void sendLookCardReq();
	//请求弃牌
	void sendGiveupReq();
	//请求跟注
	void sendFollowReq();
	//请求加注
	void sendAddReq();
	//请求比牌
	void sendCompareCardReq();
private:
	//牌局编号
	std::string strRoundID_;
	//游戏逻辑
	ZJH::CGameLogic g;
	//玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//手牌牌型
	uint8_t handCardsType_[GAME_PLAYER];
	//是机器人
	bool isAndroidUser_[GAME_PLAYER];
	//庄家用户
	uint32_t bankerUser_;
	//操作用户
	uint32_t currentUser_;
	//首发用户
	uint32_t firstUser_;
	//当前最大牌用户
	uint32_t currentWinUser_;
	//真实玩家中最大牌用户
	uint32_t realWinUser_;
	//机器人中最大牌用户
	uint32_t androidWinUser_;
	//当前第几轮(0,1,2,3,...)
	int32_t currentTurn_;
	struct turninfo_t {
		//本轮开始用户
		uint8_t startuser;
		//本轮结束用户
		uint8_t enduser;
		//本轮是否有加注过
		bool addflag;
	}turninfo_[MAX_ROUND];
	//操作总的时间/剩余时间
	uint32_t opTotalTime_;
	//操作开始时间
	uint32_t opStartTime_;
	//操作结束时间
	uint32_t opEndTime_;
	//是否看牌
	bool isLooked_[GAME_PLAYER];
	//是否弃牌
	bool isGiveup_[GAME_PLAYER];
	//是否比牌输
	bool isComparedLost_[GAME_PLAYER];
	//是否诈唬
	bool bluffFlag_;
	//桌子指针
	shared_ptr<ITableFrame> m_pTableFrame;
	//机器人对象
	shared_ptr<IServerUserItem>	m_pAndroidUserItem;
	//定时器指针
	shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;
	//等待定时器
	muduo::net::TimerId timerIdWaiting_;
	//机器人操作
	Wait_Operate currentOp_;
	//机器人决策(0,1,2)
	int SPIndex_;
	//机器人配置
	robotconfig_t robot_;
	//判断特殊牌型
	bool special235_;
	//开启随时看牌
	bool freeLookover_;
	//开启随时弃牌
	bool freeGiveup_;
	//启用三套决策(0,1,2)
	bool useStrategy_;
	STD::Random m_random;
private:
	std::default_random_engine rengine;
	std::uniform_real_distribution<float> range_K[3];
	std::uniform_real_distribution<float> range_W1[3];
	std::uniform_real_distribution<float> range_W2[3];
private:
	//当前筹码值
	int64_t jettonValue_;
	//基础跟注分
	int64_t followScore_;
private:
	//判断当前能否加注操作
	//start int& 筹码表可加注筹码起始索引
	//minaddscore int64_t* 基础最小加注分
	inline int GetCurrentAddStart(int64_t* minaddscore = NULL) {
		assert(followScore_ > 0);
		//可加注筹码表[5,10,15,20,25]
		//JettionList;
		//筹码表可加注筹码起始索引
		int start = -1;
		//初始底注 cellScore = JettionList[0] = 5
		//followScore_ = jettonValue_ = cellScore
		for (int i = 0; i < JettionList.size(); ++i) {
			//基础跟注分
			if (followScore_ == JettionList[i]) {
				//可加注筹码起始索引
				start = ++i;			
				break;
			}
		}
		//可加注筹码索引范围[start, JettionList.size())
		if (start > 0 && start < JettionList.size()) {
			//筹码表各项可加注筹码值大于基础跟注分
			assert(JettionList[start] > followScore_);
			for (int i = start + 1; i < JettionList.size(); ++i) {
				assert(JettionList[i - 1] < JettionList[i]);
			}
			//最小加注分
			if (minaddscore != NULL) {
				*minaddscore = JettionList[start];
			}
			return start;
		}
		return start = -1;
	}
	// 获取加注的筹码
	inline int GetAddChipStart(int64_t* minaddscore = NULL) {
		assert(followScore_ > 0);
		//可加注筹码表[5,10,15,20,25]
		//JettionList;
		//筹码表可加注筹码起始索引
		int start = -1;
		//初始底注 cellScore = JettionList[0] = 5
		//followScore_ = jettonValue_ = cellScore
		for (int i = 0; i < JettionList.size(); ++i) {
			//基础跟注分
			if (followScore_ == JettionList[i]) {
				//可加注筹码起始索引
				LOG(ERROR) << "===== 加注 [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " <<  " 玩家 " << currentUser_ << " 当前下标 " << i << " 当前筹码:" << JettionList[i];
				start = getAddIndex(++i);	
				LOG(ERROR) << "===== 加注 [" << strRoundID_ << "] 第[" << currentTurn_ << "]轮 " <<  " 玩家 " << currentUser_ << " 加注下标 " << start << " 加注筹码:" << JettionList[start];			
				break;
			}
		}
		//可加注筹码索引范围[start, JettionList.size())
		if (start > 0 && start < JettionList.size()) {
			//筹码表各项可加注筹码值大于基础跟注分
			assert(JettionList[start] > followScore_);
			for (int i = start + 1; i < JettionList.size(); ++i) {
				assert(JettionList[i - 1] < JettionList[i]);
			}
			//最小加注分
			if (minaddscore != NULL) {
				*minaddscore = JettionList[start];
			}
			return start;
		}
		return start = -1;
	}

	//加注操作更新当前筹码值
	//加注操作更新基础跟注分
	//start int 筹码表可加注筹码起始索引
	//index int 指定可加注筹码索引
	inline void selectAddIndex(int start, int index) {
		assert(start > 0);
		assert(index >= start);
		assert(index < JettionList.size());
		//当前筹码值
		jettonValue_ = JettionList[index];
		//基础跟注分
		followScore_ = jettonValue_;
	}

	//当前筹码表
	inline void CurrentJettionList(uint8_t chairId, std::vector<int64_t>& jettons) {
		jettons.clear();
		for (std::vector<int64_t>::const_iterator it = JettionList.begin();
			it != JettionList.end(); ++it) {
			jettons.push_back(isLooked_[chairId] ? (*it * 2) : *it);
		}
	}

	//最小加注分
	//minaddscore int64_t 基础最小加注分
	inline int64_t CurrentMinAddScore(uint8_t chairId, int64_t minaddscore) {
		return isLooked_[chairId] ? (minaddscore * 2) : minaddscore;
	}

	//当前加注分
	inline int64_t CurrentAddScore(uint8_t chairId) {
		return CurrentFollowScore(chairId);
	}

	//当前跟注分(未看牌，基础跟注分)
	//当前跟注分(看牌了，基础跟注分X2)
	inline int64_t CurrentFollowScore(uint8_t chairId) {
		return isLooked_[chairId] ? (followScore_ * 2) : followScore_;
	}

	//比牌加注分(当前跟注分X2)
	inline int64_t CurrentCompareScore(uint8_t chairId) {
		return CurrentFollowScore(chairId) * 2;
	}
	inline int GetCurrentJettionIndex() {
		assert(followScore_ > 0);
		//可加注筹码表[5,10,15,20,25]
		//JettionList;
		//筹码表可加注筹码起始索引
		int start = -1;
		//初始底注 cellScore = JettionList[0] = 5
		//followScore_ = jettonValue_ = cellScore
		for (int i = 0; i < JettionList.size(); ++i) {
			//基础跟注分
			if (followScore_ == JettionList[i]) {
				//可加注筹码起始索引
				start = i;
				break;
			}
		}
		//可加注筹码索引范围[start, JettionList.size())
		if (start > 0 && start < JettionList.size()) {
			//筹码表各项可加注筹码值大于基础跟注分
			assert(JettionList[start] == followScore_);
			return start;
		}
		return start = -1;
	}
private:
	//各玩家下注
	int64_t tableScore_[GAME_PLAYER];
	//桌面总下注
	int64_t tableAllScore_;
	//系统当前库存
	tagStorageInfo storageInfo_;
	//机器人策略
	tagAndroidStrategyParam* strategy_;
};