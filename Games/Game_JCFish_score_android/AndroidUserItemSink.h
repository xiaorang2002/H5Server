#pragma once

#include <types.h>
#include "ITableFrameSink.h"
#include "DefGameAndroidExport.h"
#include "../Game_JCFish/FishProduceUtils/CMD_Fish.h".h"
#include "Globals.h"
#include "IAndroidUserItemSink.h"
#include "IServerUserItem.h"

#include "Fish.Message.pb.h"

enum eFishGameStates
{
    EFISHSTAT_FREE = 0,
    EFISHSTAT_WATCH = 2,
    EFISHSTAT_FIRING = 1,
};

class ai_action;
class CDdzAiThread;
class CAndroidUserItemSink : public IAndroidUserItemSink
{
public:
	CAndroidUserItemSink();
	virtual ~CAndroidUserItemSink();

public:
    //重置接口
    virtual bool RepositionSink() ;
    //初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem) ;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) ;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) ;

    //游戏事件
public:
    //定时器事件
    virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam) ;
    //游戏事件
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size) ;

    virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy);
    virtual tagAndroidStrategyParam* GetAndroidStrategy();

	//消息处理
protected:
    //逻辑辅助
    bool OnSubGameScene(const void * pBuffer, WORD wDataSize);
    bool OnUserExchangeScore();
    bool OnUserFire();


private:
    WORD				m_wChairID;									// 机器人椅子ID
    shared_ptr<ITableFrame>		m_pTableFrame;								// 桌子指针
    shared_ptr<IServerUserItem>	m_pAndroidUserItem;							// 机器人对象

    //游戏变量
protected:
    DOUBLE              m_fGunAngle;                                // 当前炮角度
    float               m_lGunPower;                                // 当前炮大小

    float               m_lTotalGain;                               // 玩家总进
    float               m_lTotalLost;                               // 玩家总出

    float               m_lCurrGain;                                // 当前总进
    float               m_lCurrLost;                                // 当前总出

protected:
    int                 m_nAndroidStatus;                           // 机器人状态

};
