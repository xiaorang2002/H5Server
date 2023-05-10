#pragma once

#define GAME_STATUS_INIT                        GameStatus::GAME_STATUS_INIT //初始状态
#define GAME_STATUS_READY						GameStatus::GAME_STATUS_FREE //空闲状态
#define GAME_STATUS_PLAYING                     GameStatus::GAME_STATUS_START//进行状态
#define GAME_STATUS_END				    	    GameStatus::GAME_STATUS_END  //游戏结束
#define GAME_STATUS_GROUP						101 //理牌状态
#define GAME_STATUS_COMPARE						102 //比牌状态

#define FloorScore		(m_pTableFrame->GetGameRoomInfo()->floorScore)//底注
#define CeilScore		(m_pTableFrame->GetGameRoomInfo()->ceilScore)//顶注
#define CellScore		(FloorScore)//底注
#define JettionList     (m_pTableFrame->GetGameRoomInfo()->jettons)//筹码表

#define ThisTableId		(m_pTableFrame->GetTableId())
#define ThisGameId		(m_pTableFrame->GetGameRoomInfo()->gameId)
#define ThisRoomId		(m_pTableFrame->GetGameRoomInfo()->roomId)
#define ThisRoomName	(m_pTableFrame->GetGameRoomInfo()->roomName)
#define ThisThreadTimer	(m_pTableFrame->GetLoopThread()->getLoop())

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

//配置文件
#define INI_FILENAME "./conf/s13s_config.ini"
#define INI_CARDLIST "./conf/s13s_cardList.ini"

//游戏流程
class CGameProcess : public ITableFrameSink
{
public:
    CGameProcess(void);
    ~CGameProcess(void);
public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t dwChairID, uint8_t GETag);
    //发送场景
    virtual bool OnEventGameScene(uint32_t dwChairID, bool bIsLookUser);
    //游戏消息
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);
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
    virtual void ClearGameData();
    //初始化游戏数据
    void InitGameData();
    //清除所有定时器
    inline void ClearAllTimer();
	//理牌定时器
	void OnTimerGroupOver();
	//结束定时器，延时清理数据
    void OnTimerGameEnd();
    //读取库存配置
    void ReadStorageInformation();
	//换牌策略分析
	//默认概率随机换牌(可配置)，换牌后机器人都拿好牌，真实玩家都拿差牌
	//如果随机到不换牌，若是系统需要杀玩家，强制换牌，否则不换牌
	//如果随机到换牌，若是系统需要放水，则不换牌，否则换牌
	//如果玩家输分太多系统需要放水时，系统只是不做换牌处理，
	//玩家牌好坏由系统随机玩家牌(手气)和玩家手动摆牌策略决定
	//若玩家手动组到好牌可能会赢，如果组牌不好，输了也是正常
	//避免故意放分导致系统一直输分情况
	//真人玩家不能最大牌(随机概率)，否则与机器人牌中最大的换牌(待实现)
	//如果真人玩家最大牌，并且赢得水数超过限制，则与机器人牌中最大的换牌(待实现)
	void AnalysePlayerCards(bool killBlackUsers);
	//玩家之间两两比牌
	void StartCompareCards();
	//是否开始比牌
	bool isCompareCards_;

    bool CompareAllCardsSame(uint8_t *handcards,uint8_t *clientcards,int32_t len);
	bool bCheckCard(uint8_t *handcards, uint8_t *clientcards, int32_t len);
	bool bCheckCardIsValid(uint8_t *clientcards, int32_t len);
	string getCardString(uint8_t *cards, int32_t len);
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
protected:
	//IP访问限制
	bool checkIp_;
	//一张桌子真实玩家数量限制
	bool checkrealuser_;
protected:
	//牌局编号
	string strRoundID_;
	//游戏逻辑
	S13S::CGameLogic g;
	//各个玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//玩家牌型分析结果
	S13S::CGameLogic::handinfo_t handInfos_[GAME_PLAYER];
	//直接使用
	S13S::CGameLogic::handinfo_t* phandInfos_[GAME_PLAYER];
	//各个玩家比牌结果
	s13s::CMD_S_CompareCards compareCards_[GAME_PLAYER];
	//理牌总的时间/理牌剩余时间
	uint32_t groupTotalTime_;
	//理牌开始时间
	uint32_t groupStartTime_;
	//理牌结束时间
	uint32_t groupEndTime_;
	//本局开始时间/本局结束时间
	chrono::system_clock::time_point roundStartTime_;
	chrono::system_clock::time_point roundEndTime_;
	//uint32_t roundStartTime_, roundEndTime_;
	//确定牌型的玩家数
	int selectcount;
	//内部使用玩家状态
	int playerStatus_[GAME_PLAYER];
	//内部使用游戏状态
	int gameStatus_;
	//玩家确定三墩牌型
	//bool makesureDunHandTy_[GAME_PLAYER];
	//chairID对应userID，断线重连时验证
	int64_t saveUserIDs_[GAME_PLAYER];
	//系统输了换牌概率 80%
	int ratioSwapLost_;
	CWeight sysLostSwapCardsRatio_;
	//系统赢了换牌概率 70%
	int ratioSwapWin_;
	CWeight sysWinSwapCardsRatio_;
	//默认换牌概率
	int ratioSwap_;
	CWeight sysSwapCardsRatio_;
	//记录本局玩家手牌
	//std::string strHandCardsList_;
	//发送游戏结束数据
	s13s::CMD_S_GameEnd m_StGameEnd;
	//系统当前库存
	tagStorageInfo storageInfo_;
	//对局日志
	tagGameReplay m_replay;
	//造牌配置文件路径
	std::string INI_CARDLIST_;
	//桌子指针
	std::shared_ptr<ITableFrame> m_pTableFrame;
	//准备定时器
	muduo::net::TimerId timerIdGameReadyOver;
	//理牌定时器
	muduo::net::TimerId timerIdManual_;
	//结束定时器，清理数据
	muduo::net::TimerId timerIdGameEnd_;
	//定时器指针
	shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;
    //库存衰减百分比
    double stockWeak ;
	//玩家选择手牌
	uint8_t userChooseCards_[MAX_COUNT];
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};
