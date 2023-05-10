#ifndef MATCHROOM_H
#define MATCHROOM_H

#include "IMatchRoom.h"

class MatchRoom : public IMatchRoom
{

public:
    MatchRoom(uint16_t matchId);

    virtual bool CanJoinMatch(shared_ptr<CServerUserItem> &pIServerUserItem)override;
    virtual bool CheckJionMatch(shared_ptr<CServerUserItem> &pIServerUserItem)override;
    virtual bool CanLeftMatch();
    virtual bool JoinMatch(shared_ptr<CServerUserItem>& pIServerUserItem,Header *commandHeader=NULL)override;

    virtual bool OnUserOffline(shared_ptr<CServerUserItem>& pIServerUserItem) override;
    virtual bool OnUserOnline(shared_ptr<CServerUserItem>& pIServerUserItem) override;


    virtual void TableFinish(uint32_t tableID) override;
    virtual void UserOutTable(uint32_t userId) override;
    //获取库存状态
    virtual int8_t GetStorageStatus() override;

protected:
    void StartMatch();
    void AllocateRoom();  //用户分配房间
    void AllocateUser();  //分配用户

private:
    TimerId m_allocateTimer;
    atomic_bool m_is_StartAllcateTimer_;
    uint32_t    m_cur_round_finish_count; //这轮玩成游戏人数
};

#endif // MATCHROOM_H
