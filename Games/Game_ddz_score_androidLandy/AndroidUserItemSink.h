#ifndef ANDROID_USER_ITEM_SINK_HEAD_FILE
#define ANDROID_USER_ITEM_SINK_HEAD_FILE

#pragma once

#include "GameLogic.h"
#include "DouDiZhuRobot_VM.h"
#include "IServerUserItem.h"
#include "ITableFrame.h"
#include "IAndroidUserItemSink.h"

#include "../../public/IAndroidUserItemSink.h"
#include "../../public/IServerUserItem.h"
#include "../../public/ITableFrame.h"
//#include "GameTimer.h"
//////////////////////////////////////////////////////////////////////////

//机器人类
class CAndroidUserItemSink : public IAndroidUserItemSink
{
    //游戏变量
protected:
    WORD							m_wBankerUser;						//庄家用户
    uint8_t							m_cbCurrentLandScore;				//已叫分数
    WORD							m_wOutCardUser;						//出牌玩家

    //扑克变量
protected:
    uint8_t							m_cbTurnCardCount;					//出牌数目
    uint8_t							m_cbTurnCardData[MAX_COUNT];		//出牌列表

    //手上扑克
protected:
    uint8_t							m_cbHandCardData[MAX_COUNT];		//手上扑克
    uint8_t							m_cbHandCardCount[GAME_PLAYER];		//扑克数目

    //控件变量
protected:
    uint8_t                            m_ucGameStatus;
    CGameLogic						m_GameLogic;						//游戏逻辑
    shared_ptr<IServerUserItem>		m_pIAndroidUserItem;				//用户接口

    shared_ptr<ITableFrame>         m_pITableFrame;
    CRobot							m_Robot;

    tagGameRoomInfo*                m_pGameRoomKind;                    //配置房间


    //muduo::net::EventLoopThread     m_TimerLoopThread;
    shared_ptr<muduo::net::EventLoopThread>     m_TimerLoopThread;

	muduo::net::TimerId             OnTimerOnTimerStart;
	muduo::net::TimerId             OnTimerOnTimerCall;
	muduo::net::TimerId             OnTimerOnTimerStake;
	muduo::net::TimerId             OnTimerOnTimerOut;

	int								randomRound;
	int								CountrandomRound;
    //函数定义
public:
    //构造函数
    CAndroidUserItemSink();
    //析构函数
    virtual ~CAndroidUserItemSink();

    //基础接口
public:
    //释放对象
    virtual VOID Release() { delete this; }

    //控制接口
public:
    //初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem);
    //重置接口
    virtual bool RepositionSink();

    //游戏消息

    virtual bool OnGameMessage(uint8_t subid, const uint8_t* pData, uint32_t size);
    //游戏消息
    virtual bool OnEventFrameMessage(WORD wSubCmdID, const uint8_t* pData, WORD wDataSize);
    //场景消息
    virtual bool OnEventSceneMessage(uint8_t cbGameStatus, bool bLookonOther, const uint8_t* pData, WORD wDataSize);

    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame);

    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem);

    virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy) {}
  
    virtual tagAndroidStrategyParam* GetAndroidStrategy() {}
 
    //用户事件
    //游戏事件
public:
    //时间消息
    //virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam);
    void OnTimerOnTimerIDI_START_GAME();

    void OnTimerIDI_CALL_SCORE();

    void OnTimerIDI_STAKE_SCORE();

    void OnTimerIDI_OUT_CARD();

    /*
public:
    //用户进入
    virtual VOID OnEventUserEnter(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
    //用户离开
    virtual VOID OnEventUserLeave(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
    //用户积分
    virtual VOID OnEventUserScore(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
    //用户状态
    virtual VOID OnEventUserStatus(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
    //用户段位
    virtual VOID OnEventUserSegment(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser);
*/
    //消息处理
protected:
    //游戏开始
    bool OnSubGameStart(const uint8_t* pData, WORD wDataSize);
    //用户叫分
    bool OnSubCallScore(const uint8_t* pData, WORD wDataSize);
    //庄家信息
    bool OnSubBankerInfo(const uint8_t* pData, WORD wDataSize);
    //庄家加倍
    bool OnSubStakeScore(const uint8_t* pData, WORD wDataSize);
    //加倍结束
    bool OnSubStakeEnd(const uint8_t* pData, WORD wDataSize);
    //用户出牌
    bool OnSubOutCard(const uint8_t* pData, WORD wDataSize);
    //用户放弃
    bool OnSubPassCard(const uint8_t* pData, WORD wDataSize);
    //游戏结束
    bool OnSubGameEnd(const uint8_t* pData, WORD wDataSize);
    //托管
    bool OnSubTrustee(const uint8_t* pData, WORD wDataSize);

    void SetGameStatus(uint8_t uc);

    void  openLog(const char *p, ...);
    char * GetValue(uint8_t itmp[],uint8_t count, char * name);
};

//////////////////////////////////////////////////////////////////////////

#endif
