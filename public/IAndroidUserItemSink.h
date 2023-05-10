#pragma once

#include "Globals.h"
#include "gameDefine.h"

//机器人回调接口


class ITableFrame;
class IServerUserItem;


class IAndroidUserItemSink
{
public:
	IAndroidUserItemSink() {}
	virtual ~IAndroidUserItemSink() {}

public:
	//重置接口
    virtual bool RepositionSink() = 0;
	//初始接口
    virtual bool Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem) = 0;
    virtual bool SetUserItem(shared_ptr<IServerUserItem>& pUserItem) = 0;
    virtual void SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) = 0;

	//游戏事件
public:
	//定时器事件
//    virtual bool OnTimerMessage(uint32_t timerid, uint32_t dwParam) = 0;
	//游戏事件
    virtual bool OnGameMessage(uint8_t subId, const uint8_t* pData, uint32_t size) = 0;

    virtual void SetAndroidStrategy(tagAndroidStrategyParam* strategy)= 0;
    virtual tagAndroidStrategyParam* GetAndroidStrategy()= 0;
};

typedef shared_ptr<IAndroidUserItemSink>		(*PFNCreateAndroid)();              //生成机器人
typedef void* (*PFNDeleteAndroid)(IAndroidUserItemSink* p);							//删除机器人

#define FNAME_CREATE_ANDROID		("CreateAndroidSink")							//得到桌子的实例
#define FNAME_DELETE_ANDROID		("DeleteAndroidSink")							//删除桌子的实例
