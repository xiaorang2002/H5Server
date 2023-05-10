
#include "stdafx.h"
#include "ITableFrame.h"
// #include "acl_cpp/lib_acl.hpp"
#include <glog/logging.h>
#include "AndroidUserItemSink.h"
#include <muduo/base/Logging.h>
#include <random/RandomMT32.h>

//////////////////////////////////////////////////////////////////////////

//定时器ID
#define IDI_USER_EXCHANGE_SCORE         (101)                               //
#define IDI_USER_FIRE                   (102)								//下注定时器
#define IDI_USER_STRATEGY				(103)								//火拼定时器

//时间间隔
#define TIMER_INTERNAL_EXCHANGESCORE    (1)                                 //
#define TIMER_INTERNAL_USERFIRE			(2)									//下注时间
#define TIMER_INTERVAL_STRATEGY			(10)								//火拼

//////////////////////////////////////////////////////////////////////////

CAndroidUserItemSink::CAndroidUserItemSink()
	: m_pTableFrame(NULL)
	, m_wChairID(INVALID_CHAIR)
	, m_pAndroidUserItem(NULL)
{
    //接口变量
    srand((unsigned)time(NULL));
    m_wChairID      = INVALID_CHAIR;
    m_pTableFrame   = NULL;
    m_pAndroidUserItem = NULL;

    m_fGunAngle     = 0;
    m_lGunPower     = 0.01;

    m_lTotalGain    = 0;
    m_lTotalLost    = 0;

    m_lCurrGain     = 0;
    m_lCurrLost     = 0;

    m_nAndroidStatus = EFISHSTAT_FREE;

    LOG(ERROR) << "CAndroidUserItemSink::CAndroidUserItemSink()";
}

CAndroidUserItemSink::~CAndroidUserItemSink()
{
    LOG(ERROR) << "CAndroidUserItemSink::~CAndroidUserItemSink()";
}

bool CAndroidUserItemSink::RepositionSink()
{
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
    m_wChairID    = pAndroidUserItem->GetChairId();
    m_pAndroidUserItem = pAndroidUserItem;
}

bool CAndroidUserItemSink::OnTimerMessage(uint32_t dwTimerID, uint32_t dwParam)
{
    LOG_ERROR << " >>>>>>>> CAndroidUserItemSink::OnTimerMessage:" << dwTimerID << " , dwParam:" << dwParam;
    if (INVALID_CARD == m_wChairID) {   // ad by James.
        m_wChairID = m_pAndroidUserItem->GetChairId();
    }

    // switch timer id.
	switch (dwTimerID)
	{
        case IDI_USER_EXCHANGE_SCORE:
        {
            return OnUserExchangeScore();
        }
        break;
        case IDI_USER_FIRE:
        {
            //删除定时器
            return OnUserFire();
        }   break;
    }
//Cleanup:
    return true;
}


bool CAndroidUserItemSink::OnUserExchangeScore()
{
    LOG(ERROR) << "OnUserExchangeScore called.";
    DWORD dwUserID = 0;
    if (m_pAndroidUserItem) {
        dwUserID = m_pAndroidUserItem->GetUserId();
    }

    // try to set the special user item value.
    ::Fish::CMD_C_ExchangeScore exChangeScore;
    exChangeScore.set_userid(dwUserID);
    string content = exChangeScore.SerializeAsString();
    //m_pTableFrame->OnGameEvent(m_wChairID, Fish::SUB_C_EXCHANGE_SCORE, content.data(), content.size());

//Cleanup:
    return true;
}

// onuser
bool CAndroidUserItemSink::OnUserFire()
{
    static int bulletid = 0;
    LOG(ERROR) << "OnUserFire called.";

    ::Fish::CMD_C_UserFire UserFire;
    UserFire.set_lockedfishid(0);
    UserFire.set_cannonpower(1000);
    UserFire.set_angle(45);
    UserFire.set_bulletid(bulletid++);
    bulletid = bulletid % MAX_BULLETS;

    string content = UserFire.SerializeAsString();
    //m_pTableFrame->OnGameEvent(m_wChairID, Fish::SUB_C_USER_FIRE, content.data(), content.size());
//Cleanup:
    return true;
}


bool CAndroidUserItemSink::OnGameMessage(uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    LOG(ERROR) << "OnGameMessage wSubCmdID:" << (int)wSubCmdID;
    if (INVALID_CARD == m_wChairID) {   // ad by James.
        m_wChairID = m_pAndroidUserItem->GetChairId();
    }

    //switch commander.
	switch (wSubCmdID)
	{
        case SUB_S_GAME_SCENE:		//游戏开始
        {
            LOG(ERROR) << "CAndroidUserItemSink::OnGameMessage, SUB_S_GAME_START. status:" << m_pAndroidUserItem->GetUserStatus();
            return OnSubGameScene(pData, wDataSize);
        }   break;
        default:
            break;
	}

//Cleanup:
	return true;
}

//游戏开始
bool CAndroidUserItemSink::OnSubGameScene(const void * pBuffer, WORD wDataSize)
{
	//变量定义
    ::Fish::CMD_S_GameScene pGameScene;
    if (!pGameScene.ParseFromArray(pBuffer,wDataSize)) {
        return false;
    }

    // set the android timer.
    if (m_pAndroidUserItem) {
        //m_pAndroidUserItem->SetTimer(IDI_USER_EXCHANGE_SCORE,TIMER_INTERNAL_EXCHANGESCORE*1000);
        //m_pAndroidUserItem->SetTimer(IDI_USER_FIRE,TIMER_INTERNAL_USERFIRE*1000,REPEAT_INFINITE);
    }

    /*
	//用户信息
    m_wBankerUser  = pGameStart.wbankeruser();
    m_wCurrentUser = pGameStart.wcurrentuser();

	//加注信息
    m_bMingZhu = false;
	m_lUserMaxScore = 0;
    m_lCellScore    = (pGameStart.dcellscore());
    m_lCurrentTimes = (pGameStart.dcurrentjetton());
    m_lMaxCellScore =  m_lCellScore * MAX_JETTON_MULTIPLE/2;			//没看牌前最大倍数

   //  memcpy(m_cbPlayStatus, pGameStart->cbPlayStatus, sizeof(BYTE)*GAME_PLAYER);
    google::protobuf::RepeatedField<int> cbPlayerStatus =pGameStart.cbplaystatus();
    for (int i=0;i<GAME_PLAYER;i++)
    {
        m_cbPlayStatus[i] = cbPlayerStatus[i];
    }

	//用户状态
    for (WORD i = 0; i < GAME_PLAYER; i++)
	{
		//获取用户
		if (m_cbPlayStatus[i] != false)
		{
			m_lTableScore[i] = m_lCellScore;
		}
	}

	//玩家处理
    if (m_pAndroidUserItem->GetChairID() == pGameStart.wcurrentuser())
	{
		//设置定时器
        DWORD nElapse = rand() % TIME_USER_ADD_SCORE + TIME_LESS + 1; //开始游戏时下注给多一点时间
		m_pAndroidUserItem->SetTimer(IDI_USER_ADD_SCORE, nElapse*1000);
	}
    */

	return true;
}

