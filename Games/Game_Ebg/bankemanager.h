#ifndef BANKEMANAGER_H
#define BANKEMANAGER_H
#include "proto/ErBaGang.Message.pb.h"
#include "../../public/ITableFrame.h"
class BankeManager
{
public:
    BankeManager();

    bool CancelBanker(shared_ptr<CServerUserItem> itm);
    bool ApplyBanker(shared_ptr<CServerUserItem> itm);
    bool TryToBeBanker();
    void UpdateBankerData();
    int UpdateBankerTimes();
    uint8_t CheckBanker();
    uint32_t GetApplySize();
    void UpdateBankerInfo(ErBaGang::BankerInfo* bankerInfo);
public:
    int GetCurrentBanker()
    {
       return m_wCurrentBanker;
    }
    bool GetEnalBanker()
    {
       return m_bEnableSysBanker;
    }
    int32_t GetBankerTimes()
    {
       return m_wBankerTimes;
    }
    int64_t GetBankerScore()
    {
       return m_lBankerScore;
    }
    shared_ptr<CServerUserItem> mpCurrentBanker;
    bool IsSysBanker()
    {
        return m_bIsSysBanker;
    }
    string GetNickName()
    {
        return m_BankNickName;
    }
    int64_t GetBankerChairID()
    {
        return m_wCurrentBankerChairID;
    }
    int GetBankerHeadId()
    {
        return m_HeadId;
    }
    void SetMinBankerScore(uint64_t minBankerScore)
    {
        m_minBankerScore=minBankerScore;
    }
    void SetMinApplyBankerScore(uint64_t minBankerScore)
    {
        m_minApplyBankerScore=minBankerScore;
    }
    void SetContinueBanker(bool bContinue)
    {
        m_bContinueBanker=bContinue;
    }

 private:

    int         m_wCurrentBanker;
    int64_t     m_wCurrentBankerChairID;
    int32_t     m_wBankerTimes;
    int64_t     m_lBankerScore;
    int64_t     m_lBankerCurrGameScore;        
    int         m_HeadId;
    string      m_BankNickName;
    bool        m_bEnableSysBanker;
    int         m_MaxMultiplier;
    bool        m_bIsSysBanker;
    uint64_t    m_minBankerScore;
    uint64_t    m_minApplyBankerScore;
    bool        m_bContinueBanker;
    std::vector<shared_ptr<CServerUserItem>> VctBankerList;
};

#endif // BANKEMANAGER_H
