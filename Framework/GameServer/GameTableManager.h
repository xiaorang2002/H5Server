#pragma once

#include "public/ITableFrame.h"
#include <boost/serialization/singleton.hpp>
#include <muduo/base/Mutex.h>
#include <vector>
#include <map>
#include <list>
#include <stdint.h>

//struct tagWaitListInfo
//{
//    dword   userid;
//    uint64  clientAddr;
//    muduo::net::TcpConnectionWeakPtr pProxySock;
//};


using boost::serialization::singleton;
using namespace muduo;

//typedef std::list<tagWaitListInfo>              ListWaitList;
//typedef std::vector<shared_ptr<ITableFrame>>    VecTableFrame;
//typedef std::list<shared_ptr<ITableFrame>>      ListTableFrame;

class CGameTableManager : public singleton<CGameTableManager>
{
public:
    CGameTableManager();
    virtual ~CGameTableManager();

public:
    void InitAllTable(uint32_t tableCount, tagGameInfo * pGameInfo, tagGameRoomInfo* pRoomInfo, PFNCreateTableSink pfnCreate, shared_ptr<EventLoopThread> &loopThread, shared_ptr<EventLoopThread> &dbloopThread,ILogicThread*);
    void KickAllTableUser();
    shared_ptr<ITableFrame> GetTable(uint32_t dwTableId);

public:
    shared_ptr<ITableFrame> FindNormalTableFromTableId(uint32_t tableId);
    shared_ptr<ITableFrame> FindNormalSuitTable(shared_ptr<CServerUserItem>& pUser , uint32_t exceptTableId = INVALID_TABLE, uint32_t enterTableId = INVALID_TABLE);
    void FreeNormalTable(shared_ptr<ITableFrame>& pITableFrame);
    void UpdataPlayerCount2Redis(string &strNodeValue , tagGameInfo pGameInfo);
public:
    bool SetTableStockInfo(tagStockInfo& stockinfo);

public:
    void Clear();

public:
    vector<shared_ptr<ITableFrame>>   GetAllTableFrame();
    list<shared_ptr<ITableFrame>> &   GetUsedTableFrame();

protected:
    vector<shared_ptr<ITableFrame>>   m_vecGameTable;     // vector game table.

    vector<shared_ptr<ITableFrame>>   vecGameTable;     // vector game table.
    list<shared_ptr<ITableFrame>>     m_listFreeTable;    // list free table.
    list<shared_ptr<ITableFrame>>     m_listUsedTable;    // list used table.

protected:
    PFNCreateTableSink  m_CreateTableFrameSink; // create sink.
    tagGameInfo*        m_pGameInfo;        // game kind.
    tagGameRoomInfo*    m_pGameRoomInfo;        // game room kind.
    tagStockInfo        m_stockInfo;        // stock info.



private:
    mutable boost::shared_mutex m_mutex;
};

