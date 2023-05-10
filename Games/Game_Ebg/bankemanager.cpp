#include "bankemanager.h"

BankeManager::BankeManager()
    :m_minBankerScore(0),
    m_minApplyBankerScore(0),
    m_wCurrentBanker(1000000),
    m_lBankerCurrGameScore(1000000),
    m_lBankerScore(1000000),
    m_MaxMultiplier(1),
    m_bEnableSysBanker(true),// shi fou kai qi zuozhuang
    mpCurrentBanker(nullptr),//shifoshi xitong zhuang
    m_HeadId(-1),
    m_wBankerTimes(0),
    m_bIsSysBanker(true),    //shifoshi xitong zhuang
    m_bContinueBanker(true)
{

}
bool BankeManager::CancelBanker(shared_ptr<CServerUserItem> itm)
{
    std::vector<shared_ptr<CServerUserItem>>::iterator searchUserIT = std::find(VctBankerList.begin(),VctBankerList.end(),itm);
    if(searchUserIT != VctBankerList.end())
    {
        VctBankerList.erase(searchUserIT);
        return true;
    }
    return false;
}
bool BankeManager::ApplyBanker(shared_ptr<CServerUserItem> itm)
{
    if(false == m_bEnableSysBanker) return false;
    std::vector<shared_ptr<CServerUserItem>>::iterator searchUserIT = std::find(VctBankerList.begin(),VctBankerList.end(),itm);
    if(searchUserIT != this->VctBankerList.end()) return false;
    if(itm->GetUserScore() < m_minBankerScore)
    {
        std::cout<<"ApplyBanker failure------------------userScore:"<<itm->GetUserScore()<<std::endl;
        return false;
    }
    VctBankerList.push_back(itm);
    //TryToBeBanker()
    return true;
}
bool BankeManager::TryToBeBanker()
{

    if(VctBankerList.size() >= 1)
    {
        m_wBankerTimes = 0;
        std::vector<shared_ptr<CServerUserItem>>::iterator it=VctBankerList.begin();
        for(;it!=VctBankerList.end();it++)
        {
            if((*it)->GetUserScore()>=m_minBankerScore)
            {
                mpCurrentBanker =*it;
                VctBankerList.erase(it);
                break;
            }
        }
    }
    UpdateBankerData();
    return true;
}
int BankeManager::UpdateBankerTimes()
{
    m_wBankerTimes = m_wBankerTimes + 1;
    return m_wBankerTimes;
}
//查看上把的庄家能不能继续当下去
uint8_t BankeManager::CheckBanker()
{
    if(nullptr != mpCurrentBanker)
    {
        if(!m_bContinueBanker)
        {
            mpCurrentBanker = nullptr;
            return 3;
        }
        if(m_wBankerTimes>20)
        {
            mpCurrentBanker = nullptr;
            return 1;
        }
        if(mpCurrentBanker->GetUserScore()<m_minBankerScore)
        {
            mpCurrentBanker = nullptr;
            return 2;
        }
    }
    else if(VctBankerList.size()>0)
    {
        return 4;
    }
    return 0;
}

uint32_t BankeManager::GetApplySize()
{
    return  VctBankerList.size();
}

void BankeManager::UpdateBankerInfo(ErBaGang::BankerInfo *bankerInfo)
{
    bankerInfo->Clear();
    if(nullptr==mpCurrentBanker)
    {
        bankerInfo->set_bsystembanker(m_bIsSysBanker);
    }else
    {
        bankerInfo->set_bsystembanker(m_bIsSysBanker);
        bankerInfo->set_dwuserid(m_wCurrentBanker);
        bankerInfo->set_continuecount(m_wBankerTimes);
        bankerInfo->set_headerid(m_HeadId);
        bankerInfo->set_nickname(m_BankNickName);
        bankerInfo->set_luserscore(m_lBankerScore);
    }

//            ErBaGang::SimplePlayerInfo* user =bankerInfo->add_applylist();
//            user->set_dwuserid(0);
//            user->set_headerid(0);
//            user->set_nickname("aaa");
//            user->set_luserscore(0);

    for(auto applyUser:VctBankerList)
    {
        ErBaGang::SimplePlayerInfo* user =bankerInfo->add_applylist();
        user->set_dwuserid((applyUser)->GetUserId());
        user->set_headerid((applyUser)->GetHeaderId());
        user->set_nickname((applyUser)->GetNickName());
        user->set_luserscore((applyUser)->GetUserScore());
    }
}
void BankeManager::UpdateBankerData()
{
    m_wCurrentBanker = -1;
    m_lBankerCurrGameScore = 1000000;
    m_lBankerScore = 1000000;
    m_MaxMultiplier = 1;
    m_bEnableSysBanker = true;// shi fou kai qi zuozhuang
    mpCurrentBanker = nullptr;//shifoshi xitong zhuang
    m_wBankerTimes=0;
    m_BankNickName="财神系统庄";
    m_bIsSysBanker=true;//shifoshi xitong zhuang
    m_wCurrentBankerChairID=-1;
    m_HeadId=-1;
    if(nullptr !=mpCurrentBanker)
    {
        m_lBankerCurrGameScore = mpCurrentBanker->GetUserScore();
        m_lBankerScore = mpCurrentBanker->GetUserScore();
        m_wCurrentBanker = mpCurrentBanker->GetUserId();
        m_wCurrentBankerChairID = (int64_t)mpCurrentBanker->GetChairId();
        m_BankNickName = mpCurrentBanker->GetNickName();
        m_HeadId=mpCurrentBanker->GetHeaderId();
        m_bIsSysBanker=false;
        m_bContinueBanker=true;
    }
}
