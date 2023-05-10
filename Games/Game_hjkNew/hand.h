#ifndef HJK_HAND_H
#define HJK_HAND_H

#include "MSG_HJK.h"
#include "GameLogic.h"

struct CompareDef
{
    uint8_t CardType;
    uint8_t CardPoint;
    uint64_t BetScore;
    int64_t WinScore;
};

//def0 is banker
static bool CompareByDef(const CompareDef def0,const CompareDef def1)
{
    if(def0.CardType != def1.CardType)
    {
        return def0.CardType > def1.CardType;
    }
    if(def0.CardType == 2)
    {
        return def0.CardPoint > def1.CardPoint;
    }
    if(def0.CardType == 1)
    {
        return def0.CardPoint > def1.CardPoint;
    }
    return true;
}

class Hand
{
public:
    Hand(uint8_t index1)
    {
        index = index1;
        chairId = index%GAME_PLAYER;
        issecond = index >= GAME_PLAYER;
        userId = 0;
        isbanker = chairId == GAME_PLAYER - 1;
        stopped = true;
        opercode = 0;
        hasA = false;
        issplited = false;
        masterChair = -1;//no player at this position and others beted
        doubled = false;
    }

    void clear()
    {
        stopped = false;
        cardcount = 2;
        betscore = 0;
        winscore = 0;
        revenue = 0;
        cardtype = 0;
        userId = 0;
        isactivated = false;
        issplited = false;
        hasA = false;
        opercode = 0;
        cardpoint = 0;
        masterChair = -1;
        insurescore = 0;
        doubled = false;
    }

public:
    int64_t userId;
    uint8_t chairId;
    uint8_t index;
    uint8_t cardtype;
    uint8_t cardpoint;
    uint8_t cardcount;
    uint8_t opercode;
    uint64_t insurescore;
    bool hasA;
    bool isbanker;
    bool isactivated;
    bool issecond;
    bool issplited;
    bool stopped;
    bool doubled;
    uint64_t betscore;
    uint64_t winscore;
    uint64_t revenue;
    int32_t masterChair;
    shared_ptr<Hand> next;
};
#endif // HAND_H
