
#include <muduo/base/Logging.h>
#include "public/ITableFrameSink.h"
#include "GameTableManager.h"
#include "TableFrame.h"

#include <memory>


CGameTableManager::CGameTableManager()
{
    m_pGameRoomInfo = NULL;
}


CGameTableManager::~CGameTableManager()
{
}

void CGameTableManager::InitAllTable(uint32_t tableCount, tagGameInfo* pGameInfo, tagGameRoomInfo* pGameRoomInfo,
                                     PFNCreateTableSink pfnCreateSink, shared_ptr<EventLoopThread> &loopThread, shared_ptr<EventLoopThread>& game_dbmysql_thread ,ILogicThread* pLogicThread)
{
    // try to check all input parameter for is success now.
    if ((!pGameInfo) || (!pGameRoomInfo) || (!pfnCreateSink))
    {
        return;
    }

    m_pGameInfo = pGameInfo;
    m_pGameRoomInfo = pGameRoomInfo;
    m_CreateTableFrameSink = pfnCreateSink;

    bool bLoadStock = false;
    for(uint32_t idx = 0; idx < tableCount; ++idx)
    {
        shared_ptr<ITableFrameSink> pTableFrameSink  = pfnCreateSink();
        shared_ptr<CTableFrame> pTableFrame = make_shared<CTableFrame>();
        if ((!pTableFrameSink) || (!pTableFrame))
        {
            LOG_ERROR << "InitAllTable Table:" << idx << " Failed!";
            break;
        }

        shared_ptr<ITableFrame> pITableFrame = dynamic_pointer_cast<ITableFrame>(pTableFrame);
        {

            // 1. initialize table now.
            TableState tableState = { 0 };
            tableState.nTableID   = idx;
            tableState.bisLock    = false;
            tableState.bisLookOn  = false;
            pITableFrame->Init(pTableFrameSink, tableState, pGameInfo, pGameRoomInfo, loopThread,game_dbmysql_thread, pLogicThread);

            if(!bLoadStock)
            {
                pTableFrame->ReadStorageScore();
                bLoadStock = true;
            }

            pTableFrameSink->SetTableFrame(pITableFrame);

            vecGameTable.push_back(pITableFrame);
            m_vecGameTable.push_back(pITableFrame);
            m_listFreeTable.push_back(pITableFrame);

           // LOG_DEBUG << ">>> init table item, idx:" << pITableFrame->GetTableId() << ", table:" << pITableFrame.get();
        }
    }
}


vector<shared_ptr<ITableFrame>> CGameTableManager::GetAllTableFrame()
{
    vector<shared_ptr<ITableFrame>> gameTable;
    {
        READ_LOCK(m_mutex);
        gameTable = m_vecGameTable;
    }
    return (gameTable);
}

list<shared_ptr<ITableFrame>> &CGameTableManager::GetUsedTableFrame()
{
//    list<shared_ptr<ITableFrame>> usedTableFrame;
//    {
//        READ_LOCK(m_mutex);
//        usedTableFrame = m_listUsedTable;
//    }
    return m_listUsedTable;
}

void CGameTableManager::KickAllTableUser()
{
    LOG_ERROR << " >>> KickAllTableUser for maintain.";
    m_pGameRoomInfo->serverStatus = SERVER_STOPPED;
    list<shared_ptr<ITableFrame>> usedTableList;
    {
        READ_LOCK(m_mutex);
        usedTableList = m_listUsedTable;
    }

    for(auto iter = usedTableList.begin(); iter != usedTableList.end(); ++iter)
    {
        shared_ptr<CTableFrame> pITableFrame = dynamic_pointer_cast<CTableFrame>(*iter);
        if(pITableFrame)
        {
            tagGameRoomInfo* pRoomInfo = pITableFrame->GetGameRoomInfo();
            pRoomInfo->serverStatus = SERVER_STOPPED;

            pITableFrame->SetGameStatus(SERVER_STOPPED);
            pITableFrame->DismissGame();
        }
    }
}


// try to get the special table item data value content.
shared_ptr<ITableFrame> CGameTableManager::GetTable(uint32_t tableId)
{
    shared_ptr<ITableFrame> tableFrame;
    READ_LOCK(m_mutex);
    // try to check the special table item now.
    if(tableId < m_vecGameTable.size())
    {
        return (m_vecGameTable[tableId]);
    }
//Cleanup:
    return tableFrame;
}

shared_ptr<ITableFrame> CGameTableManager::FindNormalTableFromTableId(uint32_t tableId)
{
    list<shared_ptr<ITableFrame>>  usedTableList;
    {
        READ_LOCK(m_mutex);
        usedTableList = m_listUsedTable;
    }

    shared_ptr<ITableFrame> tableFrame;
    for (auto it : usedTableList)
    {
        shared_ptr<ITableFrame> pITableFrame = it;
        if(tableId == pITableFrame->GetTableId())
        {
            if (pITableFrame->GetPlayerCount(true) < pITableFrame->GetMaxPlayerCount())
            {
                return pITableFrame;
            }
        }
    }
    return tableFrame;
}

shared_ptr<ITableFrame> CGameTableManager::FindNormalSuitTable(uint32_t exceptTableId)
{
    list<shared_ptr<ITableFrame>> usedTableList;
    {
        READ_LOCK(m_mutex);
        usedTableList = m_listUsedTable;
    }

    for(auto it : usedTableList)
    {
        shared_ptr<ITableFrame> pITableFrame = it;
        if(INVALID_TABLE != exceptTableId && exceptTableId == pITableFrame->GetTableId())
        {
            continue;
        }

        if (pITableFrame->GetPlayerCount(true) < pITableFrame->GetMaxPlayerCount())
        {
            return pITableFrame;
        }
    }

    // update table status.
    {
        WRITE_LOCK(m_mutex);

        if(!m_listFreeTable.empty())
        {
            shared_ptr<ITableFrame> pITableFrame = m_listFreeTable.front();
            m_listUsedTable.push_back(pITableFrame);
            m_listFreeTable.pop_front();
            return pITableFrame;
        }
    }

    return shared_ptr<CTableFrame>(NULL);  //make_shared<CTableFrame>();
}

// try to free the special normal table content item value data now.
void CGameTableManager::FreeNormalTable(shared_ptr<ITableFrame>& pITableFrame)
{
    WRITE_LOCK(m_mutex);
    for(auto it = m_listUsedTable.begin(); it != m_listUsedTable.end(); ++it)
    {
        if((*it)->GetTableId() == pITableFrame->GetTableId())
        {
            m_listFreeTable.push_back(pITableFrame);
            m_listUsedTable.erase(it);
            break;
        }
    }
}


// set the special table stock information data.
bool CGameTableManager::SetTableStockInfo(tagStockInfo& stockinfo)
{
    m_stockInfo = stockinfo;
    return true;
}


// try to clear all the data now.
void CGameTableManager::Clear()
{
    m_vecGameTable.clear();
    m_listFreeTable.clear();
    m_listUsedTable.clear();
}
