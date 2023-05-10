#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/algorithm.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <muduo/net/EventLoopThread.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/TimerId.h>
#include <muduo/base/Logging.h>

#include <sys/time.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string> 
#include <iostream>
#include <math.h>
#include <deque>
#include <vector>
#include <list>
#include <map>
#include <algorithm>
#include <utility>
#include <functional>
#include <chrono>
#include <random>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "ServerUserItem.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
//#include "public/pb2Json.h"

#include "proto/Gswz.Message.pb.h"

#include "../Game_Gswz/GameLogic.h"
#include "../Game_Gswz/MSG_GSWZ.h"

using namespace Gswz;

#include "AndroidUserItemSink.h"

//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS						800									//最少时间
#define TIME_DELAY_TIME					1500									//延时时间

//游戏时间
#define TIME_USER_ADD_SCORE				2000									//下注时间

//游戏时间
#define IDI_USER_ADD_SCORE				(101)								//下注定时器

//////////////////////////////////////////////////////////////////////////


CAndroidUserItemSink::CAndroidUserItemSink()
    : m_pTableFrame(NULL)
    , m_wChairID(INVALID_CHAIR)
    , m_pAndroidUserItem(NULL)
{

    m_lStockScore = 0;
    m_wCurrentUser = INVALID_CHAIR;
    m_cbCardType = 0;


    m_lMaxCellScore = 0;
    m_lUserMaxScore = 0;
    m_lMinJettonScore = 0;
    m_cbHandCardCount=0;
    memset(m_lTableScore, 0, sizeof(m_lTableScore));


    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
    memset(m_cbRealPlayer, 0, sizeof(m_cbRealPlayer));
    memset(m_cbAndroidStatus, 0, sizeof(m_cbAndroidStatus));


    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbAllHandCardData, 0, sizeof(m_cbAllHandCardData));
    memset(m_cbHandCardType, 0, sizeof(m_cbHandCardType));

    memset(winAllInProb_, 0, sizeof(winAllInProb_));
    memset(winAddScoreProb_, 0, sizeof(winAddScoreProb_));
    memset(winPassScoreProb_, 0, sizeof(winPassScoreProb_));
    memset(winFollowScoreProb_, 0, sizeof(winFollowScoreProb_));
    memset(winGiveUpProb_, 0, sizeof(winGiveUpProb_));
    m_MinAddScore=0;

    srand((unsigned)time(NULL));
    readIni();
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{

}

void CAndroidUserItemSink::readIni()
{
    if(!boost::filesystem::exists("./conf/gswz_robot_config.ini"))
    {// init robot param
        abort();
    }
    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/gswz_robot_config.ini",pt);

    winAllInProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.AllIn",0.15);
    winAddScoreProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.AddScore",0.45);
    winPassScoreProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.PassScore",0.55);
    winFollowScoreProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.FollowScore",0.95);
    winGiveUpProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.GiveUp",0.5);
    winReAddSoreProb_[Stock_Ceil]=pt.get<float>("Win_Stock_Ceil_Prob.ReAddScore",0.5);


    winAllInProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.AllIn",0.15);
    winAddScoreProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.AddScore",0.45);
    winPassScoreProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.PassScore",0.55);
    winFollowScoreProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.FollowScore",0.95);
    winGiveUpProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.GiveUp",0.5);
    winReAddSoreProb_[Stock_Normal]=pt.get<float>("Win_Stock_Normal_Prob.ReAddScore",0.5);


    winAllInProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.AllIn",0.15);
    winAddScoreProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.AddScore",0.45);
    winPassScoreProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.PassScore",0.55);
    winFollowScoreProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.FollowScore",0.95);
    winGiveUpProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.GiveUp",0.5);
    winReAddSoreProb_[Stock_Floor]=pt.get<float>("Win_Stock_Floor_Prob.ReAddScore",0.5);


    lostAllInProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.AllIn",0.15);
    lostAddScoreProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.AddScore",0.45);
    lostPassScoreProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.PassScore",0.55);
    lostFollowScoreProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.FollowScore",0.95);
    lostGiveUpProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.GiveUp",0.5);
    lostReAddSoreProb_[Stock_Ceil]=pt.get<float>("Lost_Stock_Ceil_Prob.ReAddScore",0.5);

    lostAllInProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Normal_Prob.AllIn",0.15);
    lostAddScoreProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Normal_Prob.AddScore",0.45);
    lostPassScoreProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Normal_Prob.PassScore",0.55);
    lostFollowScoreProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Normal_Prob.FollowScore",0.95);
    lostGiveUpProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Normal_Prob.GiveUp",0.5);
    lostReAddSoreProb_[Stock_Normal]=pt.get<float>("Lost_Stock_Ceil_Prob.ReAddScore",0.5);

    lostAllInProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Floor_Prob.AllIn",0.15);
    lostAddScoreProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Floor_Prob.AddScore",0.45);
    lostPassScoreProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Floor_Prob.PassScore",0.55);
    lostFollowScoreProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Floor_Prob.FollowScore",0.95);
    lostGiveUpProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Floor_Prob.GiveUp",0.5);
    lostReAddSoreProb_[Stock_Floor]=pt.get<float>("Lost_Stock_Ceil_Prob.ReAddScore",0.5);

    m_cbJettonScroe[0]=pt.get<int>("Add_Score.Score_1",1);
    m_cbJettonScroe[1]=pt.get<int>("Add_Score.Score_2",2);
    m_cbJettonScroe[2]=pt.get<int>("Add_Score.Score_3",4);
    m_cbJettonScroe[3]=pt.get<int>("Add_Score.Score_4",8);

    OperateTimeProb_[0]=pt.get<int>("OperateTime.P20OperateTime",50);
    OperateTimeProb_[1]=pt.get<int>("OperateTime.P40OperateTime",20);
    OperateTimeProb_[2]=pt.get<int>("OperateTime.P60OperateTime",10);
    OperateTimeProb_[3]=pt.get<int>("OperateTime.P80OperateTime",5);
    OperateTimeProb_[4]=pt.get<int>("OperateTime.P100OperateTime",1);

}

bool CAndroidUserItemSink::RepositionSink()
{

    m_lStockScore = 0;
    m_wCurrentUser = INVALID_CHAIR;
    m_wChairID = INVALID_CHAIR;
    m_cbCardType = 0;

    m_lMaxCellScore = 0;
    m_lUserMaxScore = 0;
    m_lMinJettonScore = 0;
    m_cbHandCardCount=0;

    memset(m_lTableScore, 0, sizeof(m_lTableScore));
    //游戏状态
    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
    memset(m_cbRealPlayer, 0, sizeof(m_cbRealPlayer));
    memset(m_cbAndroidStatus, 0, sizeof(m_cbAndroidStatus));

    //用户扑克
    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbAllHandCardData, 0, sizeof(m_cbAllHandCardData));
    memset(m_cbHandCardType, 0, sizeof(m_cbHandCardType));
    return true;
}

bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pAndroidUserItem)
{
    LOG(ERROR) << "CAndroidUserItemSink::Initialization, pTableFrame:" << pTableFrame
               << ", pAndroidUserItem:" << pAndroidUserItem;

    // check if the input parameter content now.
    if ((NULL == pTableFrame) ||
            (NULL == pAndroidUserItem))
    {
        return false;
    }

    m_pTableFrame = pTableFrame;
    m_TimerLoopThread = m_pTableFrame->GetLoopThread();
    m_wChairID    = pAndroidUserItem->GetChairId();
    m_lUserId     = pAndroidUserItem->GetUserId();
    m_pAndroidUserItem = pAndroidUserItem;
    m_lCellScore     = m_pTableFrame->GetGameRoomInfo()->floorScore;

}


void CAndroidUserItemSink::OnTimerAddScore()
{
    if (INVALID_CARD == m_wChairID) {   // ad by James.
        m_wChairID = m_pAndroidUserItem->GetChairId();
    }
    HandleAndroidAddScore();
}

void CAndroidUserItemSink::OnTimerLookCard()
{
    m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_LOOK_CARD, NULL, 0);
}

bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t*  pData, uint32_t wDataSize)
{
    LOG_ERROR << "OnGameMessage wSubCmdID:" << wSubCmdID;
    if (INVALID_CARD == m_wChairID) {   // ad by James.
        m_wChairID = m_pAndroidUserItem->GetChairId();
    }
    switch (wSubCmdID)
    {
    case SUB_S_GAME_START:		//游戏开始
    {
        LOG_ERROR << "CAndroidUserItemSink::OnGameMessage, SUB_S_GAME_START. status:" << m_pAndroidUserItem->GetUserStatus();

        return OnSubGameStart(pData, wDataSize);
    }
    case SUB_S_ADD_SCORE:		//用户下注
    {
        return OnSubAddScore(pData, wDataSize);
    }
    case SUB_S_GIVE_UP:			//用户放弃
    {
        return OnSubGiveUp(pData, wDataSize);
    }
    case SUB_S_GAME_END:		//游戏结束
    {
        return OnSubGameEnd(pData, wDataSize);
    }
    case SUB_S_FOLLOW_SCORE:	//用户跟注
    {
        return OnSubFollowScore(pData, wDataSize);
    }
    case SUB_S_PASS_SCORE:		//用户不加
    {
        return OnSubPassScore(pData, wDataSize);
    }
    case SUB_S_SEND_CARD:		//发牌
    {
        return OnSubSendCard(pData, wDataSize);
    }
    case SUB_S_ALL_IN:
    {
        return OnSubAllIn(pData, wDataSize);
    }
    case SUB_S_ANDROID_CARD:	//机器人牌消息
    {
        return OnSubAndroidCard(pData, wDataSize);
    }
    default:
        break;
    }
    return true;
}

// 机器人消息
bool CAndroidUserItemSink::OnSubAndroidCard(const void * pBuffer, int32_t wDataSize)
{

    m_nRandGenZhuNum = GlobalFunc::RandomInt64(10, 20);
    m_nCurGenZhu = 0;

    CMD_S_AndroidCard pAndroidCard;
    if (!pAndroidCard.ParseFromArray(pBuffer,wDataSize))
    {
        return false;
    }

    m_lStockScore = pAndroidCard.lstockscore();
    const ::google::protobuf::RepeatedPtrField< ::Gswz::AndroidPlayer >& players = pAndroidCard.players();

    for (int i=0;i<GAME_PLAYER;i++)
    {
        m_cbRealPlayer[i] =0;
        m_cbAndroidStatus[i] = 0;
        m_cbUserIds[i] = 0;

    }
    m_canViewCard.clear();
    for(int index = 0;index<players.size(); ++ index)
    {
        int chairId = players[index].cbchairid();
        m_cbRealPlayer[chairId]      = players[index].cbrealplayer()?1:0;
        m_cbAndroidStatus[chairId]   = players[index].cbrealplayer()?0:1;
        m_cbUserIds[chairId] = players[index].cbuserid();
        m_cbPlayStatus[chairId]=1;
        for (int j=0;j<MAX_COUNT;j++)
        {
            m_cbAllHandCardData[chairId][j] = players[index].cballhandcarddata()[j];
        }
        if(chairId == m_wChairID)
        {
            m_canViewCard.push_back(m_cbAllHandCardData[chairId][0]);
            m_canViewCard.push_back(m_cbAllHandCardData[chairId][1]);
        }else
            m_canViewCard.push_back(m_cbAllHandCardData[chairId][1]);
    }


    //计算所有的玩家的牌型
    for (int32_t i = 0; i < GAME_PLAYER; ++i)
    {
        if (m_cbPlayStatus[i] == 0)
            continue;
        //服务器下发的牌型已经排序，不需要再进行排序
        m_cbHandCardType[i] = m_GameLogic.GetCardType(m_cbAllHandCardData[i], MAX_COUNT);
    }

    m_currentMaxCardUser=GetMaxHandCardUser();
    return true;
}

//游戏开始
bool CAndroidUserItemSink::OnSubGameStart(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_GameStart pGameStart;
    if (!pGameStart.ParseFromArray(pBuffer,wDataSize))
    {
        return false;
    }


    m_wCurrentUser = pGameStart.nextstep().wnextuser();
    m_MinAddScore=pGameStart.nextstep().wminaddscore();

    m_lUserMaxScore = 0;
    m_lMinJettonScore = 0;
    int64_t cellsore =  m_pTableFrame->GetGameRoomInfo()->floorScore ;
    m_lCellScore = cellsore;

    m_lMaxCellScore =  m_pTableFrame->GetGameRoomInfo()->jettons[1];
    m_wChairID = m_pAndroidUserItem->GetChairId();

    auto playerItems=pGameStart.gameplayers();
    for (int i=0;i<playerItems.size();i++)
    {
        m_cbPlayStatus[playerItems[i].cbchairid()] = playerItems[playerItems[i].cbchairid()].cbplaystatus();
    }
    cout<<"------>OnSubGameStart==========================="<<endl;


    for (int32_t i = 0; i < GAME_PLAYER; i++)
    {

        if (m_cbPlayStatus[i] == 1)
        {
            m_lTableScore[i] =cellsore;
            m_cbHandCardCount=2;
        }
    }
    m_OperateTimes= pGameStart.nextstep().wtimeleft();

    if (m_pAndroidUserItem->GetUserId() == m_wCurrentUser)
    {
        cout<<"------>OnGameStart";
        SC_JH_WAIT_OPT();
    }else
    {
        int ntimer=rand()%5;
        m_timerLookCard= m_TimerLoopThread->getLoop()->runAfter(ntimer, boost::bind(&CAndroidUserItemSink::OnTimerLookCard, this));
    }

    return true;
}

//用户放弃
bool CAndroidUserItemSink::OnSubGiveUp(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_GiveUp pGiveUp;
    if (!pGiveUp.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    int64_t giveupuserid = pGiveUp.wgiveupuser();
    int32_t pGiveupChairId = GetChairIdByUserId( giveupuserid);
    if(pGiveupChairId == INVALID_CHAIR)
    {
        return false;
    }

    m_cbRealPlayer[pGiveupChairId] = 0;
    m_cbPlayStatus[pGiveupChairId] = 0;
    cout<<"------>OnSubGiveUp"<<endl;
    if(pGiveUp.bendgame()) return true;
    //放弃的是上一把的玩家 如果不是有问题
    if(pGiveUp.wgiveupuser() != m_wCurrentUser) return true;
    m_lMinJettonScore=pGiveUp.nextstep().wneedjettonscore();
    m_wCurrentUser =  pGiveUp.nextstep().wnextuser();
    m_OperateTimes =pGiveUp.nextstep().wtimeleft();
    m_MinAddScore=pGiveUp.nextstep().wminaddscore();


    //update max card user
    m_currentMaxCardUser=GetMaxHandCardUser();

    if(m_wCurrentUser == m_lUserId)
    {
        SC_JH_WAIT_OPT();
    }

    return true;
}

//用户下注
bool CAndroidUserItemSink::OnSubAddScore(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_AddScore pAddScore;
    if (!pAddScore.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    int32_t opuserChairId =  GetChairIdByUserId(pAddScore.wopuserid());
    if(opuserChairId== INVALID_CHAIR)
    {
        return false;
    }
    if(m_cbAndroidStatus[opuserChairId])
        m_lStockScore-=pAddScore.wjetteonscore();
    //need min score
    int64_t llCurrentTimes = pAddScore.nextstep().wneedjettonscore();
    m_MinAddScore=pAddScore.nextstep().wminaddscore();

    m_lMinJettonScore   = llCurrentTimes;
    m_wCurrentUser    = pAddScore.nextstep().wnextuser();

    m_lTableScore[opuserChairId] += (pAddScore.wjetteonscore());
    m_OperateTimes =pAddScore.nextstep().wtimeleft();

    if (m_lUserId == pAddScore.nextstep().wnextuser())
    {
        cout<<"------>OnSubAddScore";
        SC_JH_WAIT_OPT();
    }

    return true;
}

//用户不加
bool CAndroidUserItemSink::OnSubPassScore(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_PassScore pPassScore;
    if (!pPassScore.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }
    m_wCurrentUser=pPassScore.nextstep().wnextuser();
    m_lMinJettonScore=pPassScore.nextstep().wneedjettonscore();
    m_OperateTimes =pPassScore.nextstep().wtimeleft();
    m_MinAddScore=pPassScore.nextstep().wminaddscore();

    if (m_wCurrentUser == m_lUserId)
    {
        cout<<"------>OnSubPassScore";
        SC_JH_WAIT_OPT();
    }

    return true;
}

bool CAndroidUserItemSink::OnSubSendCard(const void *pBuffer, int32_t wDataSize)
{
    CMD_S_SendCard pSendCard;
    if (!pSendCard.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }
    auto &cardItems=pSendCard.carditems();
    for(int i=0 ; i<pSendCard.carditems_size() ;i++ )
    {
        m_canViewCard.push_back(cardItems[i].cbcard());
    }

    m_cbHandCardCount+=1;
    m_wCurrentUser = pSendCard.nextstep().wnextuser();
    m_lMinJettonScore=pSendCard.nextstep().wneedjettonscore();
    m_OperateTimes =pSendCard.nextstep().wtimeleft();
    m_MinAddScore=pSendCard.nextstep().wminaddscore();
    cout<<"------>OnSubSendCard";
    if (m_lUserId == m_wCurrentUser)
    {
        SC_JH_WAIT_OPT();
    }
    return true;
}

//用户跟注
bool CAndroidUserItemSink::OnSubFollowScore(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_FollowScore pFollowScore;
    if (!pFollowScore.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    m_wCurrentUser = pFollowScore.nextstep().wnextuser();
    m_lMinJettonScore=pFollowScore.nextstep().wneedjettonscore();
    m_OperateTimes=pFollowScore.nextstep().wtimeleft();
    m_MinAddScore=pFollowScore.nextstep().wminaddscore();
    uint8_t followChairId=GetChairIdByUserId(pFollowScore.wfollowuser());

    m_lTableScore[followChairId]+=pFollowScore.wfollowscore();
    if(m_cbAndroidStatus[followChairId])
        m_lStockScore-=pFollowScore.wfollowscore();

    if (m_lUserId == m_wCurrentUser)
    {
        cout<<"------>OnSubFollowScore";
        SC_JH_WAIT_OPT();
    }
    return true;
}

bool CAndroidUserItemSink::OnSubAllIn(const void *pBuffer, int32_t wDataSize)
{
    CMD_S_AllIn pAllIn;
    if (!pAllIn.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    m_wCurrentUser = pAllIn.nextstep().wnextuser();
    uint8_t allinChairId=GetChairIdByUserId(pAllIn.wallinuser());
    m_lMinJettonScore=pAllIn.nextstep().wneedjettonscore();
    m_OperateTimes=pAllIn.nextstep().wtimeleft();
    m_MinAddScore=pAllIn.nextstep().wminaddscore();

    m_lTableScore[allinChairId]+=pAllIn.dallinscore();
    if(m_cbAndroidStatus[allinChairId])
        m_lStockScore-=pAllIn.dallinscore();

    if (m_lUserId == m_wCurrentUser)
    {
        cout<<"------>OnSubAllIn";
        SC_JH_WAIT_OPT();
    }
    return true;
}


//游戏结束
bool CAndroidUserItemSink::OnSubGameEnd(const void * pBuffer, int32_t wDataSize)
{
    CMD_S_GameEnd pGameEnd;
    if (!pGameEnd.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    m_lMaxCellScore = 0;

    for(int index=0;index<GAME_PLAYER;index++)
    {
        m_cbUserIds[index]=0;
    }

    cout<<"------>OnSubGameEnd======================================"<<endl;
    m_lMinJettonScore = 0;
    memset(m_cbHandCardData, 0, sizeof(m_cbHandCardData));
    memset(m_cbAllHandCardData, 0, sizeof(m_cbAllHandCardData));
    memset(m_cbHandCardType, 0, sizeof(m_cbHandCardType));

    return true;
}

void CAndroidUserItemSink::GetLeftBufCards(vector<uint8_t> &cards)
{
    cards.resize(MAX_CARD_TOTAL-m_canViewCard.size());
    vector<uint8_t> CardList(MAX_CARD_TOTAL);
    memcpy(&CardList[0],&m_GameLogic.m_cbCardListData[0],MAX_CARD_TOTAL);
    auto comp=[&](uint8_t a ,uint8_t b){return a < b ;};
    sort(m_canViewCard.begin(),m_canViewCard.end(),comp);
    sort(CardList.begin(),CardList.end(),comp);

    set_difference(CardList.begin(),CardList.end(),
            m_canViewCard.begin(),m_canViewCard.end(),cards.begin());
//    for(auto card:cards)
//    {
//        printf("card = %d \n",card);
//    }
}

//得到手上最大预测牌
void CAndroidUserItemSink::GetMaxCards(vector<uint8_t> &buffcards, uint8_t cards[], uint8_t chairId)
{
    assert(buffcards.size()>2);
    uint8_t tmpCard[5]={0,0,0,0,0};
    uint8_t handCardCount=m_cbHandCardCount;

    if(chairId==m_wChairID)
    {
        memcpy(&tmpCard[0],&m_cbAllHandCardData[chairId][0],handCardCount);
        if(handCardCount==5)
        {
            memcpy(&cards[0],&tmpCard[0],5);
            return ;
        }
    }
    else
    {

        handCardCount=m_cbHandCardCount-1;
        memcpy(&tmpCard[0],&m_cbAllHandCardData[chairId][1],handCardCount);

        memcpy(&cards[0],&tmpCard[0],5);
        return ;//别的玩家只要判断现有的牌
    }

    uint8_t cardCount=buffcards.size();
    uint8_t GROUP_COUNT=MAX_COUNT-handCardCount;
    vector<uint8_t> index;
    index.resize(GROUP_COUNT);
    for(int i=0;i<GROUP_COUNT;i++)
    {
        index[i]=i;
        tmpCard[handCardCount+i]=buffcards[i];
    }
    uint8_t maxType=0;
    bool is_continue=true;
    do
    {
        uint8_t type=m_GameLogic.GetCardType(tmpCard,5);
        if(maxType<type)
        {
            maxType=type;
            memcpy(&cards[0],&tmpCard[0],5);
        }
        else if(maxType==type)
        {
            if(m_GameLogic.CompareCard(&tmpCard[0],&cards[0],5))
            {
                memcpy(&cards[0],&tmpCard[0],5);
            }
        }

        if (index[GROUP_COUNT-1]==cardCount-1)
        {
            is_continue=false;
            for(int i=GROUP_COUNT-2;i>=0;i--)
            {
                if (index[i]+1 !=index[i+1] )
                {
                    index[i]++;
                    for(int j=i+1;j<GROUP_COUNT;j++)
                    {
                        index[j]=index[j-1]+1;
                    }
                    is_continue=true;
                    break;
                }
            }
        }else
        {
            index[GROUP_COUNT-1]++ ;
        }
        for(int i=0;i<GROUP_COUNT;i++)
        {
            tmpCard[handCardCount+i]=buffcards[index[i]];
        }
    }while(is_continue);
}

bool CAndroidUserItemSink::CalViewCardsMayWin()
{
    vector<uint8_t> buffCard;
    GetLeftBufCards(buffCard);
    uint8_t myCards[5]={0,0,0,0,0};
    uint8_t otherCards[5]={0,0,0,0,0};
    GetMaxCards(buffCard,myCards,m_wChairID);
    cout<<"card:";
    for(auto& card:myCards)
    {
        cout<<card<<",";
    }
    cout<<endl;
    int32_t wWinUser=INVALID_CHAIR;
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if(!m_cbPlayStatus[i]||i==m_wChairID)
            continue;

        GetMaxCards(buffCard,otherCards,i);
        //对比扑克
        if (m_GameLogic.CompareCard(myCards, otherCards, MAX_COUNT) != 1)
        {
            return false;
        }
    }

    return true;
}

bool CAndroidUserItemSink::HandleAndroidAddScore()
{
    if (m_lUserId != m_wCurrentUser)
        return false;

    switch (m_DoOpt) {
    case OP_FOLLOW:				//跟注
        m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_FOLLOW_SCORE, NULL, 0);
        break;
    case OP_PASS:				//不加
        m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_PASS_SCORE, NULL, 0);
        break;
    case OP_ALLIN:				//梭哈:
        m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_ALL_IN, NULL, 0);
        break;
    case OP_ADD:                 //加注:
        JiaZhu();
        break;
    case OP_GIVE_UP:			 //弃牌:
        m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_GIVE_UP, NULL, 0);
        break;
    default:
        break;
    }
    std::cout<<"机器人操作:"<<m_DoOpt;
    return true;
}

void CAndroidUserItemSink::JiaZhu()
{
    //int64_t pos = index==0? (cannotaddscore-1) : GlobalFunc::RandomInt64(cannotaddscore,index + cannotaddscore-1);
    int JettonIndex=rand()%3;
    int jettonScore=m_MinAddScore+(m_cbJettonScroe[JettonIndex]-1)*m_lCellScore;

    int64_t lAndroidMaxScore = 0;
    shared_ptr<CServerUserItem> pIServerUserItem = m_pTableFrame->GetTableUserItem(m_wChairID);
    if (pIServerUserItem != NULL) {
        lAndroidMaxScore = pIServerUserItem->GetUserScore();
        //sub add score
        lAndroidMaxScore = lAndroidMaxScore -  m_lTableScore[m_wChairID];
    }
    if(lAndroidMaxScore<jettonScore)
    {
        m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_GIVE_UP, NULL, 0);
        return;
    }
    cout<<"======AddScore"<<jettonScore<<endl;
    CMD_C_AddScore AddScore;
    AddScore.set_dscore(jettonScore);
    string content = AddScore.SerializeAsString();
    m_pTableFrame->OnGameEvent(m_wChairID, SUB_C_ADD_SCORE, (uint8_t*)content.data(), content.size());
}

// clear all the timer item data value now.
bool CAndroidUserItemSink::ClearAllTimer()
{	
    //    m_timerAddScore.stop();
    m_TimerLoopThread->getLoop()->cancel(m_timerAddScore);
    m_TimerLoopThread->getLoop()->cancel(m_timerLookCard);
    return true;
}

int32_t CAndroidUserItemSink::GetChairIdByUserId(int64_t userid)
{
    for(int index = 0;index<GAME_PLAYER;index++)
    {
        if(m_cbUserIds[index] == userid)
        {
            return index;
        }
    }
    return INVALID_CHAIR;
}

int32_t CAndroidUserItemSink::GetUserIdByChiarId(int32_t chairId)
{
    return m_cbUserIds[chairId];
}

int64_t CAndroidUserItemSink::GetCurrentUserScore()
{
    int64_t lAndroidMaxScore = 0;
    shared_ptr<CServerUserItem> pIServerUserItem = m_pTableFrame->GetTableUserItem(m_wChairID);
    if (pIServerUserItem != NULL) {
        lAndroidMaxScore = pIServerUserItem->GetUserScore();
        lAndroidMaxScore = lAndroidMaxScore -  m_lTableScore[m_wChairID];
    }

    //最大下注
    return lAndroidMaxScore;
}

int64_t CAndroidUserItemSink::GetMaxHandCardUser()
{
    int32_t wWinUser=INVALID_CHAIR;
    for(int i = 0; i < GAME_PLAYER; i++)
    {
        //用户过滤
        if(!m_cbPlayStatus[i])
            continue;

        if (wWinUser == INVALID_CHAIR)
        {
            wWinUser = i;
            continue;
        }

        //对比扑克
        if (m_GameLogic.CompareCard(m_cbAllHandCardData[i], m_cbAllHandCardData[wWinUser], MAX_COUNT) >= 1)
        {
            wWinUser = i;
        }
    }
    if (wWinUser == INVALID_CHAIR)
        return 0;

    return GetUserIdByChiarId(wWinUser);
}

void  CAndroidUserItemSink::SC_JH_WAIT_OPT()
{
   m_DoOpt = GetWaitOperate();
   int allProb= 0 ,weight = 0;
   for(int i = 0 ; i < 5 ; i++ ) allProb+=OperateTimeProb_[i];

   int iRand  =rand()%allProb ;
   float ftime=m_OperateTimes*0.2;
   for(int i = 0 ; i < 5 ; i++ )
   {
        weight+=OperateTimeProb_[i];
        if(iRand<weight)
        {
            ftime=ftime*i+ ftime*(rand()%100)*0.01+1;
            break;
        }
   }
   cout << "OperateTime :"<<ftime<<endl;

   m_timerAddScore=m_TimerLoopThread->getLoop()->runAfter(ftime, boost::bind(&CAndroidUserItemSink::OnTimerAddScore, this));
   if((int)ftime%2==0)
   {
       ftime=rand()%5;
       m_timerLookCard= m_TimerLoopThread->getLoop()->runAfter(ftime, boost::bind(&CAndroidUserItemSink::OnTimerLookCard, this));
   }
}

Wait_Operate CAndroidUserItemSink::GetWaitOperate()
{
    Wait_Operate Operate;

    float index=(float)(rand()%100)/100;
    //在库存以下
    //if current user is the first operate user than AddScore else FollowScore
    int stockStatus=0;
    cout<<"-MinJettonScore"<<m_lMinJettonScore;
    cout<<"-m_lStockScore:"<<m_lStockScore<<"-m_llTodayHighLimit:"<<m_llTodayHighLimit<<"-index:"<<index;
    if(m_lStockScore>m_llTodayHighLimit)
    {
        stockStatus=Stock_Ceil;
    }
    else if (m_lStockScore-m_lMinJettonScore<1)
    {
        stockStatus=Stock_Floor;
    }
    else
    {
        stockStatus=Stock_Normal;
    }
    cout<<"-stockStatus:"<<stockStatus;
    if(m_currentMaxCardUser==m_lUserId)
    {
         cout<<"-win";
        if(m_lMinJettonScore==0)//先手
        {
            cout<<"-first";
            if(m_cbHandCardCount>3&&((float)(rand()%100)/100)<winAllInProb_[stockStatus])
            {
                Operate=OP_ALLIN;
                return Operate;
            }

            if(index<winAddScoreProb_[stockStatus])
                Operate=OP_ADD;
            else if(index<winPassScoreProb_[stockStatus]+winAddScoreProb_[stockStatus])
                Operate=OP_PASS;
            else //先手不弃牌，优先过牌
                Operate=OP_PASS;

        }else if(m_lMinJettonScore>0)
        {
            if(m_cbHandCardCount>3&&((float)(rand()%100)/100)<winAllInProb_[stockStatus])
            {
                Operate=OP_ALLIN;
                return Operate;
            }


            if(index<winReAddSoreProb_[stockStatus])
            {
                if(m_MinAddScore!=0)
                    Operate=OP_ADD;
                else
                    Operate=GetCurrentUserScore()==m_lMinJettonScore?OP_ALLIN:OP_FOLLOW;
            }
            else if(index<winReAddSoreProb_[stockStatus]+winFollowScoreProb_[stockStatus])
                Operate=GetCurrentUserScore()==m_lMinJettonScore?OP_ALLIN:OP_FOLLOW;
            else
                Operate=OP_GIVE_UP;
        }
    }else
    {
        if(m_cbHandCardCount>3 &&! CalViewCardsMayWin())
        {
            cout<<"----------->cannot win !!!!!"<<endl;
            Operate=OP_GIVE_UP;
            return Operate;
        }

        cout<<"-lost";
        if(m_lMinJettonScore==0)//先手
        {
            cout<<"-first";
            if(m_cbHandCardCount>3&&((float)(rand()%100)/100)<lostAllInProb_[stockStatus])
            {
                Operate=OP_ALLIN;
                return Operate;
            }

            if(index<lostAddScoreProb_[stockStatus])
                Operate=OP_ADD;
            else if(index<lostAddScoreProb_[stockStatus]+lostPassScoreProb_[stockStatus])
                Operate=OP_PASS;
            else //先手不弃牌，优先过牌
                Operate=OP_PASS;


        }else if(m_lMinJettonScore>0)
        {
            if(m_cbHandCardCount>3&&((float)(rand()%100)/100)<lostAllInProb_[stockStatus])
            {
                Operate=OP_ALLIN;
                return Operate;
            }

            if(m_cbHandCardCount>3&&((float)(rand()%100)/100)<lostReAddSoreProb_[stockStatus])
            {
                if(m_MinAddScore!=0)
                    Operate=GetCurrentUserScore()==m_lMinJettonScore?OP_ALLIN:OP_ADD;
                else
                    Operate=GetCurrentUserScore()==m_lMinJettonScore?OP_ALLIN:OP_FOLLOW;
                return Operate;
            }

            if(index<(lostFollowScoreProb_[stockStatus]/(m_lMinJettonScore/m_lCellScore)))
                Operate=GetCurrentUserScore()==m_lMinJettonScore?OP_ALLIN:OP_FOLLOW;
            else
                Operate=OP_GIVE_UP;
        }
    }
    cout<<"-Operate:"<<Operate<<endl;
    return Operate;
}



void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    if(pTableFrame)
    {
        m_pTableFrame = pTableFrame;
        m_TimerLoopThread = m_pTableFrame->GetLoopThread();
        m_lCellScore     = m_pTableFrame->GetGameRoomInfo()->floorScore;

        // initialize the storage value sitem now.
        m_llTodayHighLimit = m_pTableFrame->GetGameRoomInfo()->totalStockHighLimit-m_pTableFrame->GetGameRoomInfo()->totalStockLowerLimit;//系统赢了不得大于库存上限，否则系统吐出来
    }
}


bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
    if ( NULL == pUserItem)
    {
        return false;
    }

    m_wChairID    = pUserItem->GetChairId();
    m_lUserId     = pUserItem->GetUserId();
    m_pAndroidUserItem = pUserItem;

}

//CreateAndroidSink 创建实例
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

//DeleteAndroidSink 删除实例
extern "C" void DeleteAndroidSink(shared_ptr<IAndroidUserItemSink>& pSink)
{
    if(pSink)
    {
        pSink->RepositionSink();
    }
    pSink.reset();
}

