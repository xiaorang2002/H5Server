
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
                                     PFNCreateTableSink pfnCreateSink, shared_ptr<EventLoopThread> &loopThread, shared_ptr<EventLoopThread> &dbloopThread,ILogicThread* pLogicThread)
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
        shared_ptr<CTableFrame> pTableFrame(new CTableFrame());
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
            pITableFrame->Init(pTableFrameSink, tableState, pGameInfo, pGameRoomInfo, loopThread,dbloopThread, pLogicThread);

            if(!bLoadStock)
            {
                pTableFrame->ReadStorageScore();
                bLoadStock = true;
            }

            pTableFrameSink->SetTableFrame(pITableFrame);

            vecGameTable.push_back(pITableFrame);
            m_vecGameTable.push_back(pITableFrame);
            m_listFreeTable.push_back(pITableFrame);

           // LOG_DEBUG << " >>> init table item, idx:" << pITableFrame->GetTableId() << ", table:" << pITableFrame.get();
        }
    }
}

////
//void CGameTableManager::KickAllTableUser()
//{
//    LOG_ERROR << " >>> KickAllTableUser for maintain.";
//    m_pRoomKind->nServerStatus = SERVER_STOPPED;
//    list<shared_ptr<ITableFrame>> usedTableList;
//    {
//        READ_LOCK(m_mutex);
//        usedTableList = m_listUsedTable;
//    }

//    for(auto iter = usedTableList.begin(); iter != usedTableList.end(); ++iter)
//    {
//        shared_ptr<CTableFrame> pITableFrame = dynamic_pointer_cast<CTableFrame>(*iter);
//        if(pITableFrame)
//        {
//            tagGameRoomInfo* pRoomKind = pITableFrame->GetGameRoomInfo();
//            pRoomKind->nServerStatus = SERVER_STOPPED;

//            pITableFrame->SetGameStatus(SERVER_STOPPED);
//            pITableFrame->DismissGame();

//            shared_ptr<CServerUserItem> pIServerUserItem;
//            for(int i = 0; i < pITableFrame->m_UserList.size(); ++i)
//            {
//                {
//                    READ_LOCK(pITableFrame->m_mutex);
//                    pIServerUserItem = pITableFrame->m_UserList[i];
//                }
//                if(pIServerUserItem)
//                {
////                    shared_ptr<IServerUserItem> pItem = dynamic_pointer_cast<IServerUserItem>(pIServerUserItem);
//                    pITableFrame->OnUserLeft(pIServerUserItem);
//                }
//            }
////            pITableFrame->ClearTableUser(INVALID_TABLE);
//        }
//    }
//}

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

shared_ptr<ITableFrame> CGameTableManager::FindNormalSuitTable(shared_ptr<CServerUserItem>& pUser, uint32_t exceptTableID, uint32_t enterTableId)
{
    list<shared_ptr<ITableFrame>> usedTableList;
    {
        READ_LOCK(m_mutex);
        usedTableList = m_listUsedTable;
    }

    // 百人场只有一个桌子
    for(auto it : usedTableList)
    {
        shared_ptr<ITableFrame> pITableFrame = it;
        if(INVALID_TABLE != exceptTableID && exceptTableID == pITableFrame->GetTableId())
        {
            continue;
        }
		if (INVALID_TABLE != enterTableId && enterTableId != pITableFrame->GetTableId())
		{
			continue;
		}
        if (pITableFrame->GetPlayerCount(true) < pITableFrame->GetMaxPlayerCount() && pITableFrame->CanJoinTable(pUser))
        {
            return pITableFrame;
        }
    }

    // update table status.
    {
        WRITE_LOCK(m_mutex);
//        for(auto iter = m_listFreeTable.begin(); iter != m_listFreeTable.end(); ++iter)
//        {
//            shared_ptr<ITableFrame> pITableFrame = dynamic_pointer_cast<ITableFrame>(*iter);
//            if(pITableFrame->GetPlayerCount(true) < pITableFrame->GetMaxPlayerCount())
//            {
//                uint32_t tableID   = pITableFrame->GetTableId();
//                uint32_t playerCount = pITableFrame->GetPlayerCount(true);

//                LOG_DEBUG << " >>>>>> m_listUsedTable add used table, tableid: " << tableID << ", playerCount: " << playerCount << ", pITableFrame:" << pITableFrame.get();

//                m_listUsedTable.push_back(pITableFrame);
//                m_listFreeTable.erase(iter);
//                return pITableFrame;
//            }
//        }
		if (INVALID_TABLE != enterTableId)
		{
			for (auto it = m_listFreeTable.begin(); it != m_listFreeTable.end(); ++it)
			{
				if ((*it)->GetTableId() == enterTableId)
				{
					shared_ptr<ITableFrame> pITableFrame = (*it);
					m_listUsedTable.push_back(pITableFrame);
					m_listFreeTable.erase(it);

					return pITableFrame;
				}
			}
			if (!m_listFreeTable.empty())
			{
				shared_ptr<ITableFrame> pITableFrame = m_listFreeTable.front();
				m_listUsedTable.push_back(pITableFrame);
				m_listFreeTable.pop_front();
				return pITableFrame;
			}
		}
		else
		{
			if (!m_listFreeTable.empty())
			{
				shared_ptr<ITableFrame> pITableFrame = m_listFreeTable.front();
				m_listUsedTable.push_back(pITableFrame);
				m_listFreeTable.pop_front();
				return pITableFrame;
			}
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

void CGameTableManager::UpdataPlayerCount2Redis(string &strNodeValue,tagGameInfo  m_GameInfo)
{
    WRITE_LOCK(m_mutex);
    uint64_t totalAndroidCount=0;
    uint64_t totalRealCount=0;
    uint32_t androidCount=0;
    uint32_t realCount=0;
    int32_t gameid = m_GameInfo.gameId;
    for(auto it = m_listUsedTable.begin(); it != m_listUsedTable.end(); ++it)
    {
        (*it)->GetPlayerCount(realCount,androidCount);
        totalAndroidCount+=androidCount;
        totalRealCount+=realCount;
        gameid =  (*it)->GetGameRoomInfo()->roomId;

    }

    if(gameid==(int32_t)eGameKindId::csd)
    {
        if(totalAndroidCount==0)  totalAndroidCount = 80+random()%100;
        string str=to_string(totalRealCount)+ "+" +to_string(totalAndroidCount);
        //cout<<str<<endl;
        REDISCLIENT.set(strNodeValue,str,120);
        LOG_ERROR << " strNodeValue  "<<strNodeValue.c_str()<<"     str   " <<str;
    }
    else
    {
        if (totalAndroidCount>0||totalRealCount>0)
        {
            string str=to_string(totalRealCount)+ "+" +to_string(totalAndroidCount);
            //cout<<str<<endl;
            REDISCLIENT.set(strNodeValue,str,120);
            LOG_ERROR << " strNodeValue  "<<strNodeValue.c_str()<<"     str   " <<str;

        }
    }

}

// set the special table stock information data.
bool CGameTableManager::SetTableStockInfo(tagStockInfo& stockinfo)
{
    m_stockInfo = stockinfo;
    return true;
}

//// try to do the special on time tick content now.
//void CGameTableManager::OnTimerTick(float delta)
//{
//    ListTableFrame usedTableList;
//    {
//        MutexLockGuard lock(m_lockUsedTable);
//        usedTableList = m_listUsedTable;
//    }

//    for (auto& it : usedTableList)
//    {
//        long tick = Utility::gettimetick();
//        ITableFrame* pITableFrame = it;
//        if(pITableFrame)
//        {
//            pITableFrame->OnTimerTick(delta);
//        }

//        // try to add the special tick item value.
//        long sub = Utility::gettimetick() - tick;
//        if (sub > TIMEOUT_PERFRAME) {
//            LOG_ERROR << " >>> processRequest timeout, CGameTableManager::OnTimerTick, time used:" << sub;
//        }
//    }
//}

// try to clear all the data now.
void CGameTableManager::Clear()
{
    m_vecGameTable.clear();
    m_listFreeTable.clear();
    m_listUsedTable.clear();
}
