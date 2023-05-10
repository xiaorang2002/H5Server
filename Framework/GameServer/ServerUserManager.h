#pragma once


#include <map>
#include <list>
#include <stdint.h>

#include <boost/serialization/singleton.hpp>
#include <boost/pool/object_pool.hpp>

#include <muduo/base/Mutex.h>

#include "public/Globals.h"
#include "ServerUserItem.h"

using namespace std;
using namespace muduo;

using boost::serialization::singleton;


class CServerUserManager : public singleton<CServerUserManager>
{
public:
    CServerUserManager();
    virtual ~CServerUserManager();

public:
    void Init(tagGameRoomInfo* pGameRoomInfo);

    shared_ptr<CServerUserItem> GetUserItem(int64_t userId);
    shared_ptr<CServerUserItem> FindUserItem(int64_t userId);
    bool DeleteUserItem(shared_ptr<CServerUserItem>& serverUserItem);
    void GetAgentPlayersMap(map<uint32_t,uint32_t>& agentPlayers);
//public:
//    void UpdateUserCount(int64_t userId, uint8_t updateMode);

protected:
    map<int64_t, shared_ptr<CServerUserItem>>        m_UserMap;
    list<shared_ptr<CServerUserItem>>                m_UsedUserItemList;
    list<shared_ptr<CServerUserItem>>                m_FreeUserItemList;

    tagGameRoomInfo*                                 m_pGameRoomInfo;

private:
    mutable boost::shared_mutex m_mutex;

};

