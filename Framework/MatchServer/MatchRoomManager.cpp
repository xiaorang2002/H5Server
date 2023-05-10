
#include <muduo/base/Logging.h>
#include "MatchRoomManager.h"
#include "MatchRoom.h"
#include "ThreadLocalSingletonRedisClient.h"

MatchRoomManager::MatchRoomManager()
{

}


MatchRoomManager::~MatchRoomManager()
{

}

void MatchRoomManager::InitAllMatchRoom(shared_ptr<EventLoopThread> &game_logic_thread,
                                        MatchRoomInfo *pMatchRoomInfo, ILogicThread *pLogicThread)
{
	LOG_DEBUG << "--- *** "
		<< pMatchRoomInfo->matchRoomId << " { "
		<< pMatchRoomInfo->matchName << " }"
		<< " 初始化比赛房间，桌子数 = " << pMatchRoomInfo->roomCount;
	for(uint32_t idx = 0; idx < pMatchRoomInfo->roomCount; ++idx)
    {
        //赛制区分时，这里要分别创建
        shared_ptr<MatchRoom> pMatchRoom(new MatchRoom(idx));
        pMatchRoom->InitMathRoom(game_logic_thread,pMatchRoomInfo,pLogicThread);

        list_FreeMatchRoom.push_back(dynamic_pointer_cast<IMatchRoom>(pMatchRoom));
        vec_MatchRoom.push_back(dynamic_pointer_cast<IMatchRoom>(pMatchRoom));
    }
}


shared_ptr<IMatchRoom> MatchRoomManager::GetMatch(uint32_t matchId)
{

    if(vec_MatchRoom.size()>matchId)
    {
        return vec_MatchRoom[matchId];
    }
    return NULL;
}

shared_ptr<IMatchRoom> MatchRoomManager::FindNormalSuitMatch(shared_ptr<CServerUserItem> &pIServerUserItem)
{
    shared_ptr<IMatchRoom> ptr_MatchRoom;
    for(auto it : list_UsedMatchRoom)
    {
        if ( it->CanJoinMatch(pIServerUserItem))
        {
            return it;
        }
    }

    if(list_FreeMatchRoom.size()>0)
    {
        ptr_MatchRoom =list_FreeMatchRoom.front();
        list_FreeMatchRoom.pop_front();

        ptr_MatchRoom->SetWaitToStart();
        list_UsedMatchRoom.push_back(ptr_MatchRoom);
    }
    return ptr_MatchRoom ;
}

void MatchRoomManager::FreeNormalMatch(uint32_t matchId)
{
    for(auto it = list_UsedMatchRoom.begin(); it != list_UsedMatchRoom.end(); ++it)
    {
        if((*it)->GetMatchId() == matchId)
        {
            list_FreeMatchRoom.push_back(*it);
            list_UsedMatchRoom.erase(it);
            break;
        }
    }
}

//更新统计人数 只统计真实人数
void MatchRoomManager::UpdataPlayerCount2Redis(string &strNodeValue)
{
    uint64_t totalAndroidCount=0;
    uint64_t totalRealCount=0;
    uint32_t androidCount=0;
    uint32_t realCount=0;
    for(auto it = list_UsedMatchRoom.begin(); it != list_UsedMatchRoom.end(); ++it)
    {
        totalRealCount+=(*it)->GetPlayerCount(true);
    }

    if (totalAndroidCount>0||totalRealCount>0)
    {
        string str=to_string(totalRealCount)+ "+" +to_string(totalAndroidCount);
        //cout<<str<<endl;
        REDISCLIENT.set(strNodeValue,str,120);
    }
}
