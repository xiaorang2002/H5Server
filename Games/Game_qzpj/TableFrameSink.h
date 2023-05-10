
#pragma once

#define GAME_STATUS_INIT                        GameStatus::GAME_STATUS_INIT //初始状态
#define GAME_STATUS_READY						GameStatus::GAME_STATUS_FREE //空闲状态
#define GAME_STATUS_START	                    GameStatus::GAME_STATUS_START//进行状态
#define GAME_STATUS_CALL						102							//叫庄状态
#define GAME_STATUS_SCORE						103							//下注状态
#define GAME_STATUS_OPEN						104							//摊牌状态
#define GAME_STATUS_NEXT			            105
#define GAME_STATUS_PREEND                      106
#define GAME_STATUS_END				    	    GameStatus::GAME_STATUS_END  //游戏结束

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
#define INI_FILENAME "./conf/qzpj_config.ini"
#define INI_CARDLIST "./conf/qzpj_cardList.ini"

#include "AnimatePlay.h"

//游戏桌子类
class CTableFrameSink :public ITableFrameSink
{
public:
	//构造函数
    CTableFrameSink();
	//析构函数
    virtual ~CTableFrameSink();
public:
    //游戏开始
    virtual void OnGameStart();
    //游戏结束
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t cbReason);
    //发送场景
    virtual bool OnEventGameScene(uint32_t chairid, bool bisLookon);
    //游戏消息
    virtual bool OnGameMessage( uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);
	//用户进入
    virtual bool OnUserEnter(int64_t userid, bool islookon);
	//用户准备
    virtual bool OnUserReady(int64_t userid, bool islookon);
	//用户离开
    virtual bool OnUserLeft( int64_t userid, bool islookon) ;
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
    void ClearAllTimer(bool bv = false);
private:
	//跑马灯 ///
    void PaoMaDeng();
	//是否最大牌 ///
    int IsMaxCard(int chairId);
    //设置托管 ///
	//游戏开始，开始叫庄(5s)
	//都叫庄了，开始下注(5s)
	//都下注了，开始摊牌(5s)
	//都摊牌了，结束游戏(2s)
    bool IsTrustee(void);
    //叫庄 ///
    void OnTimerCallBanker();
	//已叫庄人数 ///
	int GetCallBankerCount();
	//随机庄家 ///
	int RandBanker();
	//玩家叫庄 ///
    int OnUserCallBanker(int chairId, int wCallFlag);
	//下注 ///
	void OnTimerAddScore();
	//已下注人数 ///
	int GetAddScoreCount();
	//玩家下注 ///
	int OnUserAddScore(int chairId, int wJettonIndex);
	//摊牌 ///
	void OnTimerOpenCard();
	//已摊牌人数 ///
	int GetOpenCardCount();
	//玩家摊牌 ///
	int OnUserOpenCard(int chairId);
	//发送场景
	bool OnEventGameScene2(uint32_t chairid);
private:
	//换牌策略分析 ///
	//收分时，机器人都拿大牌，玩家拿小牌
	//放分时，机器人都拿小牌，玩家拿大牌
	//点杀时，机器人都拿大牌，点杀玩家拿最小牌，其他拿次大牌
    void AnalysePlayerCards(bool killBlackUsers);
	//输出玩家手牌/对局日志初始化
	void ListPlayerCards();
	//游戏结束，清理数据 ///
    void OnTimerGameEnd();
    //正常结束游戏结算 ///
    void NormalGameEnd(int chairId);
	//
	bool CheckEnterScore(int64_t* userLeftScore);
	bool isGameOver_;
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
	//读取配置
	void ReadConfig();
private:
	//牌局编号
	std::string strRoundIDNew_, strRoundID_;
	//游戏逻辑
	PJ::CGameLogic g;
	//玩家手牌
	uint8_t handCards_[GAME_PLAYER][MAX_COUNT];
	//手牌牌型
	uint8_t handCardsType_[GAME_PLAYER];
	//进行第几局
	int curRoundTurn_;
	//本局开始时间/本局结束时间
	chrono::system_clock::time_point roundStartTime_;
	chrono::system_clock::time_point roundEndTime_;
	//叫庄开始时间
	chrono::system_clock::time_point roundCallTime_;
	//下注开始时间
	chrono::system_clock::time_point roundScoreTime_;
	//摊牌开始时间
	chrono::system_clock::time_point roundOpenTime_;
	//庄家用户
	uint32_t bankerUser_;
	//系统当前库存
	tagStorageInfo storageInfo_;
	//对局日志
	tagGameReplay m_replay;
	//造牌配置文件路径
	std::string INI_CARDLIST_;
	//桌子指针
	std::shared_ptr<ITableFrame> m_pTableFrame;
	//匹配定时器
	muduo::net::TimerId timerGameReadyID_;
	//叫庄定时器
	muduo::net::TimerId timerCallBankerID_;
	//下注定时器
	muduo::net::TimerId timerAddScoreID_;
	//摊牌定时器
	muduo::net::TimerId timerOpenCardID_;
	//结束定时器
	muduo::net::TimerId timerGameEndID_;
	//定时器指针
	shared_ptr<muduo::net::EventLoopThread> m_TimerLoopThread;
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
private:
	AnimatePlay aniPlay_;
	int64_t userLeftScore_[GAME_PLAYER];
	int64_t userWinScorePure_[GAME_PLAYER];
	//玩家叫庄(-1:未叫 0:不叫 1,2,3,4:叫庄)
	int callBanker_[GAME_PLAYER];
	//玩家下注(-1:未下注 大于-1:对应筹码索引)
	int jettonIndex_[GAME_PLAYER];
	//玩家摊牌(-1:未摊牌 大于-1:对应手牌牌型)
	int openCardType_[GAME_PLAYER];
	//可用的叫庄倍数
	int8_t callableMulti_[GAME_PLAYER][MAX_BANKER_MUL];
	//可用的下注倍数
	int8_t jettonableMulti_[GAME_PLAYER][MAX_JETTON_MUL];
	//待点杀黑名单玩家
	bool isBlackUser_[GAME_PLAYER];
	//是否是比赛
    bool m_isMatch;
    double stockWeak;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};

