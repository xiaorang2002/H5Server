
#include "ServerUserItem.h"
#include "ServerUserManager.h"

#include <muduo/base/Logging.h>

CServerUserManager::CServerUserManager() : m_pGameRoomInfo(NULL)
{

}

CServerUserManager::~CServerUserManager()
{
    m_UsedUserItemList.clear();
    m_FreeUserItemList.clear();
    m_UserMap.clear();
}

void CServerUserManager::Init(tagGameRoomInfo* pGameRoomInfo)
{
    if(pGameRoomInfo)
    {
        m_pGameRoomInfo = pGameRoomInfo;
    }
}

// try to insert the special user item.
shared_ptr<CServerUserItem> CServerUserManager::GetUserItem(int64_t userId)
{
    shared_ptr<CServerUserItem> serverUserItem;
    if(m_FreeUserItemList.size() > 0)
    {
        {
            WRITE_LOCK(m_mutex);
            serverUserItem = m_FreeUserItemList.back();
            m_FreeUserItemList.pop_back();
        }
        serverUserItem->ResetUserItem();
    }else
    {
        try
        {
            serverUserItem = shared_ptr<CServerUserItem>(new CServerUserItem());
        }catch(...)
        {
            assert(false);
            return serverUserItem;
        }
    }

    {
        WRITE_LOCK(m_mutex);
        m_UsedUserItemList.push_back(serverUserItem);
        m_UserMap[userId]  = serverUserItem;
    }
    return serverUserItem;
}

shared_ptr<CServerUserItem> CServerUserManager::FindUserItem(int64_t userId)
{
    shared_ptr<CServerUserItem> serverUserItem;
    serverUserItem.reset();
    {
        READ_LOCK(m_mutex);
        auto iter = m_UserMap.find(userId);
        if(iter != m_UserMap.end())
        {
            serverUserItem = m_UserMap[userId];
            //LOG_DEBUG << ">>>>>>>>>>>>>>>>>>>>Find CServerUserItem: UserId:"<<to_string(serverUserItem->GetUserId())<<" UserId:"<<to_string(userId)<<"<<<<<<<<<<<<<<<<<<<<<<<<<";
        }
    }
    return serverUserItem;
}

bool CServerUserManager::DeleteUserItem(shared_ptr<CServerUserItem>& serverUserItem)
{
    if(!serverUserItem)
    {
        return false;
    }

    int userId  = serverUserItem->GetUserId();
    int tableId = serverUserItem->GetTableId();
    int chairId = serverUserItem->GetChairId();
    //LOG_DEBUG << ">>> DeleteUserItem called, tableid:" << tableId
    //          << ", chairId:" << chairId
    //          << ", userid:" << userId;

    shared_ptr<CServerUserItem> item;
    WRITE_LOCK(m_mutex);
    for(auto iter = m_UsedUserItemList.begin(); iter != m_UsedUserItemList.end(); ++iter)
    {
        item = *iter;
        if (item.get() != serverUserItem.get())
        {
            continue;
        }
        m_FreeUserItemList.push_back(serverUserItem);
        m_UsedUserItemList.erase(iter);
        m_UserMap.erase(userId);
        serverUserItem->ResetUserItem();
        return true;
    }
    return false;
}

void CServerUserManager::GetAgentPlayersMap(map<uint32_t, uint32_t> &agentPlayers)
{
    uint32_t agentid=0;
    shared_ptr<CServerUserItem> UserItem;
    READ_LOCK(m_mutex);
    for(auto user:m_UserMap)
    {
        UserItem=user.second;
        agentid=UserItem->GetUserBaseInfo().agentId;
        auto iter=agentPlayers.find(agentid);
        if(iter==agentPlayers.end())
        {
            agentPlayers.emplace(agentid,1);
        }else
        {
            agentPlayers[agentid] ++;
        }
    }
}

//void CServerUserManager::UpdateUserCount(int64_t userId, uint8_t updateMode)
//{
//    if (pNetCenter != NULL && pNetCenter->IsRunning())
//    {
//        CMD_Update_GS_User updateUser;
//        updateUser.dwServerID = CServerCfg::m_nServerID;
//        updateUser.wGameID = m_pGameRoomKindInfo->wGameID;
//        updateUser.wKindID = m_pGameRoomKindInfo->wKindID;
//        updateUser.wRoomKindID = m_pGameRoomKindInfo->wRoomKindID;
//        CT_WORD wAndroidCount = CAndroidUserMgr::get_instance().GetOnlineAndroidCount();
//        updateUser.wRealUserCount = (CT_WORD)m_UserMap.size();
//        updateUser.wUserCount = updateUser.wRealUserCount + wAndroidCount;

//        updateUser.dwUserID = dwUserID;
//        updateUser.cbUpdateMode = cbUpdateMode;

//        CNetModule::getSingleton().Send(pNetCenter->GetSocket(), MSG_GCS_MAIN, SUB_G2CS_UPDATE_USER, &updateUser, sizeof(CMD_Update_GS_User));
//    }
//}

/* no use yet.
void CServerUserManager::SetUserOffLine(TcpConnectionWeakPtr &pSocket, ILogicThread* pLogicThread)
{
    do
    {
        if (m_UserMap.empty()) break;
        for (auto it = m_UserMap.begin(); it != m_UserMap.end(); ++it)
        {
            TcpConnectionWeakPtr m_pSocket = it->second->GetUserConnect();
            if (m_pSocket.lock() == pSocket.lock())
            {
                if (pLogicThread) {
                    LOG_INFO << "OnUserOffline userid:" << it->first;
                    pLogicThread->OnUserOffline(it->first);
                }
            }
        }
    } while (0);
//Cleanup:
    return;
}
*/

/*
CServerUserItem* CServerUserManager::FindUserItemFromTcp(TcpConnectionWeakPtr &pSocket)
{
    CServerUserItem* pIServerUserItem = NULL;
    if (m_UserMap.empty())
    {
        return pIServerUserItem;
    }

    for (auto it = m_UserMap.begin(); it != m_UserMap.end(); ++it)
    {
        TcpConnectionWeakPtr m_pSocket = it->second->GetUserConnect();
        if (m_pSocket.lock() == pSocket.lock())
        {
            pIServerUserItem = it->second;
        }
    }
    return pIServerUserItem;
}
*/
