#include "DDZTableFrameSink.h"
#include "DouDiZhu.Message.pb.h"
#include "GameRecord/DDZGameRecordManager.h"
#include "GameScore/DDZGameScoreManager.h"
#include "Card/DDZCardAlloterMgr.h"
#include "Card/DDZCardGroup.h"
#include "Card/DDZPreCardDequeMgr.h"
#include <Protocol/CommonAntithesisGame.Message.pb.h>


#define GAME_TIME_STATE_LOG_FILE CFG_GAME_NAME(this->m_ConfigPath)+"_TIMER"
#define DDZCardAlloterMgr ((CDDZCardAlloterMgr*)CardAlloterMgr)
#define DDZPlayerManager ((CDDZPlayerMgr*)this->m_PlayerManager)
DDZTableFrameSink::DDZTableFrameSink()
{
    InitGameDesk();    
    this->RegisterEventSender(GET_GLOBAL_SENDER((EventID)EGLobalEventID::EGAME_UPDATE_USER_CARD));
    this->BindOnGameEventMessage((EventID)EGLobalEventID::EGAME_UPDATE_USER_CARD,boost::bind(&DDZTableFrameSink::onUpdateGameCardInfo,this,_1));


    REGISTER_GAME_ROUND_LOGIC_INSTANCE("DouDiZhuGameRoundData");
}
Utility::CBasePlayerAreaScoreManager * DDZTableFrameSink::CreatePlayerAreaScoreMgr()
{
    return new CDDZGameScoreManager();
}
Utility::CBaseCardAlloterMgr* DDZTableFrameSink::CreateCardAlltorMgr()
{
    return new CDDZCardAlloterMgr();
}
Utility::CBaseGameRecordMgr* DDZTableFrameSink::CreateCardGameRecordMgr()
{
    return new CDDZGameRecordManager();
}

Utility::CBaseGamePlayerMgr *DDZTableFrameSink::CreatePlayerMgr()
{
    CDDZPlayerMgr* pMgr = new CDDZPlayerMgr();
    pMgr->setGameConfigPath(this->GetGameConfigPath());
    return pMgr;
}
Utility::CBaseCardGroup* DDZTableFrameSink::CreateGameCardGroup()
{
    return new CDDZCardGroup();
}
Utility::CBasePreOutCardDequeMgr *DDZTableFrameSink::CreatePreOutCardMgr()
{
    CDDZPreCardDequeMgr *pMgr = new CDDZPreCardDequeMgr();
    return pMgr;
}

bool DDZTableFrameSink::LoadGameNetEvent()
{
    this->m_OnMessage.clear();
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_QUERY_TASKLIST_REQ,boost::bind(&DDZTableFrameSink::OnQueryTaskList,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_OUT_CARD_REQ,boost::bind(&DDZTableFrameSink::OnUserOutCard,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_ADDMUTIPLIER_REQ,boost::bind(&DDZTableFrameSink::OnAddMutiplier,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_SNATCH_LANDLORD_REQ,boost::bind(&DDZTableFrameSink::OnSnatchLandlord,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_RECEIVE_TASK_REMUNERATION_REQ,boost::bind(&DDZTableFrameSink::OnReciveTaskRemuneration,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_READY_REQ,boost::bind(&DDZTableFrameSink::OnReady,this,_1,_2,_3,_4));
    this->BindOnGameMessage(CommonAntithesisGame::SUB_C_TRUSTEESHIP_GAME_REQ,boost::bind(&DDZTableFrameSink::OnAutoPlayCardStateChageReq,this,_1,_2,_3,_4));
    return true;
}
bool DDZTableFrameSink::LoadTimerEvent()
{
    m_cbGameStatus = DouDiZhu::STATUS_SCENE::SCENE_FREE;
    this->m_OnTimerFunc.clear();
    this->BindTimerEvent(CommonAntithesisGame::SUBID::SUB_C_OUT_CARD_REQ,boost::bind(&DDZTableFrameSink::DispatchAutoOutCardReq,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_FREE,boost::bind(&DDZTableFrameSink::OnTimerGameFree,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_START,boost::bind(&DDZTableFrameSink::OnTimerGameStart,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_SNATCH_LANDLORD,boost::bind(&DDZTableFrameSink::OnTimerSnatchLandlord,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_DISPLAYLASTCARD,boost::bind(&DDZTableFrameSink::OnTimerDisplayLastCards,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_ADDMULTIER,boost::bind(&DDZTableFrameSink::OnTimerAddMutiplier,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_PLAYING,boost::bind(&DDZTableFrameSink::OnTimerPlaying,this,_1,_2));
    this->BindTimerEvent(CommonAntithesisGame::STATUS_SCENE::SCENE_END,boost::bind(&DDZTableFrameSink::OnTimerGameEnd,this,_1,_2));
    return true;
}


void DDZTableFrameSink::LoadSyncGameStateFunc()
{
    this->m_SyncGameStateFunc.clear();
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_FREE,boost::bind(&DDZTableFrameSink::OnSyncGameFree,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_START,boost::bind(&DDZTableFrameSink::OnSyncGameStart,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_SNATCH_LANDLORD,boost::bind(&DDZTableFrameSink::OnSyncSnatchLandlord,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_ADDMULTIER,boost::bind(&DDZTableFrameSink::OnSyncAddMutiplier,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_PLAYING,boost::bind(&DDZTableFrameSink::OnSyncPlaying,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_END,boost::bind(&DDZTableFrameSink::OnSyncGameEnd,this,_1,_2));
    this->BindSyncGameStateFuncMessage(CommonAntithesisGame::STATUS_SCENE::SCENE_DISPLAYLASTCARD,boost::bind(&DDZTableFrameSink::OnSyncDisplayLastCard,this,_1,_2));
}

bool DDZTableFrameSink::CheckOutCard(int userid, CommonAntithesisGame::CardGroup *pGroup)
{

    RETURN_FALSE_IF(userid != this->m_CurrentOutCardUserID);
    IServerUserItem* pUser =  this->m_pITableFrame->GetUserItemByUserId(this->m_CurrentOutCardUserID);
    Utility::GAME_CARD_GROUP_MAP_ITER it = this->m_GameCards.find(this->m_CurrentOutCardUserID);
    CDDZCardGroup OutGroup;
    OutGroup.AppendGroup(pGroup);
    bool bret= DDZCardAlloterMgr->CheckOutCardGroup(pUser->GetChairID(),&OutGroup);
    if(bret || pGroup->cards_size() == 0)
    {
        LOG_TOOL_INFO("Userid:%d ChairID:%d Out Cards:%s",pUser->GetUserID(),pUser->GetChairID(),OutGroup.FormatToString(",").c_str());
        CDDZCardGroup *pGp = new CDDZCardGroup();
        pGp->AppendGroup(pGroup);
        pGp->SetGrupAttrib(Utility::ECardGroupParam::EGROUP_USER_ID,new Utility::CAutoData<int>(userid));
        pGp->SetGrupAttrib(Utility::ECardGroupParam::EGROUP_TYPE,new Utility::CAutoData<int>(pGp->GetCardGroupType()));
        this->m_PreOutCardDeque->Add(pGp);
        return true;
    }
    LOG_TOOL_ERROR("Userid:%d ChairID:%d Out Cards:%s Failed:",pUser->GetUserID(),pUser->GetChairID(),OutGroup.FormatToString(",").c_str());
    OutGroup.RemoveAllItem();
    return false;
}
bool DDZTableFrameSink::CalculateSuggestCards(::google::protobuf::RepeatedPtrField<CommonAntithesisGame::CardGroup> *pGroup)
{
    Utility::GAME_CARD_GROUP_MAP_ITER it = this->m_GameCards.find(this->m_CurrentOutCardUserID);
    if(it==this->m_GameCards.end()) return false;
    CDDZCardGroup* pSelfGroup = (CDDZCardGroup*)it->second;
    CDDZCardGroup* pPreGroup =(CDDZCardGroup*) this->m_PreOutCardDeque->GetPreOutGroup();
    return DDZCardAlloterMgr->CalculateSuggestCards(pSelfGroup,pPreGroup,pGroup);
}

bool DDZTableFrameSink::CheckAutoOutCard(CommonAntithesisGame::CardGroup *pCardGroup)
{
    return this->AutoOutCard(pCardGroup,false);
}

bool DDZTableFrameSink::AutoOutCard(CommonAntithesisGame::CardGroup* pCardGroup,bool dispathcevent)
{
    Utility::GAME_CARD_GROUP_MAP_ITER it = this->m_GameCards.find(this->m_CurrentOutCardUserID);
    if(it==this->m_GameCards.end()) return false;
    IServerUserItem* pSelf = this->m_pITableFrame->GetUserItemByUserId(this->m_CurrentOutCardUserID);
    return DDZCardAlloterMgr->AutoOutCard(pSelf->GetChairID(),pCardGroup,dispathcevent);
}

void DDZTableFrameSink::InitGameDesk()
{
    CAntithesisTableSink::InitGameDesk();

    m_cbGameStatus = Common::SCENE_FREE;
    m_dwJettonTime=0;
    m_GameRoomName = CFG_GAME_NAME(this->GetGameConfigPath()).c_str();
}

void DDZTableFrameSink::TryModifyBaseMutilter(CommonAntithesisGame::CardGroup *cards)
{
    CDDZCardGroup OutGroup;
    OutGroup.AppendGroup(cards);
    std::string groupName = this->getCardGroupElemTypeName(OutGroup.GetCardGroupType());
    int addMutilter = GET_APP_CONFIG_INSTNACE(this->m_ConfigPath)->GetConfigInt("GAME_GROUP_MULTIPLIER",groupName,0);
    if(addMutilter != 0 )
    {
        this->AddOutPutCardMutiplier(OutGroup.GetCardGroupType(),addMutilter);
    }
    OutGroup.RemoveAllItem();

}
string DDZTableFrameSink::getCardGroupElemTypeName(int type)
{
    switch (type)
    {
        case CommonAntithesisGame::CARD_GROUP_TYEP::HUOJIAN                 :return "HUOJIAN";
        case CommonAntithesisGame::CARD_GROUP_TYEP::ZHADANG                 :return "ZHADANG";
        case CommonAntithesisGame::CARD_GROUP_TYEP::DUIZI                   :return "DUIZI";
        case CommonAntithesisGame::CARD_GROUP_TYEP::SANGZHANG               :return "SANGZHANG";
        case CommonAntithesisGame::CARD_GROUP_TYEP::SANG_DAI1               :return "SANG_DAI1";
        case CommonAntithesisGame::CARD_GROUP_TYEP::SANG_DAIDUI             :return "SANG_DAIDUI";
        case CommonAntithesisGame::CARD_GROUP_TYEP::DANG_SHUN               :return "DANG_SHUN";
        case CommonAntithesisGame::CARD_GROUP_TYEP::SHUANG_SHUN             :return "SHUANG_SHUN";
        case CommonAntithesisGame::CARD_GROUP_TYEP::FEIJI                   :return "FEIJI";
        case CommonAntithesisGame::CARD_GROUP_TYEP::FEIJI_DAI_CHI_BANG		:return "FEIJI_DAI_CHI_BANG";
        case CommonAntithesisGame::CARD_GROUP_TYEP::SI_DAI2                 :return "SI_DAI2";
        case CommonAntithesisGame::CARD_GROUP_TYEP::DANG_ZHANG              :return "DANG_ZHANG";
        case CommonAntithesisGame::CARD_GROUP_TYEP::CHUNTIAN                :return "CHUNTIAN";
        case CommonAntithesisGame::CARD_GROUP_TYEP::FANGCHUN                :return "FANGCHUN";
    }
    return "";
}

Utility::GAME_MULTIPLIER_CFG_MAP DDZTableFrameSink::CalculateAreaGameMutiplier()
{
    Utility::GAME_MULTIPLIER_CFG_MAP mutiplier;
    return mutiplier;
}

bool DDZTableFrameSink::onUpdateGameCardInfo(const CBaseEventData &data)
{
    CGameUpdateGameCardEventData* pData = (CGameUpdateGameCardEventData*)&data;
    this->m_LastCardGroup->RemoveAllItem();
    this->ClearGameCard();
    this->m_LastCardGroup->WriteCardData(pData->m_cbBankerCard,sizeof(pData->m_cbBankerCard));
    AntithesGamePlayerMgr->Visit([](IServerUserItem* pUser,CGameUpdateGameCardEventData* upData,GAME_CARD_GROUP_MAP& GameCards)
    {
        int ChairID = pUser->GetChairID();
        GAME_CARD_GROUP_MAP_ITER it =  GameCards.find(pUser->GetUserID());
        if(it == GameCards.end())
        {
            GameCards.insert(std::make_pair(pUser->GetUserID(),new CDDZCardGroup()));
            it = GameCards.find(pUser->GetUserID());
        }
        CDDZCardGroup* pGP = (CDDZCardGroup*)it->second;
        pGP->WriteCardData(upData->m_cbHandCardData[ChairID],upData->m_cbHandCardCount[ChairID]);

        CDDZCardGroup::Instance().RemoveAllItem();
        CDDZCardGroup::Instance().WriteCardData(upData->m_cbHandCardData[ChairID],upData->m_cbHandCardCount[ChairID]);
        LOG_INFO<<"---------------------DDZTableFrameSink::onUpdateGameCardInfo"<<"ChairID:"<<ChairID<<" Cards:"<<CDDZCardGroup::Instance().FormatToString(",");
        return true;
    },pData,this->m_GameCards);

    CDDZCardGroup::Instance().RemoveAllItem();
    return true;
}

void DDZTableFrameSink::AddGameRecord()
{

}
void DDZTableFrameSink::WriteGameRoundLog()
{

#ifdef USE_LOG_TOOL
    std::vector<Utility::BaseGameRoundSQLRecord*> records;
    std::map<int ,ADD_MULI_ITEM>::iterator aditemIter ;
    std::string tm = this->getLocalTime();
    LOG_TOOL_WRITE_ROUND_LOG("========================================================================================%d局开始 ========================================================================================",this->m_GameRoundTimes);
    for(Utility::GAME_RESULT_RESULT_MAP_ITER it = this->m_LastGameResult.begin();it != this->m_LastGameResult.end(); it++)
    {
        int userid = it->first;
        IServerUserItem* pUser = this->m_pITableFrame->GetUserItemByUserId(userid);
        if(pUser == nullptr)
        {
            LOG_TOOL_WRITE_ROUND_LOG("User Not Found userid:%s",to_string(userid).c_str());
            continue;
        }
        Utility::LPPLAYER_GAME_SCORE_RESULT rst = &it->second;
        DDZGameRoundSQLRecord *precord = new DDZGameRoundSQLRecord();
        precord->user_id = userid;
        if(rst->winScore>0)precord->defeng = rst->winScore;
        if(rst->lostScore<0)precord->defeng = rst->lostScore;
        precord->user_score = pUser->GetUserScore();
        precord->BaseScore = this->m_RoomKindInfo->nCellScore;
        precord->times = this->m_GameRoundTimes;
        precord->userJob = AntithesGamePlayerMgr->CheckIsLandlord(pUser);
        precord->LanlordScore = AntithesGamePlayerMgr->GetLandlordMutiplier(pUser->GetUserID());
        if(precord->userJob == 1 && precord->defeng > 0) precord->winArea =1;
        if(precord->userJob == 1 && precord->defeng < 0) precord->winArea =0;
        if(precord->userJob == 0 && precord->defeng > 0) precord->winArea =0;
        if(precord->userJob == 0 && precord->defeng < 0) precord->winArea =1;

        precord->AddMuilpter = AntithesGamePlayerMgr->GetPlayerMultiplier(pUser->GetUserID());
        precord->revenue = rst->revenue;
        aditemIter = this->m_AddMuiplierTypeDic.find(CommonAntithesisGame::CARD_GROUP_TYEP::HUOJIAN);
        precord->hasHuoJian = (aditemIter == this->m_AddMuiplierTypeDic.end()) ? 0 : ((ADD_MULI_ITEM*)(&aditemIter->second))->mucount;
        aditemIter = this->m_AddMuiplierTypeDic.find(CommonAntithesisGame::CARD_GROUP_TYEP::ZHADANG);
        precord->bormCount = (aditemIter == this->m_AddMuiplierTypeDic.end()) ? 0 : ((ADD_MULI_ITEM*)(&aditemIter->second))->mucount;

        aditemIter = this->m_AddMuiplierTypeDic.find(CommonAntithesisGame::CARD_GROUP_TYEP::CHUNTIAN);
        precord->isSpring = (aditemIter != this->m_AddMuiplierTypeDic.end()) ? 1 : 0;

        aditemIter = this->m_AddMuiplierTypeDic.find(CommonAntithesisGame::CARD_GROUP_TYEP::FANGCHUN);
        precord->isRevertSpring = (aditemIter != this->m_AddMuiplierTypeDic.end()) ? 1 : 0;

        records.push_back(precord);
        LOG_TOOL_WRITE_ROUND_LOG("时间:%s   编号:%d   账号:%s   底分:%s   叫分:%s   加倍:%s   角色:%s   春天:%s   反春:%s   火箭:%s   炸弹:%s   得分:%s   总局数:%s   输:%s   赢:%s   税收:%s   玩家积分:%s"
                                 ,tm.c_str(),
                                 userid,pUser->GetNickName(),
                                 to_string(precord->BaseScore).c_str(),
                                 to_string(precord->LanlordScore).c_str(),
                                 to_string(precord->AddMuilpter).c_str(),
                                 precord->userJob==1?"地主":"农民",
                                 precord->isSpring==1?"是":"否",
                                 precord->isRevertSpring==1?"是":"否",
                                 precord->hasHuoJian>0?"有":"无",
                                 precord->bormCount>0?to_string(precord->hasHuoJian).c_str():"无",
                                 to_string(precord->defeng).c_str(),
                                 to_string(pUser->GetPlayCount()).c_str(),
                                 to_string(pUser->GetPlayCount()-(pUser->GetWinCount())).c_str(),
                                 to_string(pUser->GetWinCount()).c_str(),
                                 to_string(rst->revenue).c_str(),
                                 to_string(pUser->GetUserScore()).c_str());
    }
    LOG_SQL_GAME_ROUND_RECORD(records);
    LOG_TOOL_WRITE_ROUND_LOG("时间:%s   第%d局桌台数据 ",tm.c_str(),this->m_GameRoundTimes);
    LOG_TOOL_WRITE_ROUND_LOG("========================================================================================%d局结束========================================================================================",this->m_GameRoundTimes);
    LOG_TOOL_FLASH_GAMEROUND_DATA();
#endif
}

bool DDZTableFrameSink::OnGameStartDispatchTableCard()
{
    std::vector<int> uids;
    this->m_PlayerManager->Visit([](IServerUserItem* pUser,std::vector<int>& uids){
        uids.push_back(pUser->GetUserID());
        return true;
    },uids);
    this->ClearGameCard();
    this->m_GameCards.clear();
    this->m_LastCardGroup->Clear();
    CardAlloterMgr->Init(CFG_ONE_ROUND_GAME_GROUPS_COUNT(this->m_ConfigPath),this,this->m_pITableFrame);
    ((CDDZCardAlloterMgr*)CardAlloterMgr)->AlloterCardsGameStartCards(this->m_GameCards,uids);
    return true;
}
string DDZTableFrameSink::GetGameConfigPath()
{
    return "conf/ddz_config.ini";
}

void DDZTableFrameSink::DeduceAreaWinner(Utility::WIN_AREA_TYPE_MAP& winData)
{
    winData.clear();
    IServerUserItem* lanlor = DDZPlayerManager->GetLandlordPlayer();
    Utility::GAME_CARD_GROUP_MAP_ITER it =  this->m_GameCards.find(lanlor->GetUserID());
    CDDZCardGroup* pLanlorGroup =(CDDZCardGroup*) it->second;
    bool lanlorWin = pLanlorGroup->GetSize() == 0;
    for(int idx = 0; idx < DDZPlayerManager->GetSize();idx++)
    {
        IServerUserItem* pUser = nullptr;
        DDZPlayerManager->TryToGet(idx,pUser);
        if(lanlorWin)
        {
            winData.insert(std::make_pair(pUser->GetUserID(),lanlor == pUser ? 1 : -1));
        }else
        {
            winData.insert(std::make_pair(pUser->GetUserID(),lanlor == pUser ? -1 : 1));
        }
    }

}
bool DDZTableFrameSink::CalculateSpring()
{
    IServerUserItem* lanlor = DDZPlayerManager->GetLandlordPlayer();
    int lanlorTimes = DDZPlayerManager->GetOutCardTimes(lanlor->GetUserID());
    CDDZCardGroup* pdg =(CDDZCardGroup* )this->m_GameCards.find(lanlor->GetUserID())->second;
    int lanlorCardsCount = pdg->GetSize();
    if(lanlorTimes == 1 && lanlorCardsCount > 0)
    {
        this->AddOutPutCardMutiplier((int)CommonAntithesisGame::CARD_GROUP_TYEP::FANGCHUN,2);
        return true;
    }
    if(lanlorCardsCount != 0 ) return false;
    for(int idx = 0; idx < DDZPlayerManager->GetSize();idx++)
    {
        IServerUserItem* pUser = nullptr;
        DDZPlayerManager->TryToGet(idx,pUser);
        if(pUser == lanlor) continue;
        int times = DDZPlayerManager->GetOutCardTimes(pUser->GetUserID());
        if(times > 0 )return false;
    }
    this->AddOutPutCardMutiplier((int)CommonAntithesisGame::CARD_GROUP_TYEP::CHUNTIAN,2);
    return true;
}
bool DDZTableFrameSink::CalculateScore()
{
    this->CalculateSpring();
    this->DeduceAreaWinner(this->m_winData);
    this->m_LastGameResult.clear();
    for(int idx = 0; idx < DDZPlayerManager->GetSize();idx++)
    {
        IServerUserItem* pUser = nullptr;
        DDZPlayerManager->TryToGet(idx,pUser);
        int landlordMutiplier = DDZPlayerManager->GetLandlordMutiplier(pUser->GetUserID());
        int addMultiplier = DDZPlayerManager->GetPlayerMultiplier(pUser->GetUserID());
        if(1==DDZPlayerManager->CheckIsLandlord(pUser))addMultiplier = 1;
        int outCardMultipiler = this->GetOutCardMuiplier();
        Utility::WIN_AREA_TYPE_MAP_ITER winIt = this->m_winData.find(pUser->GetUserID());
        SCORE uScore = this->m_RoomKindInfo->nCellScore * landlordMutiplier * addMultiplier * outCardMultipiler *1.0 ;
        if(1==DDZPlayerManager->CheckIsLandlord(pUser))uScore = uScore * 2.0;
        uScore = uScore * winIt->second;
        Utility::PLAYER_GAME_SCORE_RESULT rst;
        if(uScore > 0)
        {
            rst.winScore = uScore;
            rst.revenue = this->m_pITableFrame->CalculateRevenue(pUser->GetChairID(),rst.winScore);
            DDZPlayerManager->setAttrib(pUser->GetUserID(),Utility::EPLAYER_ATTRIB_KEY::EGAME_END_SCORE,new Utility::CAutoData<SCORE>(uScore-rst.revenue));
        }else
        {
            rst.lostScore = uScore;
            DDZPlayerManager->setAttrib(pUser->GetUserID(),Utility::EPLAYER_ATTRIB_KEY::EGAME_END_SCORE,new Utility::CAutoData<SCORE>(uScore));
        }
        LOG_TOOL_INFO("DDZTableFrameSink::CalculateScore uid:%d chairid:%d winScore:%s lostScore:%s ",pUser->GetUserID(),pUser->GetChairID(),to_string( rst.winScore).c_str(),to_string(rst.lostScore).c_str());
        this->m_LastGameResult.insert(std::make_pair(pUser->GetUserID(),rst));
    }
    return true;
}

extern "C" ITableFrameSink *CreateTableFrameSink(void)
{
    Utility::InitGlobalEvent();
    DDZTableFrameSink *tableFrameSink = new DDZTableFrameSink();
    return dynamic_cast<ITableFrameSink*> (tableFrameSink);
}


string DDZTableFrameSink::DDZGameRoundSQLRecord::GetParamStr()
{
     char filedValue[10024] = {0};
     snprintf(filedValue,sizeof(filedValue),"%s,%s,%s,%s,%s,%s,%s,%s"
              ,to_string(this->BaseScore).c_str()
              ,to_string(this->LanlordScore).c_str()
              ,to_string(this->AddMuilpter).c_str()
              ,to_string(this->bormCount).c_str()
              ,to_string(this->isSpring).c_str()
              ,to_string(this->isRevertSpring).c_str()
              ,to_string(this->hasHuoJian).c_str()
              ,to_string(this->userJob).c_str());
     return filedValue;
}

string DDZTableFrameSink::DDZGameRoundSQLRecord::GetParamFiledStr()
{
    return "`base_score`,`lanlord_score`,`add_muilpter`,`borm_count`,`is_spring`,`is_revertspring`,`has_huojian`,`user_job`";
}
