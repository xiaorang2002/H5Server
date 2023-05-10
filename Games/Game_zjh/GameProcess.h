#pragma once

#define GAME_STATUS_INIT                        GameStatus::GAME_STATUS_INIT //初始状态
#define GAME_STATUS_READY						GameStatus::GAME_STATUS_FREE //空闲状态
#define GAME_STATUS_PLAYING                     GameStatus::GAME_STATUS_START//进行状态
#define GAME_STATUS_END				    	    GameStatus::GAME_STATUS_END  //游戏结束

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)//底注
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)//顶注
#define CellScore		(FloorScore)//底注
#define JettionList     (m_pTableFrame->GetGameRoomInfo()->jettons)//筹码表

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisGameId		(m_pTableFrame->GetGameRoomInfo()->gameId)
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
//#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

#define EnterMinScore (m_pTableFrame->GetGameRoomInfo()->enterMinScore)//进入最小分
#define EnterMaxScore (m_pTableFrame->GetGameRoomInfo()->enterMaxScore)//进入最大分

#define ByChairId(chairId)	(m_pTableFrame->GetTableUserItem(chairId))
#define ByUserId(userId)	(m_pTableFrame->GetUserItemByUserId(userId))

#define UserIdBy(chairId) ByChairId(chairId)->GetUserId()
#define ChairIdBy(userId) ByUserId(userId)->GetChairId()

#define ScoreByChairId(chairId) ByChairId(chairId)->GetUserScore()
#define ScoreByUserId(userId) ByUserId(userId)->GetUserScore()

#define StockScore (storageInfo_.lEndStorage) //系统当前库存
#define StockLowLimit (storageInfo_.lLowlimit)//系统输分不得低于库存下限，否则赢分
#define StockHighLimit (storageInfo_.lUplimit)//系统赢分不得大于库存上限，否则输分

//赋值tagSpecialScoreInfo基础数据
#define SetScoreInfoBase(scoreInfo, x, p) { \
	assert(scoreInfo.chairId == (x)); \
	if(!(p)) { \
		assert(ByChairId(x)); \
		shared_ptr<CServerUserItem> userItem = ByChairId(x); \
		scoreInfo.userId = userItem->GetUserId(); \
		scoreInfo.account = userItem->GetAccount(); \
		scoreInfo.agentId = userItem->GetUserBaseInfo().agentId; \
        scoreInfo.lineCode = userItem->GetUserBaseInfo().lineCode; \
		scoreInfo.beforeScore = ScoreByChairId(x); \
	} \
	else { \
		assert(dynamic_cast<userinfo_t*>((userinfo_t*)(p))); \
		scoreInfo.userId = ((userinfo_t*)(p))->userId; \
		scoreInfo.account = ((userinfo_t*)(p))->account; \
		scoreInfo.agentId = ((userinfo_t*)(p))->agentId; \
        scoreInfo.lineCode = ((userinfo_t*)(p))->lineCode; \
		scoreInfo.beforeScore = ((userinfo_t*)(p))->userScore; \
	} \
}

//配置文件
#define INI_FILENAME "./conf/zjh_config.ini"
#define INI_CARDLIST "./conf/zjh_cardList.ini"

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

//玩家控制信息
struct ControlInfo {
	int64_t curWin;					//累计赢的分数
	int64_t conScore;				//当前需控制锁定的值【总赢的(0.5f~1.0f)】
	int32_t curWeight;				//对应锁定的干涉概率
	int32_t curIndex;				//当前控制的等级
	vector<int32_t> kcBaseTimes;    //各等级的基础的倍数
	vector<int32_t> kcweights;   	//各等级的干涉概率
};

//游戏流程
class CGameProcess : public ITableFrameSink
{
public:
    CGameProcess(void);
    ~CGameProcess(void);
public:
	void openLog(const char *p, ...);
	//游戏开始
	virtual void OnGameStart();
	//游戏结束
	virtual bool OnEventGameConclude(uint32_t dchairId, uint8_t GETag);
	//发送场景
	virtual bool OnEventGameScene(uint32_t dchairId, bool bIsLookUser);
	//游戏消息
	virtual bool OnGameMessage(uint32_t chairId, uint8_t subid, const uint8_t* data, uint32_t datasize);
	//用户进入
	virtual bool OnUserEnter(int64_t dwUserID, bool bIsLookUser);
	//用户准备
	virtual bool OnUserReady(int64_t dwUserID, bool bIsLookUser);
	//用户离开
	virtual bool OnUserLeft(int64_t dwUserID, bool bIsLookUser);
	//能否加入
	virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
	//能否离开
	virtual bool CanLeftTable(int64_t userId);
	//设置指针
	virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
private:
	//计算机器人税收
	int64_t CalculateAndroidRevenue(int64_t score);
    //清理游戏数据
    void ClearGameData();
    //初始化游戏数据
    void InitGameData();
    //清除所有定时器
    inline void ClearAllTimer();
	//剩余游戏人数
	int leftPlayerCount(bool includeAndroid = true, uint32_t* currentUser = NULL);
	//下一个操作用户
	uint32_t GetNextUser(bool& newflag, bool& exceed);
	//返回有效用户
	uint32_t GetNextUser(uint32_t startUser, bool ignoreCurrentUser = true);
	//牌型获取胜率和权值
	std::pair<double, double>* GetWinRatioAndWeight(uint32_t chairId);
	//序列化机器人配置
	void SerialConfig(std::vector<std::pair<double, double>>& robot);
private:
	//等待操作定时器
    void onTimerWaitingOver();
	//游戏结束，清理数据
    void OnTimerGameEnd();
	//用户看牌
	bool OnUserLookCard(int chairId);
	//用户弃牌
	bool OnUserGiveUp(int chairId, bool timeout = false);
	//用户跟注/加注
	bool OnUserAddScore(int chairId, int32_t index);
	//用户比牌
	bool OnUserCompareCard(int chairId, int nextChairId);
    //孤注一掷
    bool OnUserAllIn(int chairId, int startChairId);
	//防超时弃牌
	bool OnUserTimeoutGiveUpSetting(int chairId,  bool giveup);
	//超出最大轮数限制，强制比牌
	bool OnUserCompareCardForce();
	//是否大散牌
	bool IsBigSanCard(uint8_t cards[]);
	//是否大对子
	bool IsBigDuizi(uint8_t cards[]);
	//获取造牌的类型Index
	uint8_t getMadeCardTypeIndex(int ratio[],int32_t curIndex = 0);
	//计算控制的等级
	void CalculateControlLv(int chairId, int64_t allWinscore);
	//分析是否剩下最后一个机器人比牌了
	void AnalyseLastAndroidUser(int chairId, int nextChairId);
	//如果最大手牌是豹子,就不换牌了，如果是大顺金，少于8轮也不换牌
	bool isNeedChangeCards();
	//获取当前的控制等级
	int GetCurrentControlLv(int chairId,int curTime);
	//是否剩下最后一个机器人
	int IsLastAndroidUser();
	string getCardString(uint8_t cards[]);
	//获取当前赢家
	uint32_t getWinUser();
	//制造两手好牌
	void getTwoGoodCard();
protected:
    //读取库存配置
    void ReadStorageInformation();
	//读取机器人配置
	void ReadConfig();
	//更新机器人配置
	void UpdateConfig();
	//更新控制的配置
	void UpdateControlConfig();
private:
	////////////////////////////////////////////////////////////////////////////////
	//匹配规则文档：svn://Document/Design/策划文档/通用类需求/游戏匹配机制.doc
	//修改前即使用户量上来了也还是无法保证没有机器人参与进来
	//修改后用户量上来了肯定是没有机器人参与的，只要玩家在3.6s(可配置)之前匹配到真实用户(肯定的)
	//会立即进入游戏桌子，没有一点延迟时间，游戏秒开
	//有一种情况是，刚好概率随机到当前是个5人局，已经有3个人在匹配了，也就是还差2个人才会开始游戏，
	//正常情况是3.6s(可配置)超时之后若人数不足由机器人补齐空位，但是若机器人数量不足或被关闭掉了，
	//就会始终都处于匹配等待状态，无法进入游戏桌子，所以处理这种情况的办法是3.6s(可配置)超时之后
	//由机器人候补空位后判断仍然没有达到游戏要求的5个人时，只要达到最小游戏人数(MIN_GAME_PLAYER)即可进入游戏
	//否则继续等待玩家进入游戏桌子(此时机器人数量肯定不足)
	//不过有一种情况可能匹配不到人，比如有0号和1号两个桌子，0号桌子被占用了，这时新进来一个玩家A匹配游戏，
	//系统会分配1号桌子给该新玩家A，但是机器人没有库存了，此时之前的0号桌子游戏结束空闲下来，又新进来一个玩家B匹配游戏，
	//系统会重新把0号桌子分配给该新玩家B，这样玩家A和玩家B只会在各自分配的桌子当中一直等待其他玩家， 如果没有其他
	//玩家加入，玩家A和玩家B永远也匹配不到一块去，除非1号桌子的玩家退出重新开始匹配。框架处理游戏桌子分配的逻辑是从编号
	//为0的桌子开始遍历查找空闲桌子，找到则分配给当前进入匹配的玩家。原先匹配桌子编号靠后的玩家，在前面的桌子游戏结束空闲
	//下来后不会匹配到任何玩家，直到前面的桌子全部被重新占用，或者退出重新开始匹配游戏，逻辑没问题。
	////////////////////////////////////////		
    tagGameRoomInfo *m_pGameRoomInfo;

	//累计匹配时长，当前累计已经匹配了多长时间
	double totalMatchSeconds_;
	//分片匹配时长，比如0.1s做一次检查
	double sliceMatchSeconds_;
	//超时匹配时长，累计匹配多长时间算超时了，比如3.6s算超时了
	//目前机制是匹配开始，间隔1s补一个机器人，把真实玩家的坑给占了
	//修改后3.6s之前禁入机器人，匹配超时到期(3.6s后)桌子人数不足则用机器人补齐空位
	//如果3.6秒之前匹配进行中，真实玩家人数达到游戏要求人数，撤销定时器立即开始游戏
	double timeoutMatchSeconds_;
	//机器人候补空位超时时长
	double timeoutAddAndroidSeconds_;
	////////////////////////////////////////
	//桌子要求标准游戏人数，最大人数上限GAME_PLAYER和概率值计算得到
	int MaxGamePlayers_;
	//桌子游戏人数概率权重
	int ratioGamePlayerWeight_[GAME_PLAYER];
	//放大倍数
	int ratioScale_;
	//5/4/3/2/单人局的概率
	int ratio5_, ratio4_, ratio3_, ratio2_, ratio1_;
	//计算桌子要求标准游戏人数
	CWeight poolGamePlayerRatio_;
	//初始化桌子游戏人数概率权重
	void initPlayerRatioWeight();
	//准备定时器
	void OnTimerGameReadyOver();
	//结束准备，开始游戏
	void GameTimerReadyOver();
	//拼接各玩家手牌cardValue
	std::string StringCardValue();
	//换牌策略分析
    void AnalysePlayerCards(bool killBlackUsers,int killOrLose);
	//换牌策略分析(小黑屋)
	void AnalysePlayerCards_BlackRoom(bool killBlackUsers);
	//输出玩家手牌/对局日志初始化
	void ListPlayerCards();
	//是否加难控制的代理
	bool isControlAgentId(int chairId);
protected:
	//牌局编号
	string strRoundID_;
	//游戏逻辑
	ZJH::CGameLogic g;
	//最大的手牌
	uint8_t madeMaxCards_[MAX_COUNT];
	uint8_t madeMaxCardsType_;
	//各花色牌值数
	uint8_t handCardColorIndex[4][13]; 
	//玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//手牌牌型
	uint8_t handCardsType_[GAME_PLAYER];
	//本局开始时间/本局结束时间
	chrono::system_clock::time_point roundStartTime_;
	chrono::system_clock::time_point roundEndTime_;
	//庄家用户
	uint32_t bankerUser_;
	//操作用户
	uint32_t currentUser_;
	//首发用户
	uint32_t firstUser_;
	//当前最大牌用户
	uint32_t currentWinUser_;
	//当前第几轮(0,1,2,3,...)
	int32_t currentTurn_;
	//是否超出最大轮
	bool isExceed_;
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
	//玩家游戏结果
	struct playturn_t {
		enum {
			GIVEUP = 1, //弃牌输
			CMPLOST = 2, //比牌输
			ALLINLOST = 3, //孤注一掷输
		};
		//玩家经历轮数
		int turn;
		int result;
	}playTurn_[GAME_PLAYER];
private:
	//是否看牌
	bool isLooked_[GAME_PLAYER];
	//是否弃牌
	bool isGiveup_[GAME_PLAYER];
	//是否比牌输
	bool isComparedLost_[GAME_PLAYER];
	//防超时弃牌
	bool noGiveUpTimeout_[GAME_PLAYER];
private:
	//当前筹码值
	int64_t jettonValue_;
	//基础跟注分
	int64_t followScore_;
	//各玩家下注
	int64_t tableScore_[GAME_PLAYER];
	//桌面总下注
	int64_t tableAllScore_;
	//玩家跟注次数
	int32_t followCount[GAME_PLAYER];
	//玩家下注详情
	struct betinfo_t {
		int64_t jetton;	//筹码值
		int64_t score;	//下注分
		int32_t opValue;//跟注(OP_FOLLOW=3)/加注(OP_ADD=4)/比牌/孤注一掷/火拼
	};
	std::vector<betinfo_t> betinfo_[GAME_PLAYER];
	//最近轮下注分
	inline int64_t GetLastBetScore(uint32_t chairId) {
		assert(currentTurn_ >= 0);
		assert(currentTurn_ < MAX_ROUND);
		if (currentTurn_ < betinfo_[chairId].size()) {
			return betinfo_[chairId][currentTurn_].score;
		}
		return 0;
	}
	//系统当前库存
	tagStorageInfo storageInfo_;
private:
	//系统换牌概率
	int ratioSwap_;
	CWeight sysSwapCardsRatio_;
	//系统通杀概率
	int ratioSwapLost_;
	CWeight sysLostSwapCardsRatio_;
	//系统放水概率
	int ratioSwapWin_;
	CWeight sysWinSwapCardsRatio_;
	//发牌时尽量都发好牌的概率，玩起来更有意思
	int ratioDealGood_;
	//点杀时尽量都发好牌的概率
	int ratioDealGoodKill_;
	//剩最后一个AI和真人比牌,需控制系统赢时,换牌的概率
	int ratioChangeGoodCard_;
	//剩最后一个AI和真人比牌,需控制系统赢时,玩家赢分的范围
	int controlWinScoreRate_[2];
	//是否小黑屋方式控制(0:个人库存控制；1:小黑屋控制)
	int controlType_BlackRoom;
private:
	//IP访问限制
	bool checkIp_;
	//一张桌子真实玩家数量限制
	bool checkrealuser_;

private:
	//chairId对应userID，断线重连时验证
	int64_t saveUserIDs_[GAME_PLAYER];
	//准备定时器
	muduo::net::TimerId timerIdGameReadyOver;
	//等待操作定时器
	muduo::net::TimerId timerIdWaiting_;
	//结束定时器，清理数据
	muduo::net::TimerId timerIdGameEnd_;

	shared_ptr<muduo::net::EventLoopThread>  m_TimerLoopThread;
	//桌子指针
	std::shared_ptr<ITableFrame> m_pTableFrame;
	//造牌配置文件路径
	std::string INI_CARDLIST_;
	//机器人配置
	robotconfig_t robot_;
	//每步骤详情
	std::map<int, string> m_mOutCardRecord;
	//比过牌的用户
	std::vector<uint32_t> comparedUser_[GAME_PLAYER];
	//发送游戏结束数据
	zjh::CMD_S_GameEnd m_StGameEnd;
	//对局日志
	tagGameReplay m_replay;
	//玩家出局(弃牌/比牌输)，若离开房间，信息将被清理，用户数据副本用来写记录
	struct userinfo_t {
		uint32_t chairId;
		int64_t userId;
		uint8_t headerId;
		std::string nickName;
		std::string location;
		std::string account;
		uint32_t agentId;

        std::string lineCode;
		//携带积分
		int64_t userScore;
		//是否机器人
		bool isAndroidUser;
	};
	typedef std::map<uint32_t, userinfo_t> UserInfoList;
	UserInfoList userinfos_;
	//是否有效玩家
	inline bool IsExistUser(uint32_t chairId, int64_t* puserId = NULL) {
		UserInfoList::const_iterator it = userinfos_.find(chairId);
		if (it != userinfos_.end()) {
			if (puserId) {
				*puserId = it->second.userId;
			}
			return true;
		}
		return false;
	}
	//判断特殊牌型
	bool special235_;
	//开启随时看牌
	bool freeLookover_;
	//开启随时弃牌
	bool freeGiveup_;
	//启用三套决策(0,1,2)
	bool useStrategy_;
	//游戏结束是否全部摊牌
	bool openAllCards_;

    double stockWeak;

    // 造牌次数
	int32_t madeCardNum_[7];
	uint8_t haveMadeCardType[7];	//已经得到了的牌型
	//个人金额控制	
	bool blastAndroidChangeCard;    //最后一个AI是否已经换牌了
    int64_t currentHaveWinScore_[GAME_PLAYER];  //玩家累计赢的分数
	ControlInfo userControlinfo_[GAME_PLAYER];
	//控制过期时间
    double kcTimeoutHour_;
    //玩家累计为负时的衰减率(百分比)
    double ratiokcreduction_;
	int32_t userOperate_[GAME_PLAYER][20];  //记录玩家20轮的操作
	int32_t userPlayCount;					//玩的局数

	int32_t isHaveUserPlay_[GAME_PLAYER];		//本座位是否有玩家在玩
	// 分代理线控制加难
	vector<int32_t> controlledProxy;	
	int             controllPoint;
	//调整个人库存控制的控制倍数
	int             addBaseTimes_[6];
	bool			isGemeEnd_;
	//新玩家控制时间
	double newUserTimeoutMinute_;

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

	//加注操作更新当前筹码值
	//加注操作更新基础跟注分
	//start int 筹码表可加注筹码起始索引
	//index int 指定可加注筹码索引
	inline void selectAddIndex(int start, int index) {
		assert(start > 0);
        if(index>=JettionList.size()||index < start)
        {
            return;
        }
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
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};
