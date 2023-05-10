#pragma once

#define INI_FILENAME   "./conf/gswz_config.ini"

class UserInfo
{
public:
    UserInfo()
    {
        userId = 0;
        chairId = 0;
        headerId = 0;
        nickName = "";
        location = "";
        bLeave   = false;
    }



public:
    int64_t     userId;	//玩家ID.
    uint32_t    chairId;
    uint8_t     headerId;	//玩家头像.
    string      nickName;	//玩家昵称.
    string      location;
    bool        bLeave;      //玩家是否已离开
    string     account;
    uint32_t   agentId;
    int64_t      initScore;

};

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

    //事件接口
public:
    //游戏消息
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize);
    //用户接口
public:
    //用户进入
    virtual bool OnUserEnter(int64_t dwUserID, bool bIsLookUser);
    //用户准备
    virtual bool OnUserReady(int64_t dwUserID, bool bIsLookUser);
    //用户离开
    virtual bool OnUserLeft(int64_t dwUserID, bool bIsLookUser);
    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser);
    virtual bool CanLeftTable(int64_t userId);

public:
    //设置指针
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);
    //发送数据
    bool SendTableData(uint32_t chairId, uint8_t subId, const uint8_t* data = 0, int len = 0, bool isRecord = true);

private:
    //清理游戏数据
    virtual void ClearGameData();
    //初始化游戏数据
    void InitGameData();
    //清除所有定时器
    inline void ClearAllTimer();

private:
    void OnTimerGameReadyOver();        //准备定时器
    void OnTimerLockPlayingOver();        //提前一秒锁定玩家不让离开.
    int64_t GetCurrentUserScore(int wChairID);//玩家积分

private:
    void OnTimerAddScore();         //下注定时器

    void OnTimerGameEnd();
    //处理超过局数比牌
    bool HandleMorethanRoundEnd();
    //放弃事件
    bool OnUserGiveUp(int wChairID, bool bExit = false);
    //不加事件
    bool OnUserPassScore(int wChairID);
    //跟注事件
    bool OnUserFollowScore(int wChairID);
    //加注事件
    bool OnUserAddScore(int wChairID, int64_t lScore);
    //梭哈
    bool OnUserAllIn(int wChairID);
    bool OnUserTimeoutGiveUpSetting(int wChairID,  uint32_t mask);

    void SendServerNotify2Client(int32_t chairID,string&str);
    int64_t GetUserIdByChairId(int32_t chairId);
    //将牌堆里的牌复制到手牌
    void SendBufferCard2HandCard(uint8_t count);
    int32_t GetMaxShowCardChairID();
    int32_t GetNextPlayer();
    void SendCard();
    void WriteLoseUserScore(int32_t chairId);
    int64_t GetCanAddMaxScore(int32_t chairId);
protected:

    //读取库存配置
    void ReadStorageInformation();
    //配牌
    bool readJsonCard();
    //更新REDIS库存
    void UpdateStorageScore();

    //功能函数
private:
    void openLog(const char *p, ...);
    void ResetTodayStorage()
    {
        static int64_t nLastTime = 0;
        if(nLastTime == 0)
            nLastTime = time(NULL);

        int64_t nTimeNow = time(NULL);
        if(nTimeNow - nLastTime > 86400)
        {
            m_llTodayStorage = 0;
            nLastTime = nTimeNow;
        }
    }
	//设置当局游戏详情
	void SetGameRecordDetail();
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
	int ratioGamePlayerWeightScale_;
	//计算桌子要求标准游戏人数
	CWeight poolGamePlayerRatio_;
	void GameTimerReadyOver();
    //功能函数
protected:
    //扑克分析
    void AnalyseStartCard();
    string StringCards(uint8_t cbHand[], uint8_t count);
private:
    tagGameRoomInfo*				m_pGameRoomKind;						//游戏
    shared_ptr<ITableFrame>			m_pITableFrame;							//游戏指针
    CGameLogic						m_GameLogic;							//游戏逻辑

//属性变量
protected:
    int64_t							m_llMaxJettonScore;						//最大下注筹码
    int                             m_wPlayCount;                             //当局人数
    string                          m_GameIds;                              //本局游戏编号
    chrono::system_clock::time_point  m_startTime;                      //

private:
    uint8_t							m_cbGameStatus;							//游戏状态
    uint32_t                        m_nTableID;                             // TableID.
    uint32_t                        m_dwStartTime;                          //开始时间
    int64_t                          m_lMarqueeMinScore;                     // 跑马灯最小分值

protected:
    int32_t                             m_wCurrentChairID;					//当前用户座位
    uint8_t                             m_lastMaxViewCardChairID;
    int64_t                             m_wWinnerUser;							//赢家

    //用户状态
protected:
    bool							m_bPlayStatus[GAME_PLAYER];			//游戏状态[true表示还在玩]
    bool                            m_bJoinedGame[GAME_PLAYER];             //玩家是否参与游戏
    uint8_t							m_cbGiveUp[GAME_PLAYER];				//别人不能看到弃牌玩家的牌,游戏结束的时候用
    uint8_t							m_cbSystemGiveUp[GAME_PLAYER];			//系统弃牌
    int64_t                         m_wCurrentRoundScore[GAME_PLAYER];      //本圈玩家下注
    int64_t                         m_dwPlayUserID[GAME_PLAYER];			//参加游戏的玩家ID
    int64_t                         m_dwPlayFakeUserID[GAME_PLAYER];		//参加游戏的玩家假ID
    uint8_t							m_cbRealPlayer[GAME_PLAYER];			//真人玩家
    uint8_t							m_cbAndroidStatus[GAME_PLAYER];			//机器状态
    bool                            m_bWaitEnterTable[GAME_PLAYER];         //等待入桌
    //下注信息
protected:
    int64_t							m_llUserSourceScore[GAME_PLAYER];		//玩家原始金币
    int64_t							m_lTableScore[GAME_PLAYER];				//下注数目
    int64_t                         m_iUserWinScore[GAME_PLAYER];           //下注数目
    int64_t							m_lCellScore;							//单元下注
    uint64_t                        m_dAllInScore;                          //本局梭哈值
    int64_t							m_lCurrentJetton;						//当前筹码
    int64_t                         m_MinAddScore;                          //最少需要加注金额
    uint32_t						m_NoGiveUpTimeout[GAME_PLAYER];			//防超时弃牌.
    uint32_t                        m_dwOpEndTime;							//操作结束时间
    uint32_t                        m_wTotalOpTime;							//操作总时间
	string							m_strHandCardsList;
    map<int, string>                m_mOutCardRecord;

    bool							m_bGameEnd;								//结束状态
    CMD_S_GameEnd					m_StGameEnd;							//结束数据

    //扑克变量
protected:
    uint8_t							m_cbHandCardData[GAME_PLAYER][MAX_COUNT];//桌面扑克
    uint8_t							m_cbHandCardBufer[GAME_PLAYER][MAX_COUNT];//桌面扑克
    uint8_t							m_cbHandCardType[GAME_PLAYER];			//牌型
    bool                            m_bCompardChairID[GAME_PLAYER];			//最终比牌玩家
    uint8_t							m_cbRepertoryCard[MAX_CARD_TOTAL];		//库存扑克
    int32_t                         m_LastAction[GAME_PLAYER];              //最后操作命令
    uint8_t                         m_HandCardCount;
    //AI变量
protected:
    int64_t                           m_lCurrStockScore;                      //当前赢分
    static int64_t                    m_lStockScore;							//总输赢分
    static int64_t                    m_lStockLowLimit;						//总库存下限
    static int64_t                    m_llTodayStorage;						//今日库存
    static int64_t                    m_llTodayHighLimit;						//今日上限

    int32_t 							m_wSystemAllKillRatio;                  //系统通杀率
    int32_t 							m_wSystemChangeCardRatio;               //系统换牌率

	CWeight							m_poolSystemLostChangeCardRatio;
	int32_t								m_wSystemLostChangeCardRatio;				//系统输了换牌概率 80%
	CWeight						m_poolSystemWinChangeCardRatio;
	int32_t								m_wSystemWinChangeCardRatio;				//系统赢了换牌概率 70%

    muduo::net::TimerId             m_TimerIdGameReadyOver;                     //准备定时器
    muduo::net::TimerId             m_TimerIdGameLockPlaying;                     // lock user leave.
    muduo::net::TimerId             m_TimerIdAddScore;                      //下注定时器
    muduo::net::TimerId             m_TimerIdGameEnd;                  //清理离线玩家
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;                      //thread.

    bool                            m_bContinueServer;                      //判断服务器是否已经开始.
    int32_t                         m_TurnsRound;                           //游戏轮数
    int64_t                         m_lAllScoreInTable;                     //桌面上的总金币
    map<int64_t, shared_ptr<UserInfo>>  m_UserInfo;
    list<vector<uint8_t>>         m_listTestCardArray;
    tagGameReplay       m_replay;//game replay
    int64_t                       m_joinRoomScoreMultipleLimit;
//    uint32_t                m_dwReadyTime;
private:
    bool                            m_bLimitAllIn;                          //
    bool                            m_IsEnableOpenlog;
    bool                            m_IsMatch;
    double                          stockWeak;
    STD::Random                m_random;
    int32_t                    m_difficulty;                                      //玩家难度 千分制
};
