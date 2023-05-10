#pragma once

#include <boost/serialization/singleton.hpp>
#include <muduo/base/Mutex.h>
#include <memory>
#include <vector>
#include <map>
#include <list>
#include <stdint.h>
#include "IMatchRoom.h"

using boost::serialization::singleton;
using namespace muduo;
using namespace std;


class MatchRoomManager : public singleton<MatchRoomManager>
{
public:
    MatchRoomManager();
    virtual ~MatchRoomManager();

public:
    void InitAllMatchRoom(shared_ptr<EventLoopThread>& game_logic_thread,
                          MatchRoomInfo* pMatchRoomInfo,ILogicThread* pLogicThread);
    shared_ptr<IMatchRoom> GetMatch(uint32_t dwMatchId);
    shared_ptr<IMatchRoom> FindNormalSuitMatch(shared_ptr<CServerUserItem> &pIServerUserItem);
    void FreeNormalMatch(uint32_t matchId);
    void UpdataPlayerCount2Redis(string &strNodeValue);

public:
    void Clear();

public:
    vector<shared_ptr<IMatchRoom>>   GetAllMatch(){return vec_MatchRoom;}
    list<shared_ptr<IMatchRoom>> &   GetUsedMatch(){return list_UsedMatchRoom;}

protected:
    vector<shared_ptr<IMatchRoom>>   vec_MatchRoom;
    list<shared_ptr<IMatchRoom>>     list_FreeMatchRoom;
    list<shared_ptr<IMatchRoom>>     list_UsedMatchRoom;


};

