#pragma once

#include <stdint.h>
#include <string>

#include "ITableFrame.h"

// table frame sink.
class ITableFrameSink
{
public:
    ITableFrameSink() {}
    virtual ~ITableFrameSink() {}

public:
    virtual void OnGameStart()  = 0;
    virtual bool OnEventGameConclude(uint32_t chairId, uint8_t nEndTag) = 0;
    virtual bool OnEventGameScene(uint32_t chairId, bool bisLookon)    = 0;

public://users.
    virtual bool OnUserEnter(int64_t userId, bool isLookon) = 0;
    virtual bool OnUserReady(int64_t userId, bool isLookon) = 0;
    virtual bool OnUserLeft(int64_t userId, bool isLookon) = 0;

    virtual bool CanJoinTable(shared_ptr<CServerUserItem>& pUser) = 0;
    virtual bool CanLeftTable(int64_t userId) = 0;


public://events.
    virtual bool OnGameMessage(uint32_t chairid, uint8_t subid, const uint8_t* data, uint32_t datasize) = 0;

public:
    virtual bool SetTableFrame(shared_ptr<ITableFrame>& pTableFrame) = 0;
    virtual void RepositionSink() {}
};

#define PFNNAME_CREATE_SINK      ("CreateTableFrameSink")
#define PFNNAME_DEL_SINK         ("DeleteTableFrameSink")
#define PFNNAME_VALID_CONFIG     ("ValidTableFrameConfig")

// get the special table frame sink from the library.
extern "C" shared_ptr<ITableFrameSink> GetTableFrameSink(void);
