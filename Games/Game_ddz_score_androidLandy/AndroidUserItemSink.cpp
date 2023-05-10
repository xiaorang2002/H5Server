#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <math.h>

#include "public/StdRandom.h"
#include "../../proto/ddz.Message.pb.h"
#include "../Game_ddzLandy/CMD_Game.h"
#include "AndroidUserItemSink.h"

using namespace  ddz;
//////////////////////////////////////////////////////////////////////////

//辅助时间
#define TIME_LESS					3									//最少时间
#define TIME_LESSNew				2									//最少时间
#define TIME_DISPATCH				5									//发牌时间

//游戏时间
//#define TIME_OUT_CARD				8									//出牌时间
#define TIME_OUT_CARD				3									//出牌时间
#define TIME_START_GAME				8									//开始时间
#define TIME_CALL_SCORE				4									//叫分时间
#define TIME_STAKE_SCORE			4									//加注时间
#define TIME_READY			        5									//加注时间

//游戏时间
#define IDI_OUT_CARD				(0)			//出牌时间
#define IDI_START_GAME				(1)			//开始时间
#define IDI_CALL_SCORE				(2)			//叫分时间
#define IDI_STAKE_SCORE				(3)		    //加倍时间
#define IDI_Ready                   (4)		    //ready时间

#define TIMEXS          1000

//////////////////////////////////////////////////////////////////////////

//组件创建函数
// DECLARE_CREATE_MODULE(AndroidUserItemSink);
extern "C" shared_ptr<IAndroidUserItemSink> CreateAndroidSink()
{
    shared_ptr<CAndroidUserItemSink> pAndroidSink(new CAndroidUserItemSink());
    shared_ptr<IAndroidUserItemSink> pIAndroidSink = dynamic_pointer_cast<IAndroidUserItemSink>(pAndroidSink);
    return pIAndroidSink;
}

// reset all the data now.
extern "C" void DeleteAndroidSink(IAndroidUserItemSink* pSink)
{
    if (pSink) {
        pSink->RepositionSink();
    }
    //Cleanup:
    delete pSink;
    pSink = NULL;
}


#define AI

//构造函数
CAndroidUserItemSink::CAndroidUserItemSink()
{
    //游戏变量
    m_wBankerUser=INVALID_CHAIR;
    m_cbCurrentLandScore = 255 ;				//已叫分数
    m_wOutCardUser = INVALID_CHAIR ;

    //扑克变量
    m_cbTurnCardCount=0;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

    //手上扑克
    ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
    ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));

    //接口变量
    m_pIAndroidUserItem=NULL;;

	randomRound = 0;
	CountrandomRound = 0;

    return;
}

//析构函数
CAndroidUserItemSink::~CAndroidUserItemSink()
{
}

/*
//接口查询
VOID * CAndroidUserItemSink::QueryInterface(REFGUID Guid, DWORD dwQueryVer)
{
    QUERYINTERFACE(IAndroidUserItemSink,Guid,dwQueryVer);
    QUERYINTERFACE_IUNKNOWNEX(IAndroidUserItemSink,Guid,dwQueryVer);
    return NULL;
}
*/
void CAndroidUserItemSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{
    if ((!pTableFrame)) {
        //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
        return;
    }
    m_pITableFrame   =  pTableFrame;

    m_pGameRoomKind = m_pITableFrame->GetGameRoomInfo();

	randomRound = 1;//rand() % 5 + 2;
	CountrandomRound = 0;
//    m_TimerLoopThread=m_TimerLoopThread->getLoop();
    m_TimerLoopThread = m_pITableFrame->GetLoopThread();
}

bool CAndroidUserItemSink::SetUserItem(shared_ptr<IServerUserItem>& pUserItem)
{
    if ((!pUserItem)) {
        //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
        return false;
    }

    m_pIAndroidUserItem = pUserItem;

}
//初始接口
bool CAndroidUserItemSink::Initialization(shared_ptr<ITableFrame>& pTableFrame, shared_ptr<IServerUserItem>& pUserItem)
{
    //查询接口
    bool bSuccess = false;
    do
    {

        if ((!pTableFrame) || (!pUserItem)) {
            //LOG(INFO) << "pTable frame or pUserItem is NULL!, pTableFrame:" << pTableFrame << ", pUserItem:" << pUserItem;
            break;
        }

        //查询接口
        // m_pIAndroidUserItem=QUERY_OBJECT_PTR_INTERFACE(pIUnknownEx,IAndroidUserItem);
        // if (m_pIAndroidUserItem==NULL) return false;
        m_pITableFrame   =  pTableFrame;
        m_pIAndroidUserItem = pUserItem;

        // try to update the specdial room config data item.
        m_pGameRoomKind = m_pITableFrame->GetGameRoomInfo();
        /*
        if (g_pLog == NULL)
        {
            g_pLog = GetNewLog(TEXT(".\\log\\Ox5Android\\"), TEXT("log"));
        } */

    }   while (0);

    return true;
}

//重置接口
bool CAndroidUserItemSink::RepositionSink()
{
    //游戏变量
    m_wBankerUser=INVALID_CHAIR;

    //扑克变量
    m_cbTurnCardCount=0;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

    //手上扑克
    ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
    ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));

    return true;
}

void CAndroidUserItemSink::OnTimerOnTimerIDI_START_GAME()
{
    //m_TimerSTARTGAME.stop();

    m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerStart);
    //开始判断
    //m_pIAndroidUserItem->setUserReady();

    openLog("IDI_START_GAME || IDI_Ready AIState = %d ",m_pIAndroidUserItem->GetUserStatus());
}
void CAndroidUserItemSink::OnTimerIDI_CALL_SCORE()
{
    //m_TimerCALLSCORE.stop();

    m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerCall);
    //openLog("IDI_CALL_SCORE");
    //构造变量
    uint8_t			cbCallScore;

    //设置变量
    cbCallScore = 255;
    if(m_cbCurrentLandScore == 4 )
        m_cbCurrentLandScore= 0;

    uint8_t cbCallScoreTemp = m_GameLogic.LandScore(m_pIAndroidUserItem->GetChairId(), m_cbCurrentLandScore);

    if(cbCallScoreTemp<0 || cbCallScoreTemp>3)
        cbCallScoreTemp = 0;


    if(cbCallScoreTemp>m_cbCurrentLandScore)
    {
        cbCallScore = cbCallScoreTemp;
    }
    else
        cbCallScore = 0;


    //openLog("111111 cbCallScore=%d",static_cast<int>(cbCallScore));
    ddz::CMD_C_CallBanker pCallScore;
    pCallScore.set_cbcallinfo(cbCallScore);
    std::string data = pCallScore.SerializeAsString();
    m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),ddz::SUB_C_CALL_BANKER,(uint8_t *)data.c_str(),data.size());

}
void CAndroidUserItemSink::OnTimerIDI_STAKE_SCORE()
{
    //m_TimerSTAKESCORE.stop();
    m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerStake);

    //openLog("IDI_STAKE_SCORE 1");
    if (m_pIAndroidUserItem->GetChairId() != m_wBankerUser)
    {
        uint8_t							cbOpId;

        //openLog("IDI_STAKE_SCORE 2");
        cbOpId = m_GameLogic.DoubleScore(m_pIAndroidUserItem->GetChairId(), 0) >=2 ? CB_ADD_DOUBLE : CB_NO_ADD_DOUBLE;


        CMD_C_Double  wDouble;
        wDouble.set_cbdoubleinfo(cbOpId);
        std::string data = wDouble.SerializeAsString();

        //发送数据
        m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),ddz::SUB_C_ADD_DOUBLE, (uint8_t *)data.c_str(),data.size());
    }
}
//机器人出牌分析就在这里了
void CAndroidUserItemSink::OnTimerIDI_OUT_CARD()
{
    //m_TimerOUTCARD.stop();
    m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);

    //openLog("IDI_OUT_CARD 1");
    //扑克分析
    tagOutCardResult OutCardResult;
    memset(&OutCardResult,0,sizeof(OutCardResult));
    try
    {
        WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
        openLog("wMeChairID=%d",wMeChairID);
		if (wMeChairID >= 0 && wMeChairID < GAME_PLAYER)
		{
			m_GameLogic.SearchOutCard(m_cbHandCardData, m_cbHandCardCount[wMeChairID], m_cbTurnCardData, m_cbTurnCardCount, m_wOutCardUser, m_pIAndroidUserItem->GetChairId(), OutCardResult);
		}
		else
		{
			openLog("获取玩家椅子号错误 wMeChairID=%d", wMeChairID);
		}

    }catch(...)
    {
        //这里的设置，使得进入下面的if处理
        ZeroMemory(OutCardResult.cbResultCard, sizeof(OutCardResult.cbResultCard)) ;
        OutCardResult.cbCardCount = 10 ;
        //openLog("catch....wMeChairID=%d",m_pIAndroidUserItem->GetChairId());
    }

    //牌型合法判断
    if(OutCardResult.cbCardCount>0 && CT_ERROR==m_GameLogic.GetCardType(OutCardResult.cbResultCard, OutCardResult.cbCardCount))
    {
        //ASSERT(false) ;
        ZeroMemory(&OutCardResult, sizeof(OutCardResult)) ;
    }

    //先出牌不能为空
    if(m_cbTurnCardCount==0)
    {
        //ASSERT(OutCardResult.cbCardCount>0) ;
        if(OutCardResult.cbCardCount==0)
        {
            WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
            //最小一张
            OutCardResult.cbCardCount = 1 ;
            OutCardResult.cbResultCard[0]=m_cbHandCardData[m_cbHandCardCount[wMeChairID]-1] ;
        }
    }
    else
    {
        if(!m_GameLogic.CompareCard(m_cbTurnCardData,OutCardResult.cbResultCard,m_cbTurnCardCount,OutCardResult.cbCardCount))
        {
            //openLog("1114343411......");
            //放弃出牌
            m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
            return ;
        }
    }

    //printf("-------------------------------m_cbTurnCardCount%d OutCardResult.cbCardCount :%d\r\n" ,m_cbTurnCardCount,OutCardResult.cbCardCount);

    //openLog("11111......");
    //结果判断
    if (OutCardResult.cbCardCount>0)
    {
        //  openLog("222222......");
        WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
        ////删除扑克
        m_cbHandCardCount[wMeChairID]-=OutCardResult.cbCardCount;
        bool success =m_GameLogic.RemoveCard(OutCardResult.cbResultCard,OutCardResult.cbCardCount,m_cbHandCardData,m_cbHandCardCount[wMeChairID]+OutCardResult.cbCardCount);

        if(!success && m_cbTurnCardCount==0)
        {
            WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
            //最小一张
            OutCardResult.cbCardCount = 1 ;
            OutCardResult.cbResultCard[0]=m_cbHandCardData[m_cbHandCardCount[wMeChairID]-1] ;

            m_cbHandCardCount[wMeChairID]-=OutCardResult.cbCardCount;
            bool success =m_GameLogic.RemoveCard(OutCardResult.cbResultCard,OutCardResult.cbCardCount,m_cbHandCardData,m_cbHandCardCount[wMeChairID]+OutCardResult.cbCardCount);
        }
        else if(!success && m_cbTurnCardCount!=0)
        {
            //openLog("11erere111......");
            //放弃出牌
            m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
            return ;
        }
        //char tmp[100]={0};
        //GetValue(OutCardResult.cbResultCard,OutCardResult.cbCardCount,tmp);
        //openLog("OutCard wMeChairID=%d card=%s",wMeChairID,tmp);
        //构造变量
        ddz::CMD_C_OutCard  OutCard;
        OutCard.set_cbcardcount(OutCardResult.cbCardCount);
        for(int i=0;i<OutCardResult.cbCardCount;i++)
        {
            OutCard.add_cbcarddata(OutCardResult.cbResultCard[i]);
            // openLog("OutCard i=%d carddata=%d",i,OutCardResult.cbResultCard[i]);
        }
        char tmp[100]={0};
        GetValue(OutCardResult.cbResultCard,OutCardResult.cbCardCount,tmp);
        openLog("OutCard wChairID=%d %s",m_pIAndroidUserItem->GetChairId(),tmp);

        string data = OutCard.SerializeAsString();

        m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_OUT_CARD,(uint8_t *)data.c_str(),data.size());
    }else
    {
        //openLog("33333......");
        //放弃出牌
        m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
    }
}

//时间消息
//bool CAndroidUserItemSink::OnEventTimer(UINT nTimerID)
//bool CAndroidUserItemSink::OnTimerMessage(uint32_t timerid, uint32_t dwParam)
//{
//    switch (timerid)
//    {
//    case IDI_Ready:
//    case IDI_START_GAME:	//开始游戏
//    {
//        //开始判断
//        m_pIAndroidUserItem->setUserReady();
//        openLog("IDI_START_GAME || IDI_Ready");
//        return true;
//    }
//    case IDI_CALL_SCORE:	//用户叫分
//    {
//        openLog("IDI_CALL_SCORE");
//        //构造变量
//        uint8_t			cbCallScore;
//        //设置变量
//        cbCallScore = 255;
//        if(m_cbCurrentLandScore == 4 )
//            m_cbCurrentLandScore= 0;
//       // char tmp1[100]={0};
//        //GetValue(m_GameLogic.m_cbLandScoreCardData,MAX_COUNT,tmp1);
//       // m_GameLogic.SortCardList(m_GameLogic.m_cbLandScoreCardData,MAX_COUNT,ST_ORDER);
//       openLog("IDI_CALL_SCORE id=%d cbLandScoreCardData=%s",m_pIAndroidUserItem->GetChairId(),tmp1);
//       openLog("111111 id=%d m_cbCurrentLandScore=%d",m_pIAndroidUserItem->GetChairId(),static_cast<int>(m_cbCurrentLandScore));
//        // uint8_t cbCallScoreTemp = m_Robot.GrabLandlord();
//        uint8_t cbCallScoreTemp = m_GameLogic.LandScore(m_pIAndroidUserItem->GetChairId(), m_cbCurrentLandScore);
//        if(cbCallScoreTemp<0 || cbCallScoreTemp>3)
//            cbCallScoreTemp = 0;
//        if(cbCallScoreTemp>m_cbCurrentLandScore)
//        {
//            cbCallScore = cbCallScoreTemp;
//        }
//        else
//            cbCallScore = 0;
//        openLog("111111 cbCallScore=%d",static_cast<int>(cbCallScore));
//        ddz::CMD_C_CallBanker pCallScore;
//        pCallScore.set_cbcallinfo(cbCallScore);
//        std::string data = pCallScore.SerializeAsString();
//        m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),ddz::SUB_C_CALL_BANKER,data.data(),data.size());
//        return true;
//    }
//    case IDI_STAKE_SCORE:
//    {
//        openLog("IDI_STAKE_SCORE 1");
//        if (m_pIAndroidUserItem->GetChairId() != m_wBankerUser)
//        {
//            uint8_t							cbOpId;
//            openLog("IDI_STAKE_SCORE 2");
//            cbOpId = m_GameLogic.DoubleScore(m_pIAndroidUserItem->GetChairId(), 0) >=2 ? CB_ADD_DOUBLE : CB_NO_ADD_DOUBLE;
//            CMD_C_Double  wDouble;
//            wDouble.set_cbdoubleinfo(cbOpId);
//            std::string data = wDouble.SerializeAsString();
//            //发送数据
//            m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),ddz::SUB_C_ADD_DOUBLE, data.data(),data.size());
//        }
//        return true;
//    }
//    case IDI_OUT_CARD:		//用户出牌
//    {
//        openLog("IDI_OUT_CARD 1");
//        //扑克分析
//        tagOutCardResult OutCardResult;
//        memset(&OutCardResult,0,sizeof(OutCardResult));
//        try
//        {
//            WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
//            m_GameLogic.SearchOutCard(m_cbHandCardData,m_cbHandCardCount[wMeChairID],m_cbTurnCardData,m_cbTurnCardCount, m_wOutCardUser, m_pIAndroidUserItem->GetChairId(), OutCardResult);
//        }catch(...)
//        {
//            //这里的设置，使得进入下面的if处理
//            ZeroMemory(OutCardResult.cbResultCard, sizeof(OutCardResult.cbResultCard)) ;
//            OutCardResult.cbCardCount = 10 ;
//            openLog("catch....wMeChairID=%d",m_pIAndroidUserItem->GetChairId());
//        }
//        //牌型合法判断
//        if(OutCardResult.cbCardCount>0 && CT_ERROR==m_GameLogic.GetCardType(OutCardResult.cbResultCard, OutCardResult.cbCardCount))
//        {
//            //ASSERT(false) ;
//            ZeroMemory(&OutCardResult, sizeof(OutCardResult)) ;
//        }
//        //先出牌不能为空
//        if(m_cbTurnCardCount==0)
//        {
//            //ASSERT(OutCardResult.cbCardCount>0) ;
//            if(OutCardResult.cbCardCount==0)
//            {
//                WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
//                //最小一张
//                OutCardResult.cbCardCount = 1 ;
//                OutCardResult.cbResultCard[0]=m_cbHandCardData[m_cbHandCardCount[wMeChairID]-1] ;
//            }
//        }
//        else
//        {
//            if(!m_GameLogic.CompareCard(m_cbTurnCardData,OutCardResult.cbResultCard,m_cbTurnCardCount,OutCardResult.cbCardCount))
//            {
//                openLog("1114343411......");
//                //放弃出牌
//                m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
//                return true;
//            }
//        }
//        //printf("-------------------------------m_cbTurnCardCount%d OutCardResult.cbCardCount :%d\r\n" ,m_cbTurnCardCount,OutCardResult.cbCardCount);
//        openLog("11111......");
//        //结果判断
//        if (OutCardResult.cbCardCount>0)
//        {
//            //  openLog("222222......");
//            WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
//            ////删除扑克
//            m_cbHandCardCount[wMeChairID]-=OutCardResult.cbCardCount;
//            bool success =m_GameLogic.RemoveCard(OutCardResult.cbResultCard,OutCardResult.cbCardCount,m_cbHandCardData,m_cbHandCardCount[wMeChairID]+OutCardResult.cbCardCount);
//            if(!success && m_cbTurnCardCount==0)
//            {
//                WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
//                //最小一张
//                OutCardResult.cbCardCount = 1 ;
//                OutCardResult.cbResultCard[0]=m_cbHandCardData[m_cbHandCardCount[wMeChairID]-1] ;
//                m_cbHandCardCount[wMeChairID]-=OutCardResult.cbCardCount;
//                bool success =m_GameLogic.RemoveCard(OutCardResult.cbResultCard,OutCardResult.cbCardCount,m_cbHandCardData,m_cbHandCardCount[wMeChairID]+OutCardResult.cbCardCount);
//            }
//            else if(!success && m_cbTurnCardCount!=0)
//            {
//                openLog("11erere111......");
//                //放弃出牌
//                m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
//                return true;
//            }
//            //char tmp[100]={0};
//            //GetValue(OutCardResult.cbResultCard,OutCardResult.cbCardCount,tmp);
//            openLog("OutCard wMeChairID=%d card=%s",wMeChairID,tmp);
//            //构造变量
//            ddz::CMD_C_OutCard  OutCard;
//            OutCard.set_cbcardcount(OutCardResult.cbCardCount);
//            for(int i=0;i<OutCardResult.cbCardCount;i++)
//            {
//                OutCard.add_cbcarddata(OutCardResult.cbResultCard[i]);
//                openLog("OutCard i=%d carddata=%d",i,OutCardResult.cbResultCard[i]);
//            }
//            string data = OutCard.SerializeAsString();
//            m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_OUT_CARD,data.data(),data.size());
//        }else
//        {
//            openLog("33333......");
//            //放弃出牌
//            m_pITableFrame->OnGameEvent(m_pIAndroidUserItem->GetChairId(),SUB_C_PASS_CARD,0,0);
//        }
//        return true;
//    }
//    }
//    return false;
//}

//游戏消息
//bool CAndroidUserItemSink::OnEventGameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize)
bool CAndroidUserItemSink::OnGameMessage(uint8_t subid, const uint8_t* pData, uint32_t size)
{

    //if(!m_pIAndroidUserItem||!m_pITableFrame)
    //{
    //    return false;
    //}
    //openLog("OnGameMessage............");
    WORD wSubCmdID = subid;
    WORD wDataSize = size;
    switch (wSubCmdID)
    {
    case SUB_S_GAME_START:	//游戏开始
    {
        //openLog("OnGameMessage.. SUB_S_GAME_START..........");
        bool bResult = OnSubGameStart(pData, wDataSize);
        //openLog("OnGameMessage... SUB_S_GAME_START end.........");
        return bResult;
    }
    case SUB_S_CALL_BANKER:	//用户叫分
    {
        //openLog("OnGameMessage...SUB_S_CALL_BANKER.........");
        bool bResult = OnSubCallScore(pData, wDataSize);
        //openLog("OnGameMessage.....SUB_S_CALL_BANKER end.......");
        return bResult;
    }
    case SUB_S_BANKER_INFO:	//庄家信息
    {
        //openLog("OnGameMessage.....SUB_S_BANKER_INFO.......");
        bool bResult = OnSubBankerInfo(pData, wDataSize);
        //openLog("OnGameMessage.....SUB_S_BANKER_INFO end.......");
        return bResult;
    }
    case SUB_S_ADD_DOUBLE: //加倍信息
    {
        //openLog("OnGameMessage....SUB_S_ADD_DOUBLE........");
        bool bResult = OnSubStakeScore(pData, wDataSize);
        //openLog("OnGameMessage....SUB_S_ADD_DOUBLE end........");
        return bResult;
    }
    case SUB_S_OUT_START_START: //加倍结束
    {
        //openLog("OnGameMessage.....SUB_S_OUT_START_START.......");
        bool bResult = OnSubStakeEnd(pData, wDataSize);
        //openLog("OnGameMessage..SUB_S_OUT_START_START end..........");
        return bResult;
    }
    case SUB_S_OUT_CARD:	//用户出牌
    {
        //  openLog("OnGameMessage....SUB_S_OUT_CARD ........");
        bool bResult = OnSubOutCard(pData, wDataSize);
        //openLog("OnGameMessage....SUB_S_OUT_CARD end........");
        return bResult;
    }
    case SUB_S_PASS_CARD:	//用户放弃
    {
        //openLog("OnGameMessage...SUB_S_PASS_CARD.........");
        bool bResult = OnSubPassCard(pData, wDataSize);
        //openLog("OnGameMessage....SUB_S_PASS_CARD end........");
        return bResult;
    }
    case SUB_S_GAME_CONCLUDE:	//游戏结束
    {
        //openLog("OnGameMessage....SUB_S_GAME_CONCLUDE........");
        bool bResult = OnSubGameEnd(pData, wDataSize);
        //openLog("OnGameMessage...SUB_S_GAME_CONCLUDE end.........");
        return bResult;
    }
        // case SUB_S_USER_EXIT:	//用户退出
        // {
        //     return true;
        //}
    case SUB_S_TRUSTEE:		//用户委托
    {
        bool bResult = OnSubTrustee(pData, wDataSize);
        return bResult;
    }
    case SC_GAMESCENE_FREE:
    case SC_GAMESCENE_CALL:
    case SC_GAMESCENE_PLAY:
    case SC_GAMESCENE_END:
    case SC_GAMESCENE_DOUBLE:
    {
        //openLog("OnGameMessage....OnEventSceneMessage........");
        return  OnEventSceneMessage(wSubCmdID,false,pData,wDataSize);
    }
    }

    //错误断言
    //ASSERT(FALSE);

    return true;
}

//游戏消息
bool CAndroidUserItemSink::OnEventFrameMessage(WORD wSubCmdID, const uint8_t* pData, WORD wDataSize)
{
    return true;
}

//场景消息
bool CAndroidUserItemSink::OnEventSceneMessage(uint8_t cbGameStatus, bool bLookonOther, const uint8_t* pData, WORD wDataSize)
{
    switch (cbGameStatus)
    {
    case GAME_SCENE_FREE:	//空闲状态
    {
        ddz::CMD_S_StatusFree pStatusFree;
        if (!pStatusFree.ParseFromArray(pData,wDataSize))
            return false;

        //m_pIAndroidUserItem->SetTimer(IDI_START_GAME,1);
        // time_t    tmp= IDI_START_GAME* TIMEXS;
        //m_TimerSTARTGAME.start(tmp, bind(&CAndroidUserItemSink::OnTimerOnTimerIDI_START_GAME, this, std::placeholders::_1), NULL, 1, false);
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerStart);
        OnTimerOnTimerStart=m_TimerLoopThread->getLoop()->runAfter(IDI_START_GAME,boost::bind(&CAndroidUserItemSink::OnTimerOnTimerIDI_START_GAME, this));
        return true;
    }
    case GAME_SCENE_CALL:	//叫分状态
    {

        ddz::CMD_S_StatusCall pStatusCall;
        if (!pStatusCall.ParseFromArray(pData,wDataSize))
            return false;

        //扑克数据
        for (WORD i=0;i<GAME_PLAYER;i++)
            m_cbHandCardCount[i]=NORMAL_COUNT;

        const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&handcarddata=  pStatusCall.cbhandcarddata();
        for(WORD i=0;i<handcarddata.size();i++)
            m_cbHandCardData[i] = handcarddata[i];

        //叫分设置
        if (m_pIAndroidUserItem->GetChairId()==pStatusCall.wcurrentuser())
        {
            UINT nElapse=rand()%TIME_CALL_SCORE+TIME_LESS;
           // m_pIAndroidUserItem->SetTimer(IDI_CALL_SCORE,(nElapse+TIME_DISPATCH)* TIMEXS,1);
            // time_t    tmp= nElapse* TIMEXS;
            m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerCall);
            OnTimerOnTimerCall=m_TimerLoopThread->getLoop()->runAfter(nElapse, bind(&CAndroidUserItemSink::OnTimerIDI_CALL_SCORE, this));

        }

        return true;
    }
    case GAME_SCENE_STAKE:
    {
        //效验数据
        //if (wDataSize != sizeof(CMD_S_StatusStake)) return false;
        //CMD_S_StatusStake * pStatusStake = (CMD_S_StatusStake *)pData;

        ddz::CMD_S_StatusDouble pStatusStake ;
        if (!pStatusStake.ParseFromArray(pData,wDataSize))
            return false;

        return true;
    }
    case GAME_SCENE_PLAY:	//游戏状态
    {
        //效验数据
        //if (wDataSize!=sizeof(CMD_S_StatusPlay)) return false;
        //CMD_S_StatusPlay * pStatusPlay=(CMD_S_StatusPlay *)pData;

        ddz::CMD_S_StatusPlay  pStatusPlay;
        if (!pStatusPlay.ParseFromArray(pData,wDataSize))
            return false;

        //出牌变量
        m_cbTurnCardCount=pStatusPlay.cbturncardcount();

        const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&turncarddata = pStatusPlay.cbturncarddata();
        for(int i=0;i<m_cbTurnCardCount;i++)
            m_cbTurnCardData[i] = turncarddata[i];

        //扑克数据
        // WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
        const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&handcardcount =pStatusPlay.cbhandcardcount();
        for (WORD i=0;i<GAME_PLAYER;i++)
            m_cbHandCardCount[i] =handcardcount[i];
        const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&handcarddata =pStatusPlay.cbhandcarddata();
        for(WORD j=0;j<handcarddata.size();j++)
            m_cbHandCardData[j] = handcarddata[j];

        //玩家设置
        if (pStatusPlay.dwcurrentuser()==m_pIAndroidUserItem->GetChairId())
        {
            UINT nElapse=rand()%TIME_OUT_CARD+TIME_LESS;
            //m_pIAndroidUserItem->SetTimer(IDI_OUT_CARD,nElapse*TIMEXS,1);
            // time_t    tmp= nElapse*TIMEXS;
            m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);
            OnTimerOnTimerOut=m_TimerLoopThread->getLoop()->runAfter(nElapse,boost::bind(&CAndroidUserItemSink::OnTimerIDI_OUT_CARD,this));
        }

        return true;
    }
    }

    //错误断言
    ASSERT(FALSE);

    return false;
}
/*
//用户进入
VOID CAndroidUserItemSink::OnEventUserEnter(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser)
{
    return;
}

//用户离开
VOID CAndroidUserItemSink::OnEventUserLeave(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser)
{
    return;
}

//用户积分
VOID CAndroidUserItemSink::OnEventUserScore(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser)
{
    return;
}

//用户状态
VOID CAndroidUserItemSink::OnEventUserStatus(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser)
{
    return;
}

//用户段位
VOID CAndroidUserItemSink::OnEventUserSegment(IAndroidUserItem * pIAndroidUserItem, bool bLookonUser)
{
    return;
}
*/
short form_val2(short v);

short form_color(uint8_t v);

void CAndroidUserItemSink::SetGameStatus(uint8_t uc)
{
    m_ucGameStatus = uc;
}


//游戏开始
bool CAndroidUserItemSink::OnSubGameStart(const uint8_t* pData, WORD wDataSize)
{
    //效验参数

    ddz::CMD_S_GameStartAi AndroidCard;
    if (!AndroidCard.ParseFromArray(pData,wDataSize))
        return false;


    //设置状态
    SetGameStatus(GAME_SCENE_CALL);
    m_cbCurrentLandScore = 0 ;
	CountrandomRound++;
    //扑克变量
    m_cbTurnCardCount=0;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));

    //手上扑克
    WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
    for (WORD i=0;i<GAME_PLAYER;i++)
        m_cbHandCardCount[i]=NORMAL_COUNT;

    uint8_t hands[3][pai_i_type_max] = { 0 };
    uint32_t hands_real[3][pai_num_max] = { 0 };

    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >& carddata = AndroidCard.cbcarddata();
    int tsize =  carddata.size();
    for(WORD wChairID=0; wChairID<GAME_PLAYER; ++wChairID)
    {
        uint8_t cbHandCardDataTmp[MAX_COUNT];		//手上扑克

        for(int i=0;i<NORMAL_COUNT;i++)
            cbHandCardDataTmp[i] =carddata[wChairID*NORMAL_COUNT+i];

        if(wChairID==wMeChairID)
            CopyMemory(m_cbHandCardData,cbHandCardDataTmp,sizeof(cbHandCardDataTmp));

        char tmp[100]={0};
        GetValue(cbHandCardDataTmp,NORMAL_COUNT,tmp);
        openLog("wChairID=%s",tmp);
        m_GameLogic.SetUserCard(wChairID, cbHandCardDataTmp, NORMAL_COUNT) ;

		shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
		if (pIServerUserItem != NULL) {
			openLog("pIServerUserItem wChairID = %d", pIServerUserItem->GetChairId());
		}
    }

    //叫牌扑克
    uint8_t cbLandScoreCardData[MAX_COUNT] ;
    uint8_t cbBackCard[3];
    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >& bankercard = AndroidCard.cbbankercard();
    for(int i=0;i<3;i++)
        cbBackCard[i] = bankercard[i];
    CopyMemory(cbLandScoreCardData, m_cbHandCardData,sizeof(m_cbHandCardData)) ;
    CopyMemory(cbLandScoreCardData+NORMAL_COUNT, cbBackCard, sizeof(cbBackCard)) ;

    //char tmp1[100]={0};
    //GetValue(cbLandScoreCardData,MAX_COUNT,tmp1);
    //m_GameLogic.SortCardList(cbLandScoreCardData,MAX_COUNT,ST_ORDER);
    //openLog("id=%d cbLandScoreCardData=%s",wMeChairID,tmp1);

    char tmp[100]={0};
    GetValue(cbBackCard,3,tmp);
    openLog("BackCard =%s",tmp);
    m_GameLogic.SetLandScoreCardData(cbLandScoreCardData, sizeof(cbLandScoreCardData)) ;

    //排列扑克
    m_GameLogic.SortCardList(m_cbHandCardData,m_cbHandCardCount[wMeChairID],ST_ORDER);

    //玩家处理
    if (m_pIAndroidUserItem->GetChairId()==AndroidCard.dwcurrentuser())
    {
        //openLog("m_pIAndroidUserItem->GetChairId()==AndroidCard.dwcurrentuser()");
        UINT nElapse=rand()%TIME_CALL_SCORE+TIME_LESS;
        //m_pIAndroidUserItem->SetTimer(IDI_CALL_SCORE,(nElapse+TIME_DISPATCH)*TIMEXS,1);
        // time_t    tmp= (nElapse+TIME_DISPATCH)*TIMEXS;
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerCall);
        OnTimerOnTimerCall=m_TimerLoopThread->getLoop()->runAfter(nElapse, bind(&CAndroidUserItemSink::OnTimerIDI_CALL_SCORE, this));
    }

    return true;
}

//用户叫分
bool CAndroidUserItemSink::OnSubCallScore(const uint8_t* pData, WORD wDataSize)
{
    //效验参数
    //ASSERT(wDataSize==sizeof(CMD_S_CallScore));
    //if (wDataSize!=sizeof(CMD_S_CallScore)) return false;

    ddz::CMD_S_CallBanker CallBanker;
    if (!CallBanker.ParseFromArray(pData,wDataSize))
        return false;

    //openLog("111111 m_cbCurrentLandScore=%d",CallBanker.cbcallinfo());

    if(CallBanker.cbcallinfo() > m_cbCurrentLandScore)
         m_cbCurrentLandScore = (m_cbCurrentLandScore>0 && CallBanker.cbcallinfo()==4 )? m_cbCurrentLandScore:CallBanker.cbcallinfo() ;
    if(m_cbCurrentLandScore == 4)
        m_cbCurrentLandScore= 0;
    //用户处理
    if (m_pIAndroidUserItem->GetChairId()==CallBanker.dwcurrentuser())
    {
        //openLog("m_pIAndroidUserItem->GetChairId()==CallBanker.dwcurrentuser()");
        UINT nElapse=rand()%TIME_CALL_SCORE+1;
        //m_pIAndroidUserItem->SetTimer(IDI_CALL_SCORE,nElapse*TIMEXS,1);
        // time_t    tmp= nElapse*TIMEXS;
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerCall);
        OnTimerOnTimerCall=m_TimerLoopThread->getLoop()->runAfter(nElapse, bind(&CAndroidUserItemSink::OnTimerIDI_CALL_SCORE, this));
    }

    return true;
}

//庄家信息
bool CAndroidUserItemSink::OnSubBankerInfo(const uint8_t* pData, WORD wDataSize)
{
    //效验参数
    //ASSERT(wDataSize==sizeof(CMD_S_BankerInfo));
    //if (wDataSize!=sizeof(CMD_S_BankerInfo)) return false;

    ddz::CMD_S_BankerInfo BankerInfo;

    if (!BankerInfo.ParseFromArray(pData,wDataSize))
        return false;




    //设置状态
    SetGameStatus(GAME_SCENE_STAKE);

    //设置变量
    m_wBankerUser=BankerInfo.dwbankeruser();


    m_cbHandCardCount[m_wBankerUser]+=3;

    uint8_t bankercardtmp[3] ={0};
    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&bankercard =  BankerInfo.cbbankercard();
    for(int i=0;i<3;i++)
        bankercardtmp[i] =bankercard[i];

    //设置扑克
    if (BankerInfo.dwbankeruser()==m_pIAndroidUserItem->GetChairId())
    {
        //设置扑克
        CopyMemory(&m_cbHandCardData[NORMAL_COUNT],bankercardtmp,sizeof(bankercardtmp));

        //排列扑克
        WORD wMeChairID=m_pIAndroidUserItem->GetChairId();
        m_GameLogic.SortCardList(m_cbHandCardData,m_cbHandCardCount[wMeChairID],ST_ORDER);
    }
    //设置底牌
    m_GameLogic.SetBackCard(BankerInfo.dwbankeruser(), bankercardtmp, 3) ;
    m_GameLogic.SetBanker(BankerInfo.dwbankeruser()) ;

    if (m_wBankerUser != m_pIAndroidUserItem->GetChairId())
    {
        //openLog("m_wBankerUser != m_pIAndroidUserItem->GetChairId()");
        UINT nElapse = rand() % TIME_STAKE_SCORE + 1;
        //m_pIAndroidUserItem->SetTimer(IDI_STAKE_SCORE, nElapse*TIMEXS,1);
        // time_t    tmp= nElapse*TIMEXS;
        //m_TimerSTAKESCORE.start(tmp, bind(&CAndroidUserItemSink::OnTimerIDI_STAKE_SCORE, this, std::placeholders::_1), NULL, 1, false);
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerStake);
        OnTimerOnTimerStake=m_TimerLoopThread->getLoop()->runAfter(nElapse, bind(&CAndroidUserItemSink::OnTimerIDI_STAKE_SCORE, this));
    }

    return true;
}


//庄家加倍
bool CAndroidUserItemSink::OnSubStakeScore(const uint8_t* pData, WORD wDataSize)
{
    /*
    if (m_pIAndroidUserItem->GetGameStatus() != GAME_SCENE_STAKE)
    {
        ASSERT(0);
        return false;
    }

    //效验参数
    ASSERT(wDataSize == sizeof(CMD_S_StakeScore));
    if (wDataSize != sizeof(CMD_S_StakeScore)) return false;
    */

    return true;
}

//加倍结束
bool CAndroidUserItemSink::OnSubStakeEnd(const uint8_t* pData, WORD wDataSize)
{
    ddz::CMD_S_StartOutCard StartOutCard;

    if (!StartOutCard.ParseFromArray(pData,wDataSize))
        return false;

    if (StartOutCard.dwcurrentuser() == m_pIAndroidUserItem->GetChairId())
    {
       // openLog("StartOutCard.dwcurrentuser() == m_pIAndroidUserItem->GetChairId()");
        UINT nElapse = rand() % TIME_OUT_CARD + TIME_LESS + 3;
        //m_pIAndroidUserItem->SetTimer(IDI_OUT_CARD, nElapse*TIMEXS,1);
        // time_t    tmp= nElapse*TIMEXS;
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);
        OnTimerOnTimerOut=m_TimerLoopThread->getLoop()->runAfter(nElapse,boost::bind(&CAndroidUserItemSink::OnTimerIDI_OUT_CARD,this));
    }

    return true;
}

static bool	IsDanPai(const vector<ACard>& vCard)
{//判断是否是单牌
    return vCard.size() == 1;
}
static bool	IsKing(const ACard& vCard)//判断是否是王牌(大王、小王)
{
    switch (vCard.n8CardNumber)
    {
    case emPokeValue_SmallKing:
    case emPokeValue_BigKing:
        return true;
    }
    return false;
}

static bool	IsDuiZi(const vector<ACard>& vCard)
{//判断是否是对子牌
    //两张牌 and 两张牌都不能是王牌 and (两张牌点数相等 or (两张牌中有一张是听牌))
    return vCard.size() == 2 &&
            (!IsKing(vCard[0]) && !IsKing(vCard[1])) && ((vCard[0].n8CardNumber == vCard[1].n8CardNumber));

}

static bool SortTotalCardNum(const vector<ACard>& vCard, map<int, int>& mCardNum)
{//分类汇总牌点的出现次数(返回是否有听用牌)

    ACard info;
    for (unsigned i = 0; i<vCard.size(); i++)
    {
        info = vCard[i];

        mCardNum[info.n8CardNumber] += 1;
    }
    return false;
}

static bool  IsLianXuPai(vector<int> &vData)
{//判断是否是连续的
    if (vData.size() <= 0)return  false;
    if (vData.size()>1)
    {
        sort(vData.begin(), vData.end());
        for (unsigned int i = 0; i<vData.size() - 1; i++)
        {
            if (vData[i] + 1 != vData[i + 1])
                return false;
        }
    }
    return true;

}

static bool	IsLianDui(const vector<ACard>& vCard)
{//判断是否是连对
    //至少6张牌
    if (vCard.size()<6 || vCard.size() % 2 != 0 || vCard.size()>20)//最多10对
        return false;
    //没有大小王和2
    map<int, int> mCardNum; //牌点计数
    ACard info;
    for (unsigned int i = 0; i<vCard.size(); i++)
    {
        info = vCard[i];

        if (info.CanCalShuiZhi() == false) //不是能计算顺子的牌
            return false;
        mCardNum[info.n8CardNumber] += 1;
    }
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->second != 2)//对子的数量不对
            return false;
    }
    vector<int> vTwoCard;//数量够的牌
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        vTwoCard.push_back(it->first);
    }

    //检测顺子
    bool bRe = IsLianXuPai(vTwoCard);
    return bRe;
}

static bool	HasKing(const vector<ACard>& vCard)//判断是否有王牌
{
    for (unsigned int i = 0; i<vCard.size(); i++)
    {
        if (IsKing(vCard[i]))
            return true;
    }
    return false;
}

static bool	IsSanZhang(const vector<ACard>& vCard)
{//是否是三张
    if (vCard.size() != 3) return false;
    if (HasKing(vCard)) return false;//不能有王牌
    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    if (mCardNum.size() != 1)return false; //只会有一张牌的统计
    map<int, int>::iterator it = mCardNum.begin();
    return it->second == 3;
}

static bool	IsSanDaiDanPai(const vector<ACard>& vCard)
{//三带单牌

    if (vCard.size() != 4) return false;
    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    if (mCardNum.size() != 2) return false;//牌花不够
    int nCheckNum = 3;
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->second == nCheckNum)
            return true;

    }
    return false;
}
static bool	IsSanDaiDuiZi(const vector<ACard>& vCard)
{
    if (vCard.size() != 5) return false;
    if (HasKing(vCard)) return false;//不能有王牌

    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    if (mCardNum.size() != 2) return false;//先保证只有两种点数的牌
    //5张牌2种点数，可能出现1+4和3+2，因此只要保证一种点数不能有4张就可以了
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->second == 4)
            return false;
    }
    return true;
}

static bool				IsShunZi(const vector<ACard>& vCard)
{

    if (vCard.size()<5 || vCard.size()>12) //不是5张不能连||顺子最多12张
        return false;
    vector<int> vTemp;
    ACard info;
    for (int i = 0; i<(int)vCard.size(); i++)	//存在听用T除
    {
        info = vCard[i];
        if (info.CanCalShuiZhi() == false)//出现不能当顺子的牌
        {
            return false;
        }
        vTemp.push_back(vCard[i].n8CardNumber);
    }
    if (vTemp.size()>1)
        sort(vTemp.begin(), vTemp.end());
    return IsLianXuPai(vTemp);


}

static bool	IsSanShunZi(const vector<ACard>& vCard)
{
    //至少6张牌
    if (vCard.size() < 6 || vCard.size() % 3 != 0 || vCard.size()>21)//最多21张
        return false;
    //没有大小王和2
    map<int, int> mCardNum; //牌点计数
    ACard info;
    for (unsigned int i = 0; i<vCard.size(); i++)
    {
        info = vCard[i];

        if (info.CanCalShuiZhi() == false) //不是能计算顺子的牌
            return false;
        mCardNum[info.n8CardNumber] += 1;
    }
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->second != 3)//数量不对
            return false;
    }
    vector<int> vTwoCard;//数量够的牌
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        vTwoCard.push_back(it->first);
    }
    //检测顺子
    bool bRe = IsLianXuPai(vTwoCard);
    return bRe;
}


static bool		IsFeiJiDaiDuiZiNoTingYongPai(const vector<ACard>& vCard)
{

    //至少10张牌
    if (vCard.size()<10)
        return false;

    //牌的个数一定是5的倍数
    if ((vCard.size() % 5) != 0) return false;

    //int needNum=0;
    //needNum = vCard.size()/5;//需要至少连续3张一样牌的个数

    //找出等于3张牌的牌,并且没有2
    int nTemp2 = 0;//找出等于2张的牌
    int nTemp4 = 0;//找出等于4张的牌
    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    vector<int> sTemp;
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        switch (it->second)
        {
        case 2://找出等于2张的牌
            nTemp2 = nTemp2 + 1;
            break;
        case 4:	//找出等于4张的牌
            nTemp4 = nTemp4 + 1;
            break;
        case 5: //5张表是三张+2对。有听用牌
            if (it->first == (int)emPokeValue_Two)
                continue;
            sTemp.push_back(it->first);
            nTemp2 = nTemp2 + 1;
            break;
        case 3:
            if (it->first == (int)emPokeValue_Two)
                continue;

            sTemp.push_back(it->first);
            break;
        }

    }

    //牌数的判定
    if (sTemp.size() != nTemp2 + nTemp4 * 2)
        return false;
    if (sTemp.size() * 3 + nTemp2 * 2 + nTemp4 * 4 != vCard.size())
        return false;

    //连续判定

    bool bRe = IsLianXuPai(sTemp);
    return bRe;

}

static bool		IsFeiJiDaiDanPaiNoTingYongPai(const vector<ACard>& vCard)
{		//至少8张牌
    if (vCard.size()<8)
        return false;

    //牌的个数一定是4的倍数
    if ((vCard.size() % 4) != 0) return false;

    int needNum = 0;
    needNum = vCard.size() / 4;//需要至少连续3张一样牌的个数

    //找出大于等于3张牌的牌,并且没有2
    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);

    vector<int> sTemp;
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->first == (int)emPokeValue_Two)
            continue;
        if (it->second >= 3)
            sTemp.push_back(it->first);
    }


    int maxNum = 1;//最大的连续点数
    if (sTemp.size() > 1)
    {
        sort(sTemp.begin(), sTemp.end());
        for (unsigned int i = 0; i<sTemp.size() - 1; i++)//如果不相等，那么就从1开始计数
        {
            if (sTemp[i] + 1 == sTemp[i + 1])
                maxNum = maxNum + 1;
            else
                maxNum = 1;
        }
    }

    if (maxNum >= needNum) //如果3张一样的牌连续次数大于等于需要的个数，就是飞机带单牌
        return true;
    else
        return false;
}

static bool	IsFeiJiDaiDanPai(const vector<ACard>& vCard)
{//飞机带单牌
    //至少8张牌
    if (vCard.size()<8)
        return false;

    //牌的个数一定是4的倍数
    if ((vCard.size() % 4) != 0) return false;

    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    return IsFeiJiDaiDanPaiNoTingYongPai(vCard);

}

static bool	IsFeiJiDaiDuiZi(const vector<ACard>& vCard)
{
    //至少10张牌
    if (vCard.size()<10)
        return false;

    //牌的个数一定是5的倍数
    if ((vCard.size() % 5) != 0) return false;

    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);

    return IsFeiJiDaiDuiZiNoTingYongPai(vCard);
}

static bool	Is4Dai2DanPai(const vector<ACard>& vCard)
{
    //4带两单牌
    if (vCard.size() != 6) return false;

    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);

    int nCheckNum = 4;
    for (map<int, int>::iterator it = mCardNum.begin(); it != mCardNum.end(); it++)
    {
        if (it->second >= nCheckNum)
            return true;
    }
    return false;
}

static bool	IsYingZha(const vector<ACard>& vCard)
{//是否是硬炸
    if (vCard.size() != 4)return false;
    map<int, int> mCardNum;
    SortTotalCardNum(vCard, mCardNum);
    return mCardNum.size() == 1; //没有听用牌+3个一样的牌
}
static bool	IsHuoJian(const vector<ACard>& vCard)
{//火箭
    if (vCard.size() == 2)
    {
        for (unsigned int i = 0; i<vCard.size(); i++)
        {
            switch (vCard[i].n8CardNumber)
            {
            case emPokeValue_SmallKing:
            case emPokeValue_BigKing:
                continue;
            default:
                return false;
            }
        }
        return true;
    }
    else
        return false;
}

emCODDZPaiXing		GetPaiXing(const vector<ACard>& vCard)
{
    if (IsDanPai(vCard))
        return emCODDZPaiXing_DanPai;
    else if (IsDuiZi(vCard))
        return emCODDZPaiXing_DuiZi;
    else if (IsLianDui(vCard))
        return emCODDZPaiXing_LianDui;
    else if (IsSanZhang(vCard))
        return emCODDZPaiXing_SanZhang;
    else if (IsSanDaiDanPai(vCard))
        return emCODDZPaiXing_SanDaiDanPai;
    else if (IsSanDaiDuiZi(vCard))
        return emCODDZPaiXing_SanDaiDuiZi;
    else if (IsShunZi(vCard))
        return emCODDZPaiXing_ShunZi;
    else if (IsSanShunZi(vCard))
        return emCODDZPaiXing_SanShunZi;
    else if (IsFeiJiDaiDanPai(vCard))
        return emCODDZPaiXing_FeiJiDaiDanPai;
    else if (IsFeiJiDaiDuiZi(vCard))
        return emCODDZPaiXing_FeiJiDaiDuiZi;
    else if (IsYingZha(vCard))
        return emCODDZPaiXing_YingZha;
    else if (IsHuoJian(vCard))
        return emCODDZPaiXing_HuoJian;
#ifdef SIDAI2DANPAI
    else if (Is4Dai2DanPai(vCard))
        return emCODDZPaiXing_4Dai2DanPai;
#endif
#ifdef SIDAI2DUIZI
    else if (Is4Dai2DuiZi(vCard))
        return emCODDZPaiXing_4Dai2DuiZi;
#endif
    else
    {
        return emCODDZPaiXing_Invalid;
    }
}

//用户出牌
bool CAndroidUserItemSink::OnSubOutCard(const uint8_t* pData, WORD wDataSize)
{
    if (NULL == pData) return false;

    ddz::CMD_S_OutCard pOutCard;
    if (!pOutCard.ParseFromArray(pData,wDataSize))
        return false;

    WORD wMeChairID = m_pIAndroidUserItem->GetChairId();
	m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);

    uint8_t outCardTmp[20];

    const ::google::protobuf::RepeatedField< ::google::protobuf::int32 >&outCard = pOutCard.cbcarddata();
    for (unsigned int i = 0; i < pOutCard.cbcardcount(); i++)
    {
        outCardTmp[i] = outCard[i];
    }


    //出牌变量
    if (pOutCard.dwcurrentuser()==pOutCard.dwoutcarduser())
    {
        m_cbTurnCardCount=0;
        ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));
    }
    else
    {
        m_cbTurnCardCount=pOutCard.cbcardcount();
        CopyMemory(m_cbTurnCardData,outCardTmp,sizeof(outCardTmp));
        m_wOutCardUser = pOutCard.dwoutcarduser() ;
    }

    //扑克数目
    if (pOutCard.dwoutcarduser() !=m_pIAndroidUserItem->GetChairId())
    {
        m_cbHandCardCount[pOutCard.dwoutcarduser()]-=pOutCard.cbcardcount();
    }

    //设置变量
    m_GameLogic.RemoveUserCardData(pOutCard.dwoutcarduser(), outCardTmp, pOutCard.cbcardcount()) ;

    //玩家设置
    if (m_pIAndroidUserItem->GetChairId()==pOutCard.dwcurrentuser())
    {
        openLog("m_pIAndroidUserItem->GetChairId()==pOutCard.dwcurrentuser()");
        UINT nElapse=rand()%TIME_OUT_CARD+TIME_LESSNew;
        //m_pIAndroidUserItem->SetTimer(IDI_OUT_CARD,nElapse*TIMEXS,1);
        // time_t    tmp= nElapse*TIMEXS;
        openLog("OnSubOutCard time=%u",nElapse);
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);
        OnTimerOnTimerOut=m_TimerLoopThread->getLoop()->runAfter(nElapse,boost::bind(&CAndroidUserItemSink::OnTimerIDI_OUT_CARD,this));
    }


    return true;
}

//用户放弃
bool CAndroidUserItemSink::OnSubPassCard(const uint8_t* pData, WORD wDataSize)
{
    //效验数据
    //ASSERT(wDataSize==sizeof(CMD_S_PassCard));
    //if (wDataSize!=sizeof(CMD_S_PassCard)) return false;

    ddz::CMD_S_PassCard PassCard;
    if (!PassCard.ParseFromArray(pData,wDataSize))
        return false;
    
    openLog("OnSubPassCard passUser=%d",PassCard.dwcurrentuser());
	m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);
    //一轮判断
    if (PassCard.cbturnover()==TRUE)
    {
        m_cbTurnCardCount=0;
        ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));
    }

    //  if(PassCard.dwpasscarduser() == m_pIAndroidUserItem->GetChairId())
    //{
    //     m_pIAndroidUserItem->KillTimer(IDI_OUT_CARD);
    //  }

    //玩家设置
    if (m_pIAndroidUserItem->GetChairId()==PassCard.dwcurrentuser())
    {
        openLog("m_pIAndroidUserItem->GetChairId()==PassCard.dwcurrentuser()");
        UINT nElapse = rand() % TIME_OUT_CARD + TIME_LESSNew;
        //m_pIAndroidUserItem->SetTimer(IDI_OUT_CARD,nElapse*TIMEXS,1);
        // time_t    tmp= nElapse*TIMEXS;
        //m_TimerOUTCARD.start(tmp, bind(&CAndroidUserItemSink::OnTimerIDI_OUT_CARD, this, std::placeholders::_1), NULL, 1, false);
        m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerOut);
        OnTimerOnTimerOut=m_TimerLoopThread->getLoop()->runAfter(nElapse,boost::bind(&CAndroidUserItemSink::OnTimerIDI_OUT_CARD,this));
    }

    return true;
}

//游戏结束
bool CAndroidUserItemSink::OnSubGameEnd(const uint8_t* pData, WORD wDataSize)
{
    //m_TimerOUTCARD.stop();
    UINT nElapse = rand() % TIME_READY + TIME_LESS;
    // time_t    tmp= nElapse;
    openLog("OnSubGameEnd runAfter time=%d ",nElapse);
    m_TimerLoopThread->getLoop()->cancel(OnTimerOnTimerStart);
    OnTimerOnTimerStart= m_TimerLoopThread->getLoop()->runAfter(nElapse, bind(&CAndroidUserItemSink::OnTimerOnTimerIDI_START_GAME, this));
	if (CountrandomRound >= randomRound)
	{
		openLog("OnSubGameEnd m_pIAndroidUserItem->setOffline() = %d ", m_pIAndroidUserItem->GetChairId());
		m_pIAndroidUserItem->setOffline();
		CountrandomRound = 0;
	}
	int getout = rand() % 100;
	if (m_pITableFrame->GetPlayerCount(false) >= 2 && getout < 50)
	{
		m_pIAndroidUserItem->setOffline();
		CountrandomRound = 0;
	}
	if (m_pIAndroidUserItem->GetUserScore() < m_pITableFrame->GetGameRoomInfo()->enterMinScore)
	{
		m_pIAndroidUserItem->setOffline();
	}

    /*
    //效验数据
    //ASSERT(wDataSize==sizeof(CMD_S_GameConclude));
    //if (wDataSize!=sizeof(CMD_S_GameConclude)) return false;

    CMD_S_GameConclude GameEnd;
    DouDiZhu::CCMD_S_GameConclude _CCMD_S_GameConclude;
    _CCMD_S_GameConclude.ParseFromArray(pData,wDataSize);
    _CCMD_S_GameConclude.ToStruct(GameEnd);

    //变量定义
    CMD_S_GameConclude * pGameEnd= &GameEnd;//(CMD_S_GameConclude *)pData;

    //设置状态
    SetGameStatus(GAME_SCENE_FREE);

    //设置变量
    m_cbTurnCardCount=0;
    ZeroMemory(m_cbTurnCardData,sizeof(m_cbTurnCardData));
    ZeroMemory(m_cbHandCardData,sizeof(m_cbHandCardData));
    ZeroMemory(m_cbHandCardCount,sizeof(m_cbHandCardCount));


    //删除时间
    m_pIAndroidUserItem->KillTimer(IDI_OUT_CARD);
    m_pIAndroidUserItem->KillTimer(IDI_CALL_SCORE);


    //开始设置
    UINT nElapse = rand()%(5)+TIME_LESS+3;
    m_pIAndroidUserItem->SetTimer(IDI_START_GAME, nElapse*TIMEXS);
*/
    return true;
}

//托管
bool CAndroidUserItemSink::OnSubTrustee(const uint8_t* pData, WORD wDataSize)
{
    return true;
}


void  CAndroidUserItemSink::openLog(const char *p, ...)
{
    char Filename[256] = { 0 };

    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    sprintf(Filename, "log//ddz//ddz_%d%d%d_%d_AI_%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,m_pITableFrame->GetGameRoomInfo()->roomId, m_pITableFrame->GetTableId());
    FILE *fp = fopen(Filename, "a");
    if (NULL == fp)
    {
        return;
    }
    va_list arg;
    va_start(arg, p);
    fprintf(fp, "[%d%d%d %02d:%02d:%02d][AndroidUser wchairID=%u]",
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec,m_pIAndroidUserItem->GetChairId());
    vfprintf(fp, p, arg);
    fprintf(fp, "\n");
    fclose(fp);
}


char * CAndroidUserItemSink::GetValue(uint8_t itmp[],uint8_t count, char * name)
{
    for(int i=0 ;i<count;i++)
    {
        uint8_t cbValue = m_GameLogic.GetCardValue(itmp[i]);
        //获取花色
        uint8_t cbColor = m_GameLogic.GetCardColor(itmp[i])>>4;

        if(itmp[i] == 0x4E)
        {
            strcat(name, "0x4E,");
            continue;
        }
        else if(itmp[i] == 0x4F)
        {
            strcat(name, "0x4F,");
            continue;
        }

        if (0 == cbColor)
        {
            strcat(name, "0x0");
        }
        else if (1 == cbColor)
        {
            strcat(name, "0x1");
        }
        else if (2 == cbColor)
        {
            strcat(name, "0x2");
        }
        else if (3 == cbColor)
        {
            strcat(name, "0x3");
        }


        if (cbValue == 1)
        {
            strcat(name, "1,");
        }
        else if (cbValue == 2)
        {
            strcat(name, "2,");
        }
        else if (cbValue == 3)
        {
            strcat(name, "3,");
        }
        else if (cbValue == 4)
        {
            strcat(name, "4,");
        }
        else if (cbValue == 5)
        {
            strcat(name, "5,");
        }
        else if (cbValue == 6)
        {
            strcat(name, "6,");
        }
        else if (cbValue == 7)
        {
            strcat(name, "7,");
        }
        else if (cbValue == 8)
        {
            strcat(name, "8,");
        }
        else if (cbValue == 9)
        {
            strcat(name, "9,");
        }
        else if (cbValue == 10)
        {
            strcat(name, "A,");
        }
        else if (cbValue == 11)
        {
            strcat(name, "B,");
        }
        else if (cbValue == 12)
        {
            strcat(name, "C,");
        }
        else if (cbValue == 13)
        {
            strcat(name, "D,");
        }
    }

    return name;
}
//////////////////////////////////////////////////////////////////////////
