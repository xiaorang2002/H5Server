#include <glog/logging.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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
#include <functional>
#include <chrono>

#include "public/gameDefine.h"
#include "public/GlobalFunc.h"

#include "public/ITableFrame.h"
#include "public/ITableFrameSink.h"
#include "public/IServerUserItem.h"
#include "public/IAndroidUserItemSink.h"
#include "public/ISaveReplayRecord.h"

#include "public/Random.h"
#include "public/weights.h"
#include "public/StdRandom.h"
#include "public/types.h"
#include "public/INIHelp.h"
#include "public/PathMagic.h"
#include "public/SubNetIP.h"
// #include "public/pb2Json.h"

#include "proto/wxhh.Message.pb.h"

#include "GameLogic.h"

#include <muduo/base/Mutex.h>

#include "wxhhAlgorithm.h"

using namespace wxhh;

#include "CTableFrameSink.h"

#define MIN_LEFT_CARDS_RELOAD_NUM  133
#define MIN_PLAYING_ANDROID_USER_SCORE  0.0



// initialize glog.
class CGLogInit
{
public:
    CGLogInit()
    {
        FLAGS_colorlogtostderr = true;
        FLAGS_log_dir = "./log/wxhh/";
        FLAGS_logbufsecs = 3;
        if(!boost::filesystem::exists(FLAGS_log_dir))
        {
            boost::filesystem::create_directories(FLAGS_log_dir);
        }
        google::InitGoogleLogging("wxhh");
        // google::SetStderrLogging(google::GLOG_ERROR);
        google::SetStderrLogging(google::GLOG_INFO);
    }

    virtual ~CGLogInit()
    {
        google::ShutdownGoogleLogging();
    }
};

CGLogInit glog_init;

class RomMgr
{
public:
    RomMgr()
    {
        bisInit = false;

        if(access("./log/wxhh", 0) == -1)
           mkdir("./log/wxhh",0777);
    }

    virtual ~RomMgr()
    {
        google::ShutdownGoogleLogging();
    }

public:
    static RomMgr& Singleton()
    {
        static RomMgr* sp = 0;
        if (!sp)
        {
             sp = new RomMgr();
        }
        return (*sp);
    }
private:
     bool bisInit;
};



int64_t						m_lStockScore    = 0;			//库存
int64_t						m_lInScoreVal    = 10000;		//吸分值
int64_t						m_lOutScoreVal   = 20000;		//吐分值
int64_t						m_lStorageDeduct = 1;			//回扣变量


//////////////////////////////////////////////////////////////////////////

//MAX_CARDS = 54 * 8
uint8_t CTableFrameSink::m_cbTableCard[MAX_CARDS] =
{
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,	//方块 A - K
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,	//梅花 A - K
    0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,	//红桃 A - K
    0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,	//黑桃 A - K
    0x4E,0x4F,
};


//构造函数
CTableFrameSink::CTableFrameSink()// : m_TimerLoopThread(muduo::net::EventLoopThread::ThreadInitCallback(), "LongHuTimerEventLoopThread")
{
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));

    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));

//    memset(m_cbPlayStatus, 0, sizeof(m_cbPlayStatus));
//    memset(m_long, 0, sizeof(m_long));
//    memset(m_hu, 0, sizeof(m_hu));
//    memset(m_he, 0, sizeof(m_he));

    m_strRoundId = "";

    m_Hei.clear();
    m_Hong.clear();
    m_Mei.clear();
    m_Fang.clear();
    m_Wang.clear();

    memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));

    m_IsEnableOpenlog = false;
    m_IsXuYaoRobotXiaZhu = false;
    m_WinScoreSystem = 0;

    m_nPlaceJettonTime = 0;
//    m_nPlaceJettonLeveTime = 0;
    m_nGameTimeJettonBroadcast = 0;
    m_nGameEndTime = 0;
//    m_nGameEndLeveTime = 0;

    m_ShenSuanZiId = 0;

    m_UserInfo.clear();
    m_SortUserInfo.clear();

    m_openRecord.clear();



    m_CardVecData.clear();
    m_pGameRoomInfo = nullptr;
    m_dControlkill=0.0;
    m_lLimitPoints=0;
    stockWeak = 0.0;
    for(int i=0;i<Max;i++)
    {
        leftCards[i]=0;
    }
    m_testCards.clear();
    ReadConfigInformation();


	m_nWinIndex = 0;
    return;
}

//析构函数
CTableFrameSink::~CTableFrameSink(void)
{
    /*
    if( m_pServerContro )
    {
        delete m_pServerContro;
        m_pServerContro = NULL;
    }

    if( m_hControlInst )
    {
        FreeLibrary(m_hControlInst);
        m_hControlInst = NULL;
    }
    */
}

//读取配置
bool CTableFrameSink::ReadConfigInformation()
{
    //设置文件名
    //=====config=====
    if(!boost::filesystem::exists("./conf/wxhh_config.ini"))
    {
        LOG_INFO << "./conf/wxhh_config.ini not exists";
        return false;
    }

    boost::property_tree::ptree pt;
    boost::property_tree::read_ini("./conf/wxhh_config.ini", pt);

    stockWeak=  pt.get<double>("GAME_CONFIG.STOCK_WEAK", 1.0);
    //Time
    m_nPlaceJettonTime   = pt.get<int64_t>("GAME_CONFIG.TM_GAME_PLACEJET", 18);
    m_nGameEndTime       = pt.get<int64_t>("GAME_CONFIG.TM_GAME_END", 7);
    m_nGameTimeJettonBroadcast= pt.get<float>("GAME_CONFIG.TM_JETTON_BROADCAST", 1);



    m_lLimitPoints       = pt.get<int64_t>("GAME_CONFIG.LimitPoint", 500000);
    m_dControlkill       = pt.get<double>("GAME_CONFIG.Controlkill", 0.35);

    m_AreaMaxJetton[Hei]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.HEI",  1000000);
    m_AreaMaxJetton[Hong]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.HONG", 1000000);
    m_AreaMaxJetton[Mei]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.MEI",  1000000);
    m_AreaMaxJetton[Fang]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.FANG", 1000000);
    m_AreaMaxJetton[Wang]   = pt.get<int64_t>("GAME_AREA_MAX_JETTON.WANG", 1000000);

    m_IsXuYaoRobotXiaZhu = pt.get<uint32_t>("GAME_CONFIG.IsxuyaoRobotXiazhu", 0);
    m_IsEnableOpenlog    = pt.get<uint32_t>("GAME_CONFIG.IsEnableOpenlog", 0);
    m_WinScoreSystem     = pt.get<int64_t>("GAME_CONFIG.WinScore", 500);
    m_bOpenContorl       = pt.get<bool>("GAME_CONFIG.Control", true);

    m_isTest = pt.get<bool>("GAME_CONFIG.Test", false);

    vector<int64_t> scorelevel;
    vector<int64_t> chips;
    vector<int64_t> allchips;
    m_vmulticalList.clear();
    allchips.clear();
    scorelevel.clear();
    chips.clear();
    m_allChips.clear();
    useIntelligentChips = pt.get<bool>("CHIP_CONFIGURATION.useintelligentchips", 0);
    m_vmulticalList= to_array<double>(pt.get<string>("GAME_CONFIG.multicals","4,4,4,4,20"));

    m_HeiTimes             = m_vmulticalList[0]-1;
    m_HongTimes            = m_vmulticalList[1]-1;
    m_MeiTimes             = m_vmulticalList[2]-1;
    m_FangTimes            = m_vmulticalList[3]-1;
    m_WangTimes            = m_vmulticalList[4]-1;

    scorelevel = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.scorelevel","1,500000,2000000,5000000"));
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear1","100,200,500,1000,5000,10000"));
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        m_userChips[0][i]=chips[i];
        allchips.push_back(chips[i]);
        m_allChips.push_back(chips[i]);
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear2","100,200,500,1000,5000,10000"));
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        m_userChips[1][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
            m_allChips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear3","100,200,500,1000,5000,10000"));
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        m_userChips[2][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
            m_allChips.push_back(chips[i]);
        }
    }
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000,10000"));
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        m_userChips[3][i]=chips[i];
        vector<int64_t>::iterator it=find(allchips.begin(),allchips.end(),chips[i]);
        if(it!=allchips.end())
        {

        }
        else
        {
            allchips.push_back(chips[i]);
            m_allChips.push_back(chips[i]);
        }
    }

    for(int64_t chips : allchips)
    {
        m_Hei[chips] = 0;
        m_Hong[chips] = 0;
        m_Mei[chips] = 0;
        m_Fang[chips] = 0;
        m_Wang[chips] = 0;
    }

    for(int i=0;i<4;i++)
    {
      m_userScoreLevel[i]  = scorelevel[i];
    }
    //没打开就用大三组，默认组
    chips.clear();
    chips = to_array<int64_t>(pt.get<string>("CHIP_CONFIGURATION.chipgear4","100,200,500,1000,5000,10000"));
    if(!useIntelligentChips)
    {
        for(int i=0;i<CHIPS_SIZE;i++)
        {
            m_userChips[0][i]=chips[i];
            m_userChips[1][i]=chips[i];
            m_userChips[2][i]=chips[i];
            m_userChips[3][i]=chips[i];
        }
    }
}

bool CTableFrameSink::SetTableFrame(shared_ptr<ITableFrame>& pTableFrame)
{

    LOG(INFO) << "CTableFrameSink::SetTableFrame pTableFrame:" << pTableFrame;

    m_pITableFrame = pTableFrame;
    if (pTableFrame)
    {
        m_pGameRoomInfo = m_pITableFrame->GetGameRoomInfo();
        m_TimerLoopThread = m_pITableFrame->GetLoopThread();
        m_replay.cellscore = m_pGameRoomInfo->floorScore;
        m_replay.roomname = m_pGameRoomInfo->roomName;
		m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
        return true;
    }else
        return false;
}

//游戏开始
void CTableFrameSink::OnGameStart()
{

}

bool CTableFrameSink::CanJoinTable(shared_ptr<CServerUserItem>& pUser)
{
    return true;
}

bool CTableFrameSink::CanLeftTable(int64_t userId)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool canLeft = true;

    shared_ptr<IServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
        return true;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            totalxz = userInfo->m_lUserJettonScore[Hei]
                      + userInfo->m_lUserJettonScore[Hong]
                      + userInfo->m_lUserJettonScore[Mei]
                      + userInfo->m_lUserJettonScore[Fang]
                      + userInfo->m_lUserJettonScore[Wang];
        }

        if(totalxz > 0)
            canLeft = false;
    }

    return canLeft;
}

bool CTableFrameSink::OnUserEnter(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    shared_ptr<CServerUserItem> pIServerUserItem;
    pIServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL userId:"<<userId;
        return false;
    }
    LOG_DEBUG << "OnUserEnter userId:"<<userId;
    uint32_t chairId = pIServerUserItem->GetChairId();

    if(GAME_STATUS_INIT == m_pITableFrame->GetGameStatus())
    {
        m_ShenSuanZiId = pIServerUserItem->GetUserId();
//        openLog("11111shensunzi=%d",m_ShenSuanZiId);
//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
//        m_nGameEndLeveTime = m_nGameEndTime;
        m_startTime = chrono::system_clock::now();
        m_strRoundId = m_pITableFrame->GetNewRoundId();

        m_replay.gameinfoid = m_strRoundId;
        m_dwReadyTime = (uint32_t)time(NULL);
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_pITableFrame->SetGameStatus(GAME_STATUS_START);
//        openLog("m_pITableFrame->SetGameTimer(IDI_PLACE_JETTON, 1000, -1, 0L)");
        //if(!m_isTest)
        {
            m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);

//            int cardColor=0;
//            vector<uint8_t>::iterator it=m_CardVecData.begin();
//            int index=0;
//            for(;it!=m_CardVecData.end()&&index<297;index++)
//            {
//                cardColor=m_GameLogic.GetCardColor((*it));
//                Result result;
//                if(cardColor==Hei)
//                    result.wintag = Hei;
//                else if(cardColor==Hong)
//                    result.wintag = Hong;
//                else if(cardColor==Mei)
//                    result.wintag = Mei;
//                else if(cardColor==Fang)
//                    result.wintag = Fang;
//                else if(cardColor==Wang)
//                    result.wintag = Wang;
//                result.card=(*it);
//                it=m_CardVecData.erase(it);
//                m_openRecord.push_back(result);
//            }

        }


        random_shuffle(m_CardVecData.begin(), m_CardVecData.end());
        for(int64_t jetton : m_pGameRoomInfo->jettons)
        {
            m_Hei[jetton] = 0;
            m_Hong[jetton] = 0;
            m_Mei[jetton] = 0;
            m_Fang[jetton] = 0;
            m_Wang[jetton] = 0;
        }
    }
    for(int i=0;i<CHIPS_SIZE;i++)
    {

        if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[0]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[1])
        {
            m_currentchips[chairId][i]=m_userChips[0][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[1]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[2])
        {
            m_currentchips[chairId][i]=m_userChips[1][i];
        }
        else if(pIServerUserItem->GetUserScore()>= m_userScoreLevel[2]&&
                pIServerUserItem->GetUserScore()< m_userScoreLevel[3])
        {
            m_currentchips[chairId][i]=m_userChips[2][i];
        }
        else
        {
            m_currentchips[chairId][i]=m_userChips[3][i];
        }
    }
    auto it = m_UserInfo.find(userId);
    if(m_UserInfo.end() == it)
    {
        shared_ptr<UserInfo> userInfo(new UserInfo());
        userInfo->userId = userId;
        userInfo->chairId = chairId;
        userInfo->headerId = pIServerUserItem->GetHeaderId();
        userInfo->nickName = pIServerUserItem->GetNickName();
        userInfo->location = pIServerUserItem->GetLocation();

        m_UserInfo[userId] = userInfo;
    }
//    m_cbPlayStatus[chairId] = true;
    pIServerUserItem->SetUserStatus(sPlaying);

//    m_pITableFrame->BroadcastUserInfoToOther(pIServerUserItem);
//    m_pITableFrame->SendAllOtherUserInfoToUser(pIServerUserItem);
//    m_pITableFrame->BroadcastUserStatus(pIServerUserItem, true);

    OnEventGameScene(chairId, false);
    return true;
}

//发送场景
bool CTableFrameSink::OnEventGameScene(uint32_t chairId, bool bisLookon)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " OnUserEnter pIServerUserItem==NULL chairId:"<<chairId;
        return false;
    }

    uint8_t cbGameStatus = m_pITableFrame->GetGameStatus();
    int64_t userId = pIServerUserItem->GetUserId();

    LOG(INFO) << "CTableFrameSink::OnEventGameScene chairId:" << chairId << ", userId:" << userId << ", cbGameStatus:" << cbGameStatus;

    switch(cbGameStatus)
    {
        case GAME_STATUS_FREE:		//空闲状态
        {
            return true;
        }
        break;
        case GAME_STATUS_START:	//游戏状态
        {
            SendOpenRecord(chairId);
            SendStartMsg(chairId);
            return true;
        }
        break;
        case GAME_STATUS_END:
        {
            SendOpenRecord(chairId);
            SendEndMsg(chairId);
            return true;
        }
        break;
    }
    //效验错误
    assert(false);
    return false;
}

void CTableFrameSink::SendOpenRecord(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " SendOpenRecord pIServerUserItem==NULL chairId:"<<chairId;
        return;
    }

    uint8_t cbGameStatus = m_pITableFrame->GetGameStatus();
    uint32_t size = 0;
    if(cbGameStatus == GAME_STATUS_START)
    {
        size = m_openRecord.size();
    }else
    {
        if(m_openRecord.size() > 0)
            size = m_openRecord.size() - 1;
    }

    CMD_S_GameRecord record;
    uint32_t heiWin = 0;
    uint32_t hongWin = 0;
    uint32_t meiWin = 0;
    uint32_t fangWin = 0;
    uint32_t wangWin = 0;
    for(int i = 0; i < size; ++i)
    {
        record.add_record(m_openRecord[i].card);
        if(m_openRecord[i].wintag == Hei)
            heiWin++;
        else if(m_openRecord[i].wintag == Hong)
            hongWin++;
        else if(m_openRecord[i].wintag == Mei)
            meiWin++;
        else if(m_openRecord[i].wintag == Fang)
            fangWin++;
        else
            wangWin++;
    }

    record.set_heicount(heiWin);
    record.set_hongcount(hongWin);
    record.set_meicount(meiWin);
    record.set_fangcount(fangWin);
    record.set_wangcount(wangWin);

    std::string data = record.SerializeAsString();
    m_pITableFrame->SendTableData(chairId, wxhh::SUB_S_SEND_RECORD, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendStartMsg(uint32_t chairId)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_StartPlaceJetton pGamePlayScene;
    GameDeskInfo* pInfo = pGamePlayScene.mutable_deskdata();
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        pInfo->add_userchips(m_currentchips[chairId][i]);
    }
    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
        alljitton->set_ljettonarea(i);
    }

    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
        userjetton->set_dwuserid(userId);
        userjetton->set_ljettonarea(i);

        if(pIServerUserItem->IsAndroidUser())
            userjetton->set_ljettonscore(userInfo->m_lRobotUserJettonScore[i]);
        else
            userjetton->set_ljettonscore(userInfo->m_lUserJettonScore[i]);
    }

    //divineJettonScore
    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
        divineJetton->set_dwuserid(m_ShenSuanZiId);
        shared_ptr<IServerUserItem> pUserItem = m_pITableFrame->GetUserItemByUserId(m_ShenSuanZiId);
        if(pUserItem)
            divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
        else
            divineJetton->set_ljettonscore(0);
        divineJetton->set_ljettonarea(i);
    }

    pInfo->set_lapplybankercondition(0);
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);
    for(int i = 0; i < m_vmulticalList.size(); ++i)
    {
        if(i<4)
        {
            pInfo->add_muticallist(m_vmulticalList[3-i]);
        }
        else
        {
            pInfo->add_muticallist(m_vmulticalList[i]);
        }
    }
    for(int i = 0; i < 1; ++i)
    {
        CardGroup* card =pInfo->add_cards();
        if(m_openRecord.size() > 0)
            card->add_cards(m_openRecord[m_openRecord.size()-1].card);
        else
            card->add_cards(0);
        card->set_group_id(i);
        card->set_cardtype(0);
    }

   uint32_t i = 0;
   uint32_t size = 6;
   int64_t nowScore = 0;
   int64_t xzScore = 0;
   shared_ptr<IServerUserItem> pUserItem;
   shared_ptr<UserInfo> userInfoItem;
   for(auto &userInfoItem : m_SortUserInfo)
   {
       pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
       if(!pUserItem)
           nowScore = 0;
       else
       {
           xzScore = 0;
           if(pUserItem->IsAndroidUser())
               xzScore = userInfoItem->m_lRobotUserJettonScore[Hei]
                       + userInfoItem->m_lRobotUserJettonScore[Hong]
                       + userInfoItem->m_lRobotUserJettonScore[Mei]
                       + userInfoItem->m_lRobotUserJettonScore[Fang]
                       + userInfoItem->m_lRobotUserJettonScore[Wang];
           else
               xzScore = userInfoItem->m_lUserJettonScore[Hei]
                       + userInfoItem->m_lUserJettonScore[Hong]
                       + userInfoItem->m_lUserJettonScore[Mei]
                       + userInfoItem->m_lUserJettonScore[Fang]
                       + userInfoItem->m_lUserJettonScore[Wang];

            nowScore = pUserItem->GetUserScore()- xzScore;
       }

       PlayerListItem* palyer = pInfo->add_players();
       palyer->set_dwuserid(userInfoItem->userId);
       palyer->set_headerid(userInfoItem->headerId);
       palyer->set_nickname(userInfoItem->nickName);

       palyer->set_luserscore(nowScore);

       palyer->set_ltwentywinscore(userInfoItem->lTwentyWinScore);
       palyer->set_ltwentywincount(userInfoItem->lTwentyWinCount);
       int shensuanzi = userInfoItem->userId == m_ShenSuanZiId ? 1 : 0;
       palyer->set_isdivinemath(shensuanzi);
       palyer->set_index(i+1);
       if(shensuanzi == 1)
           palyer->set_index(0);

       palyer->set_isbanker(0);
       palyer->set_isapplybanker(0);
       palyer->set_applybankermulti(0);
       palyer->set_ljetton(0);
       palyer->set_szlocation(userInfoItem->location);

       if(++i > size)
           break;
   }

    pInfo->set_wbankerchairid(0);
    pInfo->set_cbbankertimes(0);
    pInfo->set_lbankerwinscore(0);
    pInfo->set_lbankerscore(0);
    pInfo->set_benablesysbanker(true);
    pInfo->set_szgameroomname("wxhh");
    if(m_openRecord.size() > 0)
    {
        pInfo->set_areawinscore(userInfo->dAreaWin[m_openRecord[m_openRecord.size()-1].wintag]+userInfo->m_lUserJettonScore[m_openRecord[m_openRecord.size()-1].wintag]);
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1].wintag);
    }
    else
    {
        pInfo->set_wintag(0);
        pInfo->set_areawinscore(0);
    }

    pInfo->set_wincardgrouptype(0);// add_wincardgrouptype(0);

    for(auto &jetton : m_Hei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Hei);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hong)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Hong);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Mei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Mei);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Fang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Fang);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Wang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Wang);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    pInfo->set_lonlinepopulation(m_UserInfo.size());

    pGamePlayScene.set_roundid(m_strRoundId);
    pGamePlayScene.set_cbplacetime(m_nPlaceJettonTime);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_startTime);
    int32_t leaveTime = m_nPlaceJettonTime - durationTime.count();
    pGamePlayScene.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    xzScore = 0;
    bool isRobot = pIServerUserItem->IsAndroidUser();
    if(isRobot)
        xzScore = userInfo->m_lRobotUserJettonScore[Hei]
                + userInfo->m_lRobotUserJettonScore[Hong]
                + userInfo->m_lRobotUserJettonScore[Mei]
                + userInfo->m_lRobotUserJettonScore[Fang]
                + userInfo->m_lRobotUserJettonScore[Wang];
    else
        xzScore = userInfo->m_lUserJettonScore[Hei]
                + userInfo->m_lUserJettonScore[Hong]
                + userInfo->m_lUserJettonScore[Mei]
                + userInfo->m_lUserJettonScore[Fang]
                + userInfo->m_lUserJettonScore[Wang];


    nowScore = pIServerUserItem->GetUserScore()-xzScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    pGamePlayScene.set_lusermaxscore(maxscore);
    pGamePlayScene.set_luserscore(nowScore);

    pGamePlayScene.set_verificationcode(0);

    std::string data = pGamePlayScene.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), wxhh::SUB_S_START_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
}

void CTableFrameSink::SendEndMsg(uint32_t wChairID)
{
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    CMD_S_GameEnd gamend;
    GameDeskInfo* pInfo = gamend.mutable_deskdata();
    for(int i=0;i<CHIPS_SIZE;i++)
    {
        pInfo->add_userchips(m_currentchips[wChairID][i]);
    }
    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* alljitton = pInfo->add_lalljettonscore();
        alljitton->set_dwuserid(0);
        alljitton->set_ljettonarea(i);
        alljitton->set_ljettonscore(m_lAllJettonScore[i]);
    }

    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* userjetton = pInfo->add_selfjettonscore();
        userjetton->set_dwuserid(userId);
        userjetton->set_ljettonarea(i);

        if(pIServerUserItem->IsAndroidUser())
            userjetton->set_ljettonscore(userInfo->m_lRobotUserJettonScore[i]);
        else
            userjetton->set_ljettonscore(userInfo->m_lUserJettonScore[i]);
    }
     //divineJettonScore
    for(int i = 0; i < Max; ++i)
    {
        PlaceJettonScore* divineJetton = pInfo->add_divinejettonscore();
        divineJetton->set_dwuserid(m_ShenSuanZiId);
        shared_ptr<IServerUserItem> pUserItem;
        pUserItem = m_pITableFrame->GetUserItemByUserId(m_ShenSuanZiId);
        if(pUserItem)
            divineJetton->set_ljettonscore(m_lShenSuanZiJettonScore[i]);
        else
            divineJetton->set_ljettonscore(0);
        divineJetton->set_ljettonarea(i);
    }

    pInfo->set_lapplybankercondition(0);
    pInfo->set_larealimitscore(m_pGameRoomInfo->maxJettonScore);
    for(int i = 0; i < m_vmulticalList.size(); ++i)
    {
        if(i<4)
        {
            pInfo->add_muticallist(m_vmulticalList[3-i]);
        }
        else
        {
            pInfo->add_muticallist(m_vmulticalList[i]);
        }

       // LOG(INFO) << "倍数列表["<< i<<"]===="<<m_vmulticalList[i] ;
    }
    for(int i = 0; i < 1; ++i)
    {
        CardGroup* card = pInfo->add_cards();
        if(m_openRecord.size() > 0)
            card->add_cards(m_openRecord[m_openRecord.size()-1].card);
        else
            card->add_cards(0);
        card->set_group_id(i);
        card->set_cardtype(0);
    }

    bool isVIP = false;
    uint32_t i = 0;
    uint32_t size = 6;
    int64_t nowScore = 0;
//    int64_t xzScore = 0;
    shared_ptr<IServerUserItem> pUserItem;
    for(auto &userInfoItem : m_SortUserInfo)
    {
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            nowScore = 0;
        else
            nowScore = pUserItem->GetUserScore() + userInfoItem->m_EndWinScore;

        PlayerListItem* palyer = pInfo->add_players();
        palyer->set_dwuserid(userInfoItem->userId);
        palyer->set_headerid(userInfoItem->headerId);
        palyer->set_nickname(userInfoItem->nickName);

        palyer->set_luserscore(nowScore);

        palyer->set_ltwentywinscore(userInfoItem->lTwentyWinScore);
        palyer->set_ltwentywincount(userInfoItem->lTwentyWinCount);
        int shensuanzi = userInfoItem->userId == m_ShenSuanZiId ? 1 : 0;
        palyer->set_isdivinemath(shensuanzi);
        palyer->set_index(i);
        if(shensuanzi == 1)
            palyer->set_index(0);
        palyer->set_isbanker(0);
        palyer->set_isapplybanker(0);
        palyer->set_applybankermulti(0);
        palyer->set_ljetton(0);
        palyer->set_szlocation(userInfoItem->location);


        PlayerScoreValue * score = gamend.add_gameendscore();
        score->set_userid(userInfoItem->userId);
        score->set_returnscore(userInfoItem->m_EndWinScore);
        score->set_userscore(nowScore);

        if(userId == userInfoItem->userId) //vip
            isVIP = true;

        if(++i > size)
            break;
    }

    pInfo->set_wbankerchairid(0);
    pInfo->set_cbbankertimes(0);
    pInfo->set_lbankerwinscore(0);
    pInfo->set_lbankerscore(0);
    pInfo->set_benablesysbanker(true);
    pInfo->set_szgameroomname("wxhh");

    if(m_openRecord.size() > 0)
    {
        pInfo->set_areawinscore(userInfo->dAreaWin[m_openRecord[m_openRecord.size()-1].wintag]+userInfo->m_lUserJettonScore[m_openRecord[m_openRecord.size()-1].wintag]);
        pInfo->set_wintag(m_openRecord[m_openRecord.size()-1].wintag);
    }
    else
    {
        pInfo->set_wintag(0);
        pInfo->set_areawinscore(0);
    }


    pInfo->set_wincardgrouptype(0);

    for(auto &jetton : m_Hei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Hei);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Hong)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Hong);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Mei)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Mei);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Fang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Fang);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    for(auto &jetton : m_Wang)
    {
        PlaceJetInfo* jetinfo = pInfo->add_jetinfo();
        jetinfo->set_ljettonarea(Wang);
        jetinfo->set_ljettontype(jetton.first);
        jetinfo->set_ljettoncount(jetton.second);
    }
    pInfo->set_lonlinepopulation(m_UserInfo.size());


    gamend.set_roundid(m_strRoundId);
    chrono::seconds durationTime = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - m_endTime);
    int32_t leaveTime = m_nGameEndTime - durationTime.count();
    gamend.set_cbtimeleave(leaveTime > 0 ? leaveTime : 0);

    nowScore = pIServerUserItem->GetUserScore() + userInfo->m_EndWinScore;
    int64_t maxscore = nowScore >= 50 ? nowScore : 0;
    gamend.set_lusermaxscore(maxscore);
    gamend.set_luserscore(nowScore);
    gamend.set_lrevenue(userInfo->m_Revenue);
    gamend.set_verificationcode(0);

    LOG_DEBUG << "Revenue:"<<userInfo->m_Revenue;

    if(!isVIP)  //myself
    {
        PlayerScoreValue* score = gamend.add_gameendscore();
        score->set_userid(userId);
        score->set_returnscore(userInfo->m_EndWinScore);
        score->set_userscore(nowScore);
    }

    string data = gamend.SerializeAsString();
    m_pITableFrame->SendTableData(pIServerUserItem->GetChairId(), wxhh::SUB_S_GAME_END, (uint8_t*)data.c_str(), data.size());
}

//用户起立
bool CTableFrameSink::OnUserLeft(int64_t userId, bool isLookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    bool bClear = false;

    shared_ptr<CServerUserItem> pServerUserItem;
    pServerUserItem = m_pITableFrame->GetUserItemByUserId(userId);
    if(!pServerUserItem)
    {
        LOG_DEBUG << " pIServerUserItem==NULL Left UserId:"<<userId;
    }else
    {
        shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
        uint32_t chairId = userInfo->chairId;

        int64_t totalxz = 0;
        uint8_t status = m_pITableFrame->GetGameStatus();
        if(GAME_STATUS_START == status)
        {
            totalxz = userInfo->m_lUserJettonScore[0]+userInfo->m_lUserJettonScore[4]+userInfo->m_lUserJettonScore[1] + userInfo->m_lUserJettonScore[2] + userInfo->m_lUserJettonScore[3];
        }

        pServerUserItem->setOffline();
        if(totalxz == 0)
        {
            {
//                WRITE_LOCK(m_mutex);
                m_UserInfo.erase(userId);
            }
            m_pITableFrame->ClearTableUser(chairId);
            bClear = true;
        }else
        {
            pServerUserItem->SetTrustship(true);
        }

//        m_cbPlayStatus[chairId] = false;
    }

    if(userId == m_ShenSuanZiId)
    {
        m_ShenSuanZiId = 0;
    }

    return bClear;
}

bool CTableFrameSink::isInVipList(uint32_t chairId)
{
    uint32_t listNum = 0;
    for(auto userInfo : m_SortUserInfo)
    {
        if(userInfo->chairId == chairId)
        {
            return true;
        }
       if(++listNum >= 8)
            break;
    }
    return false;
}

bool CTableFrameSink::OnUserReady(int64_t userId, bool islookon)
{
    LOG_DEBUG << __FILE__ << " " << __FUNCTION__;

    return true;
}

//根据系统输赢发牌
void CTableFrameSink::GetOpenCards()
{

    float playerMuiltcle[Max]={3.0,3.0,3.0,3.0,19.0};
    playerMuiltcle[Hei]=m_HeiTimes;
    playerMuiltcle[Hong]=m_HongTimes;
    playerMuiltcle[Mei]=m_MeiTimes;
    playerMuiltcle[Fang]=m_FangTimes;
    playerMuiltcle[Wang]=m_WangTimes;

    tagStorageInfo storageInfo;
    int8_t winFlag = 1;//1-hei 2-hong 3-mei 4-fang 5-wang
    if(true)
    {
        int  isHaveBlackPlayer=0;
        for(int i=0;i<GAME_PLAYER;i++)
        {
            shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
            if(!user||!user->GetBlacklistInfo().status||user->IsAndroidUser())
            {
                continue;
            }
            int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[0]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[1]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[2]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[3]
                    +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[4];
            if(allJetton==0) continue;
            if(user->GetBlacklistInfo().listRoom.size()>0)
            {
                auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                if(it != user->GetBlacklistInfo().listRoom.end())
                {
                    if(it->second>0)
                    {
                        isHaveBlackPlayer=it->second;//
                    }

                    openLog (" 黑名单userId:%d  房间:%d  控制系数:%d", user->GetUserId(),m_pITableFrame->GetGameRoomInfo()->gameId,isHaveBlackPlayer);
                    openLog (" 控制状态:%d  当前已经杀的分数:%d  要求杀的分数:%d", user->GetBlacklistInfo().status,user->GetBlacklistInfo().current,user->GetBlacklistInfo().total);
                }
            }


        }
        //难度干涉值
        //m_difficulty
        int32_t randomRet=m_random.betweenInt(0,1000).randInt_mt(true);
        m_difficulty =m_pITableFrame->GetGameRoomInfo()->systemKillAllRatio;
        LOG(ERROR)<<" system difficulty "<<m_difficulty;
        //难度值命中概率,全局难度,命中概率系统必须赚钱
        if(randomRet<m_difficulty)
        {
            m_pITableFrame->GetStorageScore(storageInfo);

            m_WXHHAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
            ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值

            for(int i=0;i<Max;i++)
             m_WXHHAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值

            m_WXHHAlgorithm.SetMustKill(-1);
            m_WXHHAlgorithm.GetOpenCard(winFlag,leftCards[4]);//后面的参数是判断有没有王牌
        }
        else
        {
            //存在黑名单控制名单
            if(isHaveBlackPlayer)
            {
                float probilityAll[Max]={0.0f};
                //user probilty count
                for(int i=0;i<GAME_PLAYER;i++)
                {
                    shared_ptr<CServerUserItem> user=m_pITableFrame->GetTableUserItem(i);
                    if(!user||user->IsAndroidUser())
                    {
                        continue;
                    }
                    int64_t allJetton=m_UserInfo[user->GetUserId()]->m_lUserJettonScore[Hei]
                            +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[Hong]
                            +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[Mei]
                            +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[Fang]
                            +m_UserInfo[user->GetUserId()]->m_lUserJettonScore[Wang];

                    if(allJetton==0) continue;

                    if(user->GetBlacklistInfo().status == 1 )
                    {
                        int shuzhi=0;
                        auto it=user->GetBlacklistInfo().listRoom.find(to_string(m_pITableFrame->GetGameRoomInfo()->gameId));
                        if(it != user->GetBlacklistInfo().listRoom.end())
                        {
                            shuzhi=it->second;
                        }
                        else
                        {
                            shuzhi=0;
                        }
                        float gailv=(float)shuzhi/1000;
                       for(int j=0;j<MAX_COUNT;j++)
                       {
                           if(m_lTrueUserJettonScore[j]>0)
                           {
                               probilityAll[j]+=(gailv*(float)m_UserInfo[user->GetUserId()]->m_lUserJettonScore[j])/((float)m_lTrueUserJettonScore[0]+(float)m_lTrueUserJettonScore[1]+(float)m_lTrueUserJettonScore[2]+(float)m_lTrueUserJettonScore[3]);
                           }

                       }
                    }
                }

                m_pITableFrame->GetStorageScore(storageInfo);

                 m_WXHHAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
                ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值

                for(int i=0;i<5;i++)
                 m_WXHHAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值
                 m_WXHHAlgorithm.BlackGetOpenCard(winFlag,probilityAll,leftCards[4]);//后面的参数是判断有没有王牌
            }
            else
            {


                m_pITableFrame->GetStorageScore(storageInfo);

                m_WXHHAlgorithm.SetAlgorithemNum(storageInfo.lUplimit,storageInfo.lLowlimit, storageInfo.lEndStorage
                ,m_dControlkill,playerMuiltcle, m_lLimitPoints);//设置上下限,当前利润,干预难度,超红判断值

                for(int i=0;i<Max;i++)
                 m_WXHHAlgorithm.SetBetting(i,m_lTrueUserJettonScore[i]);//设置每门真人押注值
                m_WXHHAlgorithm.GetOpenCard(winFlag,leftCards[4]);//后面的参数是判断有没有王牌
            }

        }

    }
    double res=0.0;
    res = m_WXHHAlgorithm.CurrentProfit();
    if(res>0)
    {
        res = res*(1000.0-m_pITableFrame->GetGameRoomInfo()->systemReduceRatio)/1000.0;
    }
    m_pITableFrame->UpdateStorageScore(res);
    m_pITableFrame->GetStorageScore(storageInfo);
    if(winFlag==Hei)
    {
        openLog("本局开奖结果过是：♠");
    }
    else if(winFlag==Hong)
    {
        openLog("本局开奖结果过是：♥");
    }
    else if(winFlag==Mei)
    {
        openLog("本局开奖结果过是：♣");
    }
    else if(winFlag==Fang)
    {
        openLog("本局开奖结果过是：♦");
    }
    else
    {
        openLog("本局开奖结果过是：🃏");
    }
    openLog("本次系统赢分是：    %d",m_WXHHAlgorithm.CurrentProfit());
    openLog("系统库存上限是：    %ld",storageInfo.lUplimit);
    openLog("系统库存下限是：    %ld",storageInfo.lLowlimit);
    openLog("系统当前库存是：    %ld",storageInfo.lEndStorage);
    openLog("黑桃真人下注：    %d",m_lTrueUserJettonScore[Hei]);
    openLog("红桃真人下注：    %d",m_lTrueUserJettonScore[Hong]);
    openLog("梅花真人下注：    %d",m_lTrueUserJettonScore[Mei]);
    openLog("方块真人下注：    %d",m_lTrueUserJettonScore[Fang]);
    openLog("大王真人下注：    %d",m_lTrueUserJettonScore[Wang]);
    if(winFlag >= 0 && winFlag <= 4)
    LOG_DEBUG << "......winFlag:"<< winFlag <<  "......";

    m_cbCurrentRoundTableCards[0] = 0;
//    memset(m_EndWinScore, 0, sizeof(m_EndWinScore));

     //设置扑克

    switch(winFlag)
    {
        case Hei:
            do
            {
                int carIndex = -1;
                int pkSize = m_CardVecData.size();
                carIndex = rand() % pkSize;
                uint8_t openCardType = m_CardVecData[carIndex];
                uint8_t cardValue = m_GameLogic.GetCardValue(openCardType);
                uint8_t cardColor = m_GameLogic.GetCardColor(openCardType);

                if(cardValue!=0x0E&&cardValue!=0x0F&&cardColor==Hei)
                {
                    m_cbCurrentRoundTableCards[0] = openCardType;
                    m_CardVecData.erase(m_CardVecData.begin() + carIndex);

                    openLog("按算法开出的牌是:    %d  牌点数是：%d    花色是：%d",m_cbCurrentRoundTableCards[0],cardValue,cardColor);
                    break;
                }
            }while(true);
            break;
        case Hong:
            do
            {
                int carIndex = -1;
                int pkSize = m_CardVecData.size();
                carIndex = rand() % pkSize;
                uint8_t openCardType = m_CardVecData[carIndex];
                uint8_t cardValue = m_GameLogic.GetCardValue(openCardType);
                uint8_t cardColor = m_GameLogic.GetCardColor(openCardType);

                if(cardValue!=0x0E&&cardValue!=0x0F&&cardColor==Hong)
                {
                    m_cbCurrentRoundTableCards[0] = openCardType;
                    m_CardVecData.erase(m_CardVecData.begin() + carIndex);
                    openLog("按算法开出的牌是:    %d  牌点数是：%d    花色是：%d",m_cbCurrentRoundTableCards[0],cardValue,cardColor);
                    break;
                }
            }while(true);
            break;
        case Mei:
            do
            {
                int carIndex = -1;
                int pkSize = m_CardVecData.size();
                carIndex = rand() % pkSize;
                uint8_t openCardType = m_CardVecData[carIndex];
                uint8_t cardValue = m_GameLogic.GetCardValue(openCardType);
                uint8_t cardColor = m_GameLogic.GetCardColor(openCardType);

                if(cardValue!=0x0E&&cardValue!=0x0F&&cardColor==Mei)
                {
                    m_cbCurrentRoundTableCards[0] = openCardType;
                    m_CardVecData.erase(m_CardVecData.begin() + carIndex);
                    openLog("按算法开出的牌是:    %d  牌点数是：%d    花色是：%d",m_cbCurrentRoundTableCards[0],cardValue,cardColor);
                    break;
                }
            }while(true);
            break;
        case Fang:
            do
            {
                int carIndex = -1;
                int pkSize = m_CardVecData.size();
                carIndex = rand() % pkSize;
                uint8_t openCardType = m_CardVecData[carIndex];
                uint8_t cardValue = m_GameLogic.GetCardValue(openCardType);
                uint8_t cardColor = m_GameLogic.GetCardColor(openCardType);

                if(cardValue!=0x0E&&cardValue!=0x0F&&cardColor==Fang)
                {
                    m_cbCurrentRoundTableCards[0] = openCardType;
                    m_CardVecData.erase(m_CardVecData.begin() + carIndex);
                    openLog("按算法开出的牌是:    %d  牌点数是：%d    花色是：%d",m_cbCurrentRoundTableCards[0],cardValue,cardColor);
                    break;
                }
            }while(true);
            break;
        case Wang:
            do
            {
                int carIndex = -1;
                int pkSize = m_CardVecData.size();
                carIndex = rand() % pkSize;
                uint8_t openCardType = m_CardVecData[carIndex];
                uint8_t cardValue = m_GameLogic.GetCardValue(openCardType);
                uint8_t cardColor = m_GameLogic.GetCardColor(openCardType);

                if((cardValue==0x0E||cardValue==0x0F)&&cardColor==Wang)
                {
                    m_cbCurrentRoundTableCards[0] = openCardType;
                    m_CardVecData.erase(m_CardVecData.begin() + carIndex);
                    openLog("按算法开出的牌是:    %d  牌点数是：%d    花色是：%d",m_cbCurrentRoundTableCards[0],cardValue,cardColor);
                    break;
                }
            }while(true);
            break;
        default:
            LOG_DEBUG << "......WinFlag:"<<winFlag<<" Error!";
    }

    if(m_isTest)
    {
        if(m_testCards.size()==0)
        {
            for(int i=0;i<54;i++)
            {
                m_testCards.push_back(m_cbTableCard[i]);
            }
        }
        m_cbCurrentRoundTableCards[0]=m_testCards[0];
        m_testCards.erase(m_testCards.begin());
    }
}


//游戏结束
bool CTableFrameSink::OnEventGameConclude(uint32_t chairId, uint8_t cbReason)
{
    LOG(INFO) << "CTableFrameSink::OnEventGameConclude chairId:" << chairId << ", cbReason:" << (int)cbReason;
//    shared_ptr<CServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(chairId);
//    int64_t userId = pIServerUserItem->GetUserId();
//    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];
    switch(cbReason)
    {
    case GER_NORMAL:		//正常结束
        GetOpenCards();     //open cards
        uint8_t cardColor = m_GameLogic.GetCardColor(m_cbCurrentRoundTableCards[0]);
        uint8_t cardValue = m_GameLogic.GetCardValue(m_cbCurrentRoundTableCards[0]);
        openLog ("m_GameLogic cardValue:%d",m_cbCurrentRoundTableCards[0] );
        uint8_t carvalue=m_cbCurrentRoundTableCards[0];
        m_replay.addStep((uint32_t)time(NULL)-m_dwReadyTime,GlobalFunc::converToHex(&carvalue, 1),-1,opShowCard,-1,-1);

        string CardName = GetCardValueName(0);

         openLog ("m_GameLogic cardValue:%d    cardColor:%d    CardName:%s",cardValue,cardColor,CardName.c_str());
        LOG_DEBUG << "Open Card LongCard:"<<CardName;

        //结算
        if(cardValue!=14&&cardValue!=15&&cardColor==Hei)//黑桃
        {
            LOG_DEBUG<<"......Heitao Win......";
            m_nWinIndex = Hei;
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lRobotUserJettonScore[Hei] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[Hei] * m_HeiTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[Hei]*m_HeiTimes );
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hong] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hong];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Mei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Fang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Wang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Wang];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
                else   //if(!pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lUserJettonScore[Hei] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[Hei] * m_HeiTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[Hei] * m_HeiTimes );
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,Hei,userInfoItem->m_lUserJettonScore[Hei],(userInfoItem->m_lUserJettonScore[Hei]*m_HeiTimes ),"",false);
                        userInfoItem->dAreaWin[Hei] = (userInfoItem->m_lUserJettonScore[Hei] * m_HeiTimes );
                    }

                    if(userInfoItem->m_lUserJettonScore[Hong]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hong];
                        m_replay.addResult(userInfoItem->chairId,Hong,userInfoItem->m_lUserJettonScore[Hong],-userInfoItem->m_lUserJettonScore[Hong],"",false);
                        userInfoItem->dAreaWin[Hong] = -userInfoItem->m_lUserJettonScore[Hong];
                    }

                    if(userInfoItem->m_lUserJettonScore[Mei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Mei];
                        m_replay.addResult(userInfoItem->chairId,Mei,userInfoItem->m_lUserJettonScore[Mei],-userInfoItem->m_lUserJettonScore[Mei],"",false);
                        userInfoItem->dAreaWin[Mei] = -userInfoItem->m_lUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lUserJettonScore[Fang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Fang];
                        m_replay.addResult(userInfoItem->chairId,Fang,userInfoItem->m_lUserJettonScore[Fang],-userInfoItem->m_lUserJettonScore[Fang],"",false);
                        userInfoItem->dAreaWin[Fang] = -userInfoItem->m_lUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lUserJettonScore[Wang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Wang];
                        m_replay.addResult(userInfoItem->chairId,Wang,userInfoItem->m_lUserJettonScore[Wang],-userInfoItem->m_lUserJettonScore[Wang],"",false);
                        userInfoItem->dAreaWin[Wang] = -userInfoItem->m_lUserJettonScore[Wang];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }
        if(cardValue!=14&&cardValue!=15&&cardColor==Hong)//红桃
        {
            LOG_DEBUG<<"......hongtao Win......";
            m_nWinIndex = Hong;
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lRobotUserJettonScore[Hong] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[Hong] * m_HongTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[Hong]*m_HongTimes );
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Mei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Fang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Wang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Wang];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
                else   //if(!pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lUserJettonScore[Hong] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[Hong] * m_HongTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[Hong] * m_HongTimes );
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,Hong,userInfoItem->m_lUserJettonScore[Hong],(userInfoItem->m_lUserJettonScore[Hong]*m_HongTimes ),"",false);
                        userInfoItem->dAreaWin[Hong] = (userInfoItem->m_lUserJettonScore[Hong] * m_HongTimes );
                    }

                    if(userInfoItem->m_lUserJettonScore[Hei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hei];
                        m_replay.addResult(userInfoItem->chairId,Hei,userInfoItem->m_lUserJettonScore[Hei],-userInfoItem->m_lUserJettonScore[Hei],"",false);
                        userInfoItem->dAreaWin[Hei] = -userInfoItem->m_lUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lUserJettonScore[Mei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Mei];
                        m_replay.addResult(userInfoItem->chairId,Mei,userInfoItem->m_lUserJettonScore[Mei],-userInfoItem->m_lUserJettonScore[Mei],"",false);
                        userInfoItem->dAreaWin[Mei] = -userInfoItem->m_lUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lUserJettonScore[Fang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Fang];
                        m_replay.addResult(userInfoItem->chairId,Fang,userInfoItem->m_lUserJettonScore[Fang],-userInfoItem->m_lUserJettonScore[Fang],"",false);
                        userInfoItem->dAreaWin[Fang] = -userInfoItem->m_lUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lUserJettonScore[Wang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Wang];
                        m_replay.addResult(userInfoItem->chairId,Wang,userInfoItem->m_lUserJettonScore[Wang],-userInfoItem->m_lUserJettonScore[Wang],"",false);
                        userInfoItem->dAreaWin[Wang] = -userInfoItem->m_lUserJettonScore[Wang];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }
        if(cardValue!=14&&cardValue!=15&&cardColor==Mei)//梅花
        {
            LOG_DEBUG<<"......hongtao Win......";
            m_nWinIndex = Mei;
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lRobotUserJettonScore[Mei] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[Mei] * m_MeiTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[Mei]*m_MeiTimes );
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hong] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hong];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Fang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Wang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Wang];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
                else   //if(!pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lUserJettonScore[Mei] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[Mei] * m_MeiTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[Mei] * m_MeiTimes );
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,Mei,userInfoItem->m_lUserJettonScore[Mei],(userInfoItem->m_lUserJettonScore[Mei]*m_MeiTimes ),"",false);
                        userInfoItem->dAreaWin[Mei] = (userInfoItem->m_lUserJettonScore[Mei] * m_MeiTimes );
                    }

                    if(userInfoItem->m_lUserJettonScore[Hei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hei];
                        m_replay.addResult(userInfoItem->chairId,Hei,userInfoItem->m_lUserJettonScore[Hei],-userInfoItem->m_lUserJettonScore[Hei],"",false);
                        userInfoItem->dAreaWin[Hei] = -userInfoItem->m_lUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lUserJettonScore[Hong]>Hong)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hong];
                        m_replay.addResult(userInfoItem->chairId,Hong,userInfoItem->m_lUserJettonScore[Hong],-userInfoItem->m_lUserJettonScore[Hong],"",false);
                        userInfoItem->dAreaWin[Hong] = -userInfoItem->m_lUserJettonScore[Hong];
                    }
                    if(userInfoItem->m_lUserJettonScore[Fang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Fang];
                        m_replay.addResult(userInfoItem->chairId,Fang,userInfoItem->m_lUserJettonScore[Fang],-userInfoItem->m_lUserJettonScore[Fang],"",false);
                        userInfoItem->dAreaWin[Fang] = -userInfoItem->m_lUserJettonScore[Fang];
                    }
                    if(userInfoItem->m_lUserJettonScore[Wang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Wang];
                        m_replay.addResult(userInfoItem->chairId,Wang,userInfoItem->m_lUserJettonScore[Wang],-userInfoItem->m_lUserJettonScore[Wang],"",false);
                        userInfoItem->dAreaWin[Wang] = -userInfoItem->m_lUserJettonScore[Wang];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }
        if(cardValue!=14&&cardValue!=15&&cardColor==Fang)//方块
        {
            LOG_DEBUG<<"......hongtao Win......";
            m_nWinIndex = Fang;
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lRobotUserJettonScore[Fang] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[Fang] * m_FangTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[Fang]*m_FangTimes );
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lRobotUserJettonScore[Hong] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Hong];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Mei] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lRobotUserJettonScore[Wang] > 0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lRobotUserJettonScore[Wang];
                    }
                    LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
                else   //if(!pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lUserJettonScore[Fang] > 0)
                    {
                        //税收
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[Fang] * m_FangTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[Fang] * m_FangTimes );
                        LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                        m_replay.addResult(userInfoItem->chairId,Fang,userInfoItem->m_lUserJettonScore[Fang],(userInfoItem->m_lUserJettonScore[Fang]*m_FangTimes ),"",false);
                        userInfoItem->dAreaWin[Fang] = (userInfoItem->m_lUserJettonScore[Fang] * m_FangTimes );
                    }

                    if(userInfoItem->m_lUserJettonScore[Hei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hei];
                        m_replay.addResult(userInfoItem->chairId,Hei,userInfoItem->m_lUserJettonScore[Hei],-userInfoItem->m_lUserJettonScore[Hei],"",false);
                        userInfoItem->dAreaWin[Hei] = -userInfoItem->m_lUserJettonScore[Hei];
                    }

                    if(userInfoItem->m_lUserJettonScore[Hong]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Hong];
                        m_replay.addResult(userInfoItem->chairId,Hong,userInfoItem->m_lUserJettonScore[Hong],-userInfoItem->m_lUserJettonScore[Hong],"",false);
                        userInfoItem->dAreaWin[Hong] = -userInfoItem->m_lUserJettonScore[Hong];
                    }
                    if(userInfoItem->m_lUserJettonScore[Mei]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Mei];
                        m_replay.addResult(userInfoItem->chairId,Mei,userInfoItem->m_lUserJettonScore[Mei],-userInfoItem->m_lUserJettonScore[Mei],"",false);
                        userInfoItem->dAreaWin[Mei] = -userInfoItem->m_lUserJettonScore[Mei];
                    }
                    if(userInfoItem->m_lUserJettonScore[Wang]>0)
                    {
                        userInfoItem->m_EndWinScore -= userInfoItem->m_lUserJettonScore[Wang];
                        m_replay.addResult(userInfoItem->chairId,Wang,userInfoItem->m_lUserJettonScore[Wang],-userInfoItem->m_lUserJettonScore[Wang],"",false);
                        userInfoItem->dAreaWin[Wang] = -userInfoItem->m_lUserJettonScore[Wang];
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }
        if((cardValue==14||cardValue==15)&&cardColor==Wang)//大王
        {
            LOG_DEBUG<<"......He Win......";
            m_nWinIndex = Wang;
            shared_ptr<IServerUserItem> pUserItem;
            shared_ptr<UserInfo> userInfoItem;
            for(auto &it : m_UserInfo)
            {
                userInfoItem = it.second;
                pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
                if(!pUserItem)
                    continue;

                if(pUserItem->IsAndroidUser())
                {
                    if(userInfoItem->m_lRobotUserJettonScore[Wang] > 0)
                    {
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lRobotUserJettonScore[Wang] * m_WangTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lRobotUserJettonScore[Wang] * m_WangTimes );
                        LOG_DEBUG << " robot Id:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue;
                    }
                }else
                {
                    if(userInfoItem->m_lUserJettonScore[Wang] >= 0)
                    {
                        //userInfoItem->m_Revenue = m_pITableFrame->CalculateRevenue(userInfoItem->m_lUserJettonScore[Wang] * m_WangTimes);
                        userInfoItem->m_EndWinScore += (userInfoItem->m_lUserJettonScore[Wang] * m_WangTimes );
                        m_replay.addResult(userInfoItem->chairId,Wang,userInfoItem->m_lUserJettonScore[Wang],(userInfoItem->m_lUserJettonScore[Wang]*m_WangTimes ),"",false);
                        userInfoItem->dAreaWin[Wang] = (userInfoItem->m_lUserJettonScore[Wang] * m_WangTimes );
                    }
                    LOG_DEBUG << "userId:"<<userInfoItem->userId<<" SystemTax:"<<userInfoItem->m_Revenue<<" TotalWin:"<<userInfoItem->m_EndWinScore;
                }
            }
        }

        Result result;
        if(cardColor==Hei)
            result.wintag = Hei;
        else if(cardColor==Hong)
            result.wintag = Hong;
        else if(cardColor==Mei)
            result.wintag = Mei;
        else if(cardColor==Fang)
            result.wintag = Fang;
        else if(cardColor==Wang)
            result.wintag = Wang;
        result.card=m_cbCurrentRoundTableCards[0];
         m_openRecord.push_back(result);

        LOG(INFO) << "OnEventGameConclude chairId = " << chairId<<"--------open color=="<<cardColor;
        return true;
    }
    return false;
}

void CTableFrameSink::clearTableUser()
{
    int64_t min_score = MIN_PLAYING_ANDROID_USER_SCORE;
    if(m_pGameRoomInfo)
        min_score = std::max(m_pGameRoomInfo->enterMinScore, min_score);


    vector<uint64_t> clearUserIdVec;
    vector<uint32_t> clearChairIdVec;
    {
//        WRITE_LOCK(m_mutex);

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(pUserItem)
            {
                if(m_pGameRoomInfo->serverStatus !=SERVER_RUNNING|| (pUserItem->GetUserStatus() == sOffline ||
                   (pUserItem->IsAndroidUser() && pUserItem->GetUserScore() < min_score)))
                {

                    pUserItem->setOffline();
                    clearUserIdVec.push_back(userInfoItem->userId);
                    clearChairIdVec.push_back(userInfoItem->chairId);
                }
            }
        }
    }

    uint32_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_pITableFrame->ClearTableUser(clearChairIdVec[i]);
        m_UserInfo.erase(clearUserIdVec[i]);
        for(auto it = m_SortUserInfo.begin(); it != m_SortUserInfo.end(); ++it)
        {
            if(clearUserIdVec[i] == (*it)->userId)
            {
                m_SortUserInfo.erase(it);
                break;
            }
        }
    }
}

//保存玩家结算结果
void CTableFrameSink::WriteUserScore()
{
    vector<tagScoreInfo> scoreInfoVec;

    tagScoreInfo scoreInfo;
    int64_t totalBetScore = 0;

	int64_t allBetScore = 0;

    int64_t effectiveBet=0;
	int64_t userid = 0;
	m_replay.detailsData = "";
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(pUserItem)
        {
//            memset(&scoreInfo, 0, sizeof(tagScoreInfo));
			allBetScore = 0;
			userid = pUserItem->GetUserId();
            scoreInfo.clear();
            if(pUserItem->IsAndroidUser())
            {
                totalBetScore = userInfoItem->m_lRobotUserJettonScore[Hei]
                        + userInfoItem->m_lRobotUserJettonScore[Hong]
                        + userInfoItem->m_lRobotUserJettonScore[Mei]
                        + userInfoItem->m_lRobotUserJettonScore[Fang]
                        + userInfoItem->m_lRobotUserJettonScore[Wang];
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[Fang]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[Mei]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[Hong]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[Hei]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lRobotUserJettonScore[Wang]);
            }else
            {
                totalBetScore = userInfoItem->m_lUserJettonScore[Hei]
                        + userInfoItem->m_lUserJettonScore[Hong]
                        + userInfoItem->m_lUserJettonScore[Mei]
                        + userInfoItem->m_lUserJettonScore[Fang]
                        + userInfoItem->m_lUserJettonScore[Wang];
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[Fang]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[Mei]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[Hong]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[Hei]);
                scoreInfo.cellScore.push_back(userInfoItem->m_lUserJettonScore[Wang]);
				allBetScore = totalBetScore;
                if(m_nWinIndex==4)
                {
                    effectiveBet=userInfoItem->m_lUserJettonScore[Wang];
                }
            }
            if(totalBetScore > 0)
            {
//                if(userInfoItem->m_EndWinScore)
                {
                    scoreInfo.chairId = userInfoItem->chairId;
                    scoreInfo.betScore = totalBetScore;
                    if(m_nWinIndex==4)
                     scoreInfo.rWinScore = userInfoItem->m_lUserJettonScore[Wang];
                    else
                     scoreInfo.rWinScore = totalBetScore  ;
                    scoreInfo.addScore = userInfoItem->m_EndWinScore;
                    scoreInfo.revenue = userInfoItem->m_Revenue;
                    scoreInfo.cardValue = GlobalFunc::converToHex(m_cbCurrentRoundTableCards, 1);  // GetCardValueName(0) + " vs "+GetCardValueName(1);
                    scoreInfo.startTime = m_startTime;
                    scoreInfoVec.push_back(scoreInfo);
                }
            }
            // 统计连续未下注的次数
            if (totalBetScore<=0)
            {
                userInfoItem->NotBetJettonCount++;
            }
            else
            {            
                userInfoItem->NotBetJettonCount = 0;
            }
			if (allBetScore > 0)
			{
				//对局记录详情
				SetGameRecordDetail(userid, userInfoItem->chairId, pUserItem->GetUserScore(), userInfoItem);
			}
        }
    }

    m_pITableFrame->WriteUserScore(&scoreInfoVec[0], scoreInfoVec.size(), m_strRoundId);
    m_pITableFrame->SaveReplay(m_replay);
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    int64_t totalJetton = 0;

    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;

        //添加下注信息
        if(pUserItem->IsAndroidUser())
        {
            totalJetton = userInfoItem->m_lRobotUserJettonScore[Hei]
                    + userInfoItem->m_lRobotUserJettonScore[Hong]
                    + userInfoItem->m_lRobotUserJettonScore[Mei]
                    + userInfoItem->m_lRobotUserJettonScore[Fang]
                    + userInfoItem->m_lRobotUserJettonScore[Wang];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }else
        {
            totalJetton = userInfoItem->m_lUserJettonScore[Hei]
                    + userInfoItem->m_lUserJettonScore[Hong]
                    + userInfoItem->m_lUserJettonScore[Mei]
                    + userInfoItem->m_lUserJettonScore[Fang]
                    + userInfoItem->m_lUserJettonScore[Wang];
            if(userInfoItem->m_JettonScore.size() >= 20)
                userInfoItem->m_JettonScore.pop_front();
             userInfoItem->m_JettonScore.push_back(totalJetton);
        }
        totalJetton = 0;
        totalJetton = accumulate(userInfoItem->m_JettonScore.begin(), userInfoItem->m_JettonScore.end(), totalJetton);

        //修改输赢 更新输赢总局数 获取神算子
        if(userInfoItem->m_WinScore.size() >= 20)
            userInfoItem->m_WinScore.pop_front();
         userInfoItem->m_WinScore.push_back(userInfoItem->m_EndWinScore);

         //输赢总局数
         uint32_t winCount = 0;
         int64_t totalWin = 0;
         for(auto iter : userInfoItem->m_WinScore)
         {
            totalWin += iter;
            if(iter > 0)
                winCount++;
         }

         userInfoItem->lTwentyAllUserScore = totalJetton;
         userInfoItem->lTwentyWinScore = totalWin;
         userInfoItem->lTwentyWinCount = winCount;
    }
}

//改变玩家20注下注输赢信息
void CTableFrameSink::ChangeVipUserInfo()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    int64_t nLow = 0;
    m_SortUserInfo.clear();
    m_ShenSuanZiId = 0;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        if(userInfoItem)
        {
            if(m_ShenSuanZiId == 0)
            {
                m_ShenSuanZiId = userInfoItem->userId;
            }
            else if(userInfoItem->lTwentyWinCount > m_UserInfo[m_ShenSuanZiId]->lTwentyWinCount)
            {
                m_ShenSuanZiId = userInfoItem->userId;
            }
            userInfoItem->isDivineMath=false;
        }
    }
    if(m_ShenSuanZiId!=0)
    {
        m_UserInfo[m_ShenSuanZiId]->isDivineMath=true;
        m_SortUserInfo.emplace(m_UserInfo[m_ShenSuanZiId]);
    }

    bool bFirst = true;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem||m_ShenSuanZiId==userInfoItem->userId)
            continue;

         if(unlikely(bFirst))
         {
            bFirst = false;
            nLow = userInfoItem->lTwentyAllUserScore;
            m_SortUserInfo.emplace(userInfoItem);
         }else
         {
             if(m_SortUserInfo.size() < 20)
                 m_SortUserInfo.emplace(userInfoItem);
             else if(userInfoItem->lTwentyAllUserScore > nLow)
             {
                 auto it = m_SortUserInfo.end();
                 it--;
                 m_SortUserInfo.erase(it);
                 it--;
                 nLow = (*it)->lTwentyAllUserScore;
                 m_SortUserInfo.emplace(userInfoItem);
             }
         }
    }

}

//定时器事件
void CTableFrameSink::OnTimerJetton()
{
    m_endTime = chrono::system_clock::now();
//    if(--m_nPlaceJettonLeveTime < 1)
    {
//        m_TimerLoopThread->getLoop()->cancel(m_TimerIdJetton);
        m_TimerLoopThread->getLoop()->cancel(m_TimerJettonBroadcast);
//        GameTimerJettonBroadcast();

        //结束游戏
        OnEventGameConclude(INVALID_CHAIR, GER_NORMAL);

//        m_nGameEndLeveTime = m_nGameEndTime;

        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);

            if(!pUserItem)
                continue;
            SendEndMsg(userInfoItem->chairId);
        }

        m_TimerIdEnd = m_TimerLoopThread->getLoop()->runAfter(m_nGameEndTime, boost::bind(&CTableFrameSink::OnTimerEnd, this));

        WriteUserScore();
        ChangeUserInfo();
        SendWinSysMessage();

        //设置状态
        m_pITableFrame->SetGameStatus(GAME_STATUS_END);
        m_replay.clear();
        //删除玩家信息wwwwwww
        clearTableUser();  //sOffline  m_cbPlayStatus==false
        ChangeVipUserInfo();

        ClearTableData();

//        openLog("  ");
//        openLog("end usercoount=%d ",m_UserInfo.size());
        LOG_DEBUG <<"GameEnd UserCount:"<<m_UserInfo.size();

//        m_nPlaceJettonLeveTime = m_nPlaceJettonTime;
    }
}

void CTableFrameSink::OnTimerEnd()
{
    //连续5局未下注踢出房间
    //CheckKickOutUser();
    m_strRoundId = m_pITableFrame->GetNewRoundId();
    m_pITableFrame->SetGameStatus(GAME_STATUS_START);
    m_startTime = chrono::system_clock::now();
    m_replay.gameinfoid = m_strRoundId;
	m_replay.saveAsStream = true;//对局详情格式 true-binary false-jsondata
	m_nWinIndex = 0;
    {

        //ClearTableData();
        ChangeVipUserInfo();
        ReadConfigInformation();//重新读取配置
        //结束游戏
        bool IsContinue = m_pITableFrame->ConcludeGame(GAME_STATUS_START);
        if(!IsContinue)
            return;

        LOG_DEBUG <<"GameStart UserCount:"<<m_UserInfo.size();

         bool bRestart = false;
         bool NoSomeCard=false;
         for(int i=0;i<Max;i++)
         {
             leftCards[i]=0;
         }
         for(int i=0;i<m_CardVecData.size();i++)
         {
             leftCards[m_GameLogic.GetCardColor(m_CardVecData[i])]++;
         }
         for(int i=0;i<4;i++)
         {
             if(leftCards[i]==1)
             {
                 NoSomeCard=true; //
             }
         }
         if(m_CardVecData.size() < MIN_LEFT_CARDS_RELOAD_NUM||NoSomeCard)
         {
             m_CardVecData.clear();
             m_CardVecData.assign(m_cbTableCard, m_cbTableCard + MAX_CARDS);
             random_shuffle(m_CardVecData.begin(), m_CardVecData.end());

             //clear open record
             m_openRecord.clear();
             bRestart = true;
         }

         shared_ptr<IServerUserItem> pUserItem;
         shared_ptr<UserInfo> userInfoItem;
         for(auto &it : m_UserInfo)
         {
             userInfoItem = it.second;
             pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
             if(!pUserItem)
                 continue;
             if(bRestart)
                SendOpenRecord(userInfoItem->chairId);
             SendStartMsg(userInfoItem->chairId);
         }
        m_dwReadyTime = (uint32_t)time(NULL);
        //设置时间
//        m_TimerJetton.start(1000, bind(&CTableFrameSink::OnTimerJetton, this, std::placeholders::_1), NULL, m_nPlaceJettonLeveTime, false);
        m_TimerIdJetton = m_TimerLoopThread->getLoop()->runAfter(m_nPlaceJettonTime, boost::bind(&CTableFrameSink::OnTimerJetton, this));
        m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::ClearTableData()
{


//    memset(m_lUserJettonScore, 0, sizeof(m_lUserJettonScore));
//    memset(m_lRobotUserJettonScore, 0, sizeof(m_lRobotUserJettonScore));
//    memset(m_CalculateRevenue, 0, sizeof(m_CalculateRevenue));
//    memset(m_CalculateAgentRevenue, 0, sizeof(m_CalculateAgentRevenue));
//    memset(m_EndWinScore, 0, sizeof(m_EndWinScore));

    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;

        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        // 否则把重押分数记录
        if(!pUserItem->IsAndroidUser())
        {
            for (int j = 0; j < Max; j++)
            {
                userInfoItem->m_lUserLastJettonScore[j] = userInfoItem->m_lUserJettonScore[j];
                LOG(INFO)<<"m_lUserLastJettonScore[j] = m_lUserJettonScore[j]======="<<userInfoItem->m_lUserJettonScore[j];
            }
        }


        userInfoItem->clear();
        if(!pUserItem->IsAndroidUser())
        {
            for (int j = 0; j < Max; j++)
            {
                LOG(INFO)<<"m_lUserLastJettonScore[j] = m_lUserJettonScore[j]======="<<userInfoItem->m_lUserJettonScore[j];
            }
        }
    }
    memset(m_lAllJettonScore, 0, sizeof(m_lAllJettonScore));
    memset(m_lTrueUserJettonScore, 0, sizeof(m_lTrueUserJettonScore));
    memset(m_lRobotserJettonAreaScore, 0, sizeof(m_lRobotserJettonAreaScore));
    memset(m_lShenSuanZiJettonScore, 0, sizeof(m_lShenSuanZiJettonScore));
    for(int64_t jetton : m_pGameRoomInfo->jettons)
    {
        m_Hei[jetton] = 0;
        m_Hong[jetton] = 0;
        m_Mei[jetton] = 0;
        m_Fang[jetton] = 0;
        m_Wang[jetton] = 0;
    }
    memset(m_cbCurrentRoundTableCards, 0, sizeof(m_cbCurrentRoundTableCards));
    memset(m_array_jetton, 0, sizeof(m_array_jetton));
    memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
}

bool  CTableFrameSink::OnMakeDir()
{
	std::string dir="./Log";  
    if(access(dir.c_str(), 0) == -1)
    {  
        if(mkdir(dir.c_str(), 0755)== 0)
		{ 
            dir="./Log/wxhh";

            if(access(dir.c_str(), 0) == -1)
			{  
                return (mkdir(dir.c_str(), 0755)== 0);
			}  
		}
    }  	

	return false;
}
void CTableFrameSink::Rejetton(uint32_t chairid, uint8_t subid, const uint8_t *data, uint32_t datasize)
{
    shared_ptr<CServerUserItem> Usr = m_pITableFrame->GetTableUserItem(chairid);
    if (!Usr){ LOG(WARNING) << "----查找玩家不在----"; return;}

    LOG(WARNING) << "-----------Rejetton续押-------------";
    // 游戏状态判断
     if(m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
        LOG(WARNING) << "----------------游戏状态不对，不能押注----------" << m_pITableFrame->GetGameStatus() ;
        return;
    }

    int64_t userid = Usr->GetUserId();

    int64_t alljetton = 0;
    int64_t areajetton[Max] = {0};


    if(!Usr->IsAndroidUser())
    {
        for (int i = 0; i < Max; i++)
        {
            alljetton += m_UserInfo[userid]->m_lUserLastJettonScore[i];
            areajetton[i] = m_UserInfo[userid]->m_lUserLastJettonScore[i];
            LOG(WARNING) << "--dLastJetton-" << areajetton[i] ;
        }
    }
    else
    {

    }

    // 续押失败
    if (alljetton == 0 || Usr->GetUserScore() < alljetton)
    {
        CMD_S_PlaceJettonFail placeJetFail;

        placeJetFail.set_dwuserid(userid);
        placeJetFail.set_cbjettonarea(3);
        placeJetFail.set_lplacescore(0);
        placeJetFail.set_cberrorcode(3);
        placeJetFail.set_cbandroid(Usr->IsAndroidUser());

        std::string sendData = placeJetFail.SerializeAsString();
        m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)sendData.c_str(), sendData.size());
        LOG(WARNING) << "------------------续押失败(玩家分数不足)------------" << alljetton << " " << Usr->GetUserScore();
        return;
    }

    if(!Usr->IsAndroidUser())
    {
        for (int i = 0; i < Max; i++)
        {
            // 判断是否超过区域最大押分   < 每个区域的总下分  + 上次玩家总押分值
            if (m_AreaMaxJetton[i]<m_UserInfo[userid]->m_lUserJettonScore[i])
            {
                CMD_S_PlaceJettonFail placeJetFail;
                placeJetFail.set_dwuserid(userid);
                placeJetFail.set_cbjettonarea(i);
                placeJetFail.set_lplacescore(m_UserInfo[userid]->m_lUserLastJettonScore[i]);
                placeJetFail.set_cberrorcode(2);
                placeJetFail.set_cbandroid(Usr->IsAndroidUser());

                std::string data = placeJetFail.SerializeAsString();
                // if (m_bTest==0)
                    m_pITableFrame->SendTableData(Usr->GetChairId(), SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
                LOG(WARNING) << "---------续押失败(超过区域最大押分)---------" << m_AreaMaxJetton[i] ;
                return;
            }
        }
    }
    else
    {

    }


    // LOG(ERROR) << "---------重押成功--------";
    for (int i = 0; i < Max; i++)
    {
        if (areajetton[i] == 0)
        {
            continue;
        }

        while (areajetton[i] > 0)
        {
            int jSize =CHIPS_SIZE;
            for (int j=jSize-1;j>=0;j--)
            {
                if (areajetton[i] >= m_currentchips[chairid][i])
                {
                    int64_t tmpVar = m_currentchips[chairid][i];
                    OnUserAddScore1(chairid, i, tmpVar);
                    areajetton[i] -= tmpVar;
                    break;
                }
            }
        }
    }
}
// 押分判断


//游戏消息处理
//bool  CTableFrameSink::OnGameMessage(WORD wSubCmdID, VOID * pData, WORD wDataSize, IServerUserItem * pIServerUserItem)
bool CTableFrameSink::OnGameMessage(uint32_t wChairID, uint8_t wSubCmdID, const uint8_t* pData, uint32_t wDataSize)
{
    //::muduo::MutexLockGuard lock(m_mutexLock);
//    LOG(INFO) << "CTableFrameSink::OnGameMessage wChairID:" << wChairID << ", wSubCmdID:" << wSubCmdID << ", wDataSize:" << wDataSize;

    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if (!pIServerUserItem)
    {
        LOG(INFO) << "OnGameMessage pServerOnLine is NULL  wSubCmdID:" << wSubCmdID;
		return false;
    }

	switch (wSubCmdID)
    {
        case SUB_C_CAN_REJET:
        {
            int64_t alljetton=0;
            CMD_S_CAN_REJET rejet;
            for (int i = 0; i < Max; i++)
            {
                rejet.add_rejettonscore(m_UserInfo[pIServerUserItem->GetUserId()]->m_lUserLastJettonScore[i]);
                rejet.add_jettonscore(m_UserInfo[pIServerUserItem->GetUserId()]->m_lUserJettonScore[i]);
                 LOG(INFO) << "m_lUserLastJettonScore:" << m_UserInfo[pIServerUserItem->GetUserId()]->m_lUserLastJettonScore[i];
            }
            std::string data = rejet.SerializeAsString();
            m_pITableFrame->SendTableData(wChairID, wxhh::SUB_S_CAN_REJET, (uint8_t *)data.c_str(), data.size());
            return true;
        }
        case SUB_C_PLAYER_REJET:
        {
             Rejetton(wChairID, wSubCmdID, pData, wDataSize); //续押
             return true;
        }
        case SUB_C_PLACE_JETTON:			//用户下注
		{
            //变量定义
            CMD_C_PlaceJet  pJETTON;
            pJETTON.ParseFromArray(pData,wDataSize);

			//消息处理
            return OnUserAddScore(wChairID, pJETTON.cbjettonarea(), pJETTON.ljettonscore());
		}
        case SUB_C_QUERY_PLAYERLIST:
        {

            return true;
        }
        case SUB_C_SYNC_TIME:
        {
            int32_t status = m_pITableFrame->GetGameStatus();
            CMD_S_SyncTime_Res res;

            res.set_userid(pIServerUserItem->GetUserId());
            res.set_status(status);

            chrono::system_clock::time_point curTime = chrono::system_clock::now();
            int32_t leaveTime = 0;
            switch (status)
            {
            case GAME_STATUS_START:
                leaveTime = m_nPlaceJettonTime - (chrono::duration_cast<chrono::seconds>(curTime - m_startTime)).count();
                break;
            case GAME_STATUS_END:
                leaveTime = m_nGameEndTime - (chrono::duration_cast<chrono::seconds>(curTime - m_endTime)).count();
            default:
                break;
            }
            res.set_timeleave(leaveTime >0 ? leaveTime : 0);
            std::string data = res.SerializeAsString();
            m_pITableFrame->SendTableData(wChairID, wxhh::SUB_S_SYNC_TIME, (uint8_t*)data.c_str(), data.size());
        }
        break;
        return false;
    }
}

//加注事件
bool CTableFrameSink::OnUserAddScore(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore)
{
    //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    if(m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
        LOG_ERROR << " Game Status is not GameStart ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
        return true;
    }
    //=====================================
    bool bFind = false;
    int size=m_allChips.size();
    for(int i=0;i<size;i++)
    {
        if(m_allChips[i]==lJettonScore)
        {
            bFind = true;
            break;
        }
    }
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<lJettonScore;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 8);
        return true;
    }

    if(cbJettonArea < 0 || cbJettonArea > 4)
    {
        LOG_ERROR << " Can not find jettons Area:"<<cbJettonArea;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 2);
        return true;
    }
    //=================== ===================
	//校验是否当前用户
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " User is not Valid ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 3);
        return true;
    }
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();
    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lUserJettonScore[Hei]
                + userInfo->m_lUserJettonScore[Hong]
                + userInfo->m_lUserJettonScore[Mei]
                + userInfo->m_lUserJettonScore[Fang]
                + userInfo->m_lUserJettonScore[Wang];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Real User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 6);
            return true;
        }

        if(userInfo->m_lUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Real User Score exceed the limit MaxJettonScore =="<<userInfo->m_lUserJettonScore[cbJettonArea]<<"cbJettonArea=="<<cbJettonArea<<"     m_AreaMaxJetton[cbJettonArea]"<<m_AreaMaxJetton[cbJettonArea];
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
            return true;
        }

        if(lTotalJettonScore == 0)
        {
            m_replay.addPlayer(userId,pIServerUserItem->GetAccount(),lUserScore,wChairID);
        }
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,wChairID,cbJettonArea);
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
    }else //android suer
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lRobotUserJettonScore[Hei]
                + userInfo->m_lRobotUserJettonScore[Hong]
                + userInfo->m_lRobotUserJettonScore[Mei]
                + userInfo->m_lRobotUserJettonScore[Fang]
                + userInfo->m_lRobotUserJettonScore[Wang];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50)
        {
            LOG_ERROR << " Android User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 6);
            return true;
        }
        if(userInfo->m_lRobotUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Android User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 7);
            return true;
        }

        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lRobotserJettonAreaScore[cbJettonArea] += lJettonScore;
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
    }

    if(m_ShenSuanZiId)
    {
        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
        if(SSZUserInfo->chairId == wChairID)
            m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }

    if(cbJettonArea == Hei)
    {
        m_Hei[lJettonScore]++;
    }else if(cbJettonArea == Hong)
    {
        m_Hong[lJettonScore]++;
    }else if(cbJettonArea == Mei)
    {
        m_Mei[lJettonScore]++;
    }
    else if(cbJettonArea == Fang)
    {
        m_Fang[lJettonScore]++;
    }
    else if(cbJettonArea == Wang)
    {
        m_Wang[lJettonScore]++;
    }
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    CMD_S_PlaceJetSuccess Jetton;
    auto push = [&](uint32_t sendChairId, int64_t sendUserId, bool Robot)
    {
        Jetton.Clear();
        Jetton.set_dwuserid(userId);
        Jetton.set_cbjettonarea(cbJettonArea);
        Jetton.set_ljettonscore(lJettonScore);
        bool isRobot = pIServerUserItem->IsAndroidUser();
        Jetton.set_bisandroid(isRobot);

        int64_t xzScore = 0;
        if(isRobot)
            xzScore = userInfo->m_lRobotUserJettonScore[Hei]
                    + userInfo->m_lRobotUserJettonScore[Hong]
                    + userInfo->m_lRobotUserJettonScore[Mei]
                    + userInfo->m_lRobotUserJettonScore[Fang]
                    + userInfo->m_lRobotUserJettonScore[Wang];
        else
            xzScore = userInfo->m_lUserJettonScore[Hei]
                    + userInfo->m_lUserJettonScore[Hong]
                    + userInfo->m_lUserJettonScore[Mei]
                    + userInfo->m_lUserJettonScore[Fang]
                    + userInfo->m_lUserJettonScore[Wang];
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 0; i < Max; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 0; i < Max; ++i)
        {
            PlaceJettonScore* userJetton = Jetton.add_selfjettonscore();
            userJetton->set_dwuserid(sendUserId);
            userJetton->set_ljettonarea(i);
            int64_t jettonScore = 0;
            if(Robot)
                jettonScore =  userInfoItem->m_lRobotUserJettonScore[i];
             else
                jettonScore =  userInfoItem->m_lUserJettonScore[i];
             userJetton->set_ljettonscore(jettonScore);
        }
        std::string data = Jetton.SerializeAsString();
        m_pITableFrame->SendTableData(sendChairId, wxhh::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };

    if(/*!isInVipList(wChairID) && userId != m_ShenSuanZiId && m_SortUserInfo.size() >= 8*/true)
    {
        userInfoItem = m_UserInfo[userId];
        push(wChairID, userId, pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }else
    {
        for(auto &it : m_UserInfo)
        {
            if (!it.second)
                continue;

            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem)
                continue;
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}
//加注事件
bool CTableFrameSink::OnUserAddScore1(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore)
{
    //openLog("xiazhu area=%d score=%f",cbJettonArea,lJettonScore);
    if(m_pITableFrame->GetGameStatus() != GAME_STATUS_START)
    {
        LOG_ERROR << " Game Status is not GameStart ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 1);
        return true;
    }
    //=====================================
    bool bFind = false;
    int size=m_allChips.size();
    for(int i=0;i<size;i++)
    {
        if(m_allChips[i]==lJettonScore)
        {
            bFind = true;
            break;
        }
    }
    if(!bFind)
    {
        LOG_ERROR << " Jettons Error:"<<lJettonScore;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 8);
        return true;
    }

    if(cbJettonArea < 0 || cbJettonArea > 4)
    {
        LOG_ERROR << " Can not find jettons Area:"<<cbJettonArea;
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 2);
        return true;
    }
    //======================================
    //校验是否当前用户
    shared_ptr<IServerUserItem> pIServerUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pIServerUserItem)
    {
        LOG_ERROR << " User is not Valid ";
        SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 3);
        return true;
    }
    int64_t userId = pIServerUserItem->GetUserId();
    shared_ptr<UserInfo> &userInfo = m_UserInfo[userId];

    //玩家积分
    int64_t lUserScore = pIServerUserItem->GetUserScore();
    if(!pIServerUserItem->IsAndroidUser())  //real user
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lUserJettonScore[Hei]
                + userInfo->m_lUserJettonScore[Hong]
                + userInfo->m_lUserJettonScore[Mei]
                + userInfo->m_lUserJettonScore[Fang]
                + userInfo->m_lUserJettonScore[Wang];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50.0)
        {
            LOG_ERROR << " Real User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 4);
            return true;
        }

        if(userInfo->m_lUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Real User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 5);
            return true;
        }

        if(lTotalJettonScore == 0)
        {
            m_replay.addPlayer(userId,pIServerUserItem->GetAccount(),lUserScore,wChairID);
        }
        m_replay.addStep((uint32_t)time(NULL) - m_dwReadyTime,to_string(lJettonScore),-1,opBet,wChairID,cbJettonArea);
        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lTrueUserJettonScore[cbJettonArea] += lJettonScore;
        userInfo->m_lUserJettonScore[cbJettonArea] += lJettonScore;
    }else //android suer
    {
        //已经下的总注
        int64_t lTotalJettonScore = userInfo->m_lRobotUserJettonScore[Hei]
                + userInfo->m_lRobotUserJettonScore[Hong]
                + userInfo->m_lRobotUserJettonScore[Mei]
                + userInfo->m_lRobotUserJettonScore[Fang]
                + userInfo->m_lRobotUserJettonScore[Wang];
        if(lUserScore < lTotalJettonScore + lJettonScore || lUserScore - lTotalJettonScore < 50)
        {
            LOG_ERROR << " Android User Score is not enough ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 6);
            return true;
        }
        if(userInfo->m_lRobotUserJettonScore[cbJettonArea] + lJettonScore > m_AreaMaxJetton[cbJettonArea])
        {
            LOG_ERROR << " Android User Score exceed the limit MaxJettonScore ";
            SendPlaceBetFail(wChairID, cbJettonArea, lJettonScore, 7);
            return true;
        }

        m_lAllJettonScore[cbJettonArea] += lJettonScore;
        m_lRobotserJettonAreaScore[cbJettonArea] += lJettonScore;
        userInfo->m_lRobotUserJettonScore[cbJettonArea] += lJettonScore;
    }

    if(m_ShenSuanZiId)
    {
        shared_ptr<UserInfo> SSZUserInfo = m_UserInfo[m_ShenSuanZiId];
        if(SSZUserInfo->chairId == wChairID)
            m_lShenSuanZiJettonScore[cbJettonArea] += lJettonScore;
    }

    if(cbJettonArea == Hei)
    {
        m_Hei[lJettonScore]++;
    }else if(cbJettonArea == Hong)
    {
        m_Hong[lJettonScore]++;
    }else if(cbJettonArea == Mei)
    {
        m_Mei[lJettonScore]++;
    }
    else if(cbJettonArea == Fang)
    {
        m_Fang[lJettonScore]++;
    }
    else if(cbJettonArea == Wang)
    {
        m_Wang[lJettonScore]++;
    }
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;

    CMD_S_PlaceJetSuccess Jetton;
    auto push = [&](uint32_t sendChairId, int64_t sendUserId, bool Robot)
    {
        Jetton.Clear();
        Jetton.set_dwuserid(userId);
        Jetton.set_cbjettonarea(cbJettonArea);
        Jetton.set_ljettonscore(lJettonScore);
        bool isRobot = pIServerUserItem->IsAndroidUser();
        Jetton.set_bisandroid(isRobot);

        int64_t xzScore = 0;
        if(isRobot)
            xzScore = userInfo->m_lRobotUserJettonScore[Hei]
                    + userInfo->m_lRobotUserJettonScore[Hong]
                    + userInfo->m_lRobotUserJettonScore[Mei]
                    + userInfo->m_lRobotUserJettonScore[Fang]
                    + userInfo->m_lRobotUserJettonScore[Wang];
        else
            xzScore = userInfo->m_lUserJettonScore[Hei]
                    + userInfo->m_lUserJettonScore[Hong]
                    + userInfo->m_lUserJettonScore[Mei]
                    + userInfo->m_lUserJettonScore[Fang]
                    + userInfo->m_lUserJettonScore[Wang];
        Jetton.set_luserscore(pIServerUserItem->GetUserScore() - xzScore);

        for(int i = 0; i < Max; ++i)
        {
            PlaceJettonScore* areaJetton = Jetton.add_alljettonscore();
            areaJetton->set_dwuserid(0);
            areaJetton->set_ljettonarea(i);
            areaJetton->set_ljettonscore(m_lAllJettonScore[i]);
        }

        for(int i = 0; i < Max; ++i)
        {
            PlaceJettonScore* userJetton = Jetton.add_selfjettonscore();
            userJetton->set_dwuserid(sendUserId);
            userJetton->set_ljettonarea(i);
            int64_t jettonScore = 0;
            if(Robot)
                jettonScore =  userInfoItem->m_lRobotUserJettonScore[i];
             else
                jettonScore =  userInfoItem->m_lUserJettonScore[i];
             userJetton->set_ljettonscore(jettonScore);
        }
        std::string data = Jetton.SerializeAsString();
        m_pITableFrame->SendTableData(sendChairId, wxhh::SUB_S_PLACE_JETTON, (uint8_t*)data.c_str(), data.size());
    };

    if(/*!isInVipList(wChairID) && userId != m_ShenSuanZiId && m_SortUserInfo.size() >= 8*/true)
    {
        userInfoItem = m_UserInfo[userId];
        push(wChairID, userId, pIServerUserItem->IsAndroidUser());
        AddPlayerJetton(wChairID, cbJettonArea, lJettonScore);
    }else
    {
        for(auto &it : m_UserInfo)
        {
            if (!it.second)
                continue;

            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem)
                continue;
            push(userInfoItem->chairId,userInfoItem->userId,pUserItem->IsAndroidUser());
        }
    }

    return true;
}
void CTableFrameSink::AddPlayerJetton(int64_t chairId, uint8_t areaId, int64_t score)
{
    assert(chairId<GAME_PLAYER &&areaId<5);

    m_array_jetton[chairId].bJetton = true;
    m_array_jetton[chairId].areaJetton[areaId] +=score;
    m_array_tmparea_jetton[areaId] += score;
}

void CTableFrameSink::GameTimerJettonBroadcast()
{
    CMD_S_Jetton_Broadcast jetton_broadcast;

    bool havejetton = false;
    for(int i = 0; i < Max; ++i)
    {
        if(m_array_tmparea_jetton[i] > 0)
        {
            havejetton = true;
            break;
        }
    }
    if(havejetton)
    {
        shared_ptr<IServerUserItem> pUserItem;
        shared_ptr<UserInfo> userInfoItem;
        for(auto &it : m_UserInfo)
        {
            userInfoItem = it.second;
            pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
            if(!pUserItem||pUserItem->IsAndroidUser())
                continue;
            jetton_broadcast.Clear();

            for (int i = 0; i < Max; ++i)
            {
               jetton_broadcast.add_alljettonscore(m_lAllJettonScore[i]);
               jetton_broadcast.add_tmpjettonscore(m_array_tmparea_jetton[i] - m_array_jetton[userInfoItem->chairId].areaJetton[i]);
            }
            memset(m_array_jetton[userInfoItem->chairId].areaJetton, 0, sizeof(m_array_jetton[userInfoItem->chairId].areaJetton));
            std::string data = jetton_broadcast.SerializeAsString();
            m_pITableFrame->SendTableData(userInfoItem->chairId, SUB_S_JETTON_BROADCAST, (uint8_t *)data.c_str(), data.size());
        }
        memset(m_array_tmparea_jetton, 0, sizeof(m_array_tmparea_jetton));
    }
    if(m_pITableFrame->GetGameStatus() == GAME_STATUS_START)
    {
        m_TimerJettonBroadcast = m_TimerLoopThread->getLoop()->runAfter(m_nGameTimeJettonBroadcast,boost::bind(&CTableFrameSink::GameTimerJettonBroadcast, this));
    }
}

void CTableFrameSink::SendPlaceBetFail(uint32_t wChairID, uint8_t cbJettonArea, int64_t lJettonScore, int32_t errCode)
{
    CMD_S_PlaceJettonFail Jetton;

    shared_ptr<IServerUserItem> pUserItem;
    pUserItem = m_pITableFrame->GetTableUserItem(wChairID);
    if(!pUserItem)
        return;
     int64_t userid = pUserItem->GetUserId();
     bool isRobot = pUserItem->IsAndroidUser();
     Jetton.set_dwuserid(userid);
     Jetton.set_cbjettonarea(cbJettonArea);
     Jetton.set_lplacescore(lJettonScore);
     Jetton.set_cbandroid(isRobot);
     Jetton.set_cberrorcode(errCode);

     std::string data = Jetton.SerializeAsString();
     m_pITableFrame->SendTableData(wChairID, wxhh::SUB_S_PLACE_JET_FAIL, (uint8_t *)data.c_str(), data.size());
}

void CTableFrameSink::SendWinSysMessage()
{
    shared_ptr<IServerUserItem> pUserItem;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it : m_UserInfo)
    {
        userInfoItem = it.second;
        pUserItem = m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if(userInfoItem->m_EndWinScore >= m_pITableFrame->GetGameRoomInfo()->broadcastScore)
        {
           m_pITableFrame->SendGameMessage(userInfoItem->chairId, "", SMT_GLOBAL|SMT_SCROLL, userInfoItem->m_EndWinScore);
        }
    }
}

string CTableFrameSink::GetCardValueName(int index)
{
    string strName;

    //获取花色
    uint8_t cbColor = m_GameLogic.GetCardColor(m_cbCurrentRoundTableCards[index]);
    uint8_t cbValue = m_GameLogic.GetCardValue(m_cbCurrentRoundTableCards[index]);
    switch(cbColor)
    {
        case Hei:
            strName = "黑桃";
            break;
        case Hong:
            strName = "红桃";
            break;
        case Mei:
            strName = "梅花";
            break;
        case Fang:
            strName = "方块";
            break;
        case Wang:
            strName = "王";
        break;
        default:
            break;
    }
    switch(cbValue)
    {
        case 1:
            strName += "A";
            break;
        case 2:
            strName += "2";
            break;
        case 3:
            strName += "3";
            break;
        case 4:
            strName += "4";
            break;
        case 5:
            strName += "5";
            break;
        case 6:
            strName += "6";
            break;
        case 7:
            strName += "7";
            break;
        case 8:
            strName += "8";
            break;
        case 9:
            strName += "9";
            break;
        case 10:
            strName += "10";
            break;
        case 11:
            strName += "J";
            break;
        case 12:
            strName += "Q";
            break;
        case 13:
            strName += "K";
            break;
        case 14:
            strName += "小王";
            break;
        case 15:
            strName += "大王";
            break;
        default:
            break;
    }
    return strName;
}


// try to create the special table frame sink item now.
extern "C" shared_ptr<ITableFrameSink> CreateTableFrameSink(void)
{
    shared_ptr<CTableFrameSink> pTableFrameSink(new CTableFrameSink());
    shared_ptr<ITableFrameSink> pITableFramSink = dynamic_pointer_cast<ITableFrameSink>(pTableFrameSink);
    return pITableFramSink;
}

// try to delete the special table frame sink content item.
extern "C" void DeleteTableFrameSink(shared_ptr<ITableFrameSink>& pSink)
{
    pSink.reset();
}

void  CTableFrameSink::openLog(const char *p, ...)
{
    //if (m_IsEnableOpenlog)
    {

        struct tm *t;
        time_t tt;
        time(&tt);
        t = localtime(&tt);

        char Filename[256] = { 0 };
        sprintf(Filename, "./log/wxhh/wxhh_%d%d%d.log",t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
        FILE *fp = fopen(Filename, "a");
        if (NULL == fp) {
            return;
        }

        va_list arg;
        va_start(arg, p);
        fprintf(fp, "[%d%d%d %02d:%02d:%02d]", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,t->tm_hour,t->tm_min,t->tm_sec);
        vfprintf(fp, p, arg);
        fprintf(fp, "\n");
        fclose(fp);
    }
}

void CTableFrameSink::CheckKickOutUser()
{
    vector<int> clearUserIdVec;
    shared_ptr<UserInfo> userInfoItem;
    for(auto &it :m_UserInfo){
        userInfoItem = it.second;
        shared_ptr<CServerUserItem> pUserItem=m_pITableFrame->GetTableUserItem(userInfoItem->chairId);
        if(!pUserItem)
            continue;
        if (pUserItem->IsAndroidUser())
            continue;      
        if (userInfoItem->NotBetJettonCount>=5)
        {
            pUserItem->SetUserStatus(sGetoutAtplaying);
            clearUserIdVec.push_back(userInfoItem->userId);            
            m_pITableFrame->BroadcastUserStatus(pUserItem, true);            
            m_pITableFrame->ClearTableUser(userInfoItem->chairId,true,true);
            if(userInfoItem->userId == m_ShenSuanZiId)
            {
                m_ShenSuanZiId = 0;
            }
            LOG(WARNING)<<"====连续5局未下注踢出房间===="<<" chairid="<<userInfoItem->chairId<<" userId="<<userInfoItem->userId;          
        } 
    }
    uint8_t size = clearUserIdVec.size();
    for(int i = 0; i < size; ++i)
    {
        m_UserInfo.erase(clearUserIdVec[i]);
    }
}

//设置当局游戏详情
void CTableFrameSink::SetGameRecordDetail(int64_t userid, int32_t chairId, int64_t userScore, shared_ptr<UserInfo> userInfo)
{
	//记录当局游戏详情json
	//boost::property_tree::ptree root, items, player[5];
	//记录当局游戏详情binary
	string strDetail = "";
    wxhh::CMD_S_GameRecordDetail details;
	details.set_gameid(m_pITableFrame->GetGameRoomInfo()->gameId);
	details.set_userscore(userScore);			//携带积分
	details.set_winlostscore(userInfo->m_EndWinScore); //总输赢积分
	//系统开牌
    for (int i = 0; i < 1; ++i)
	{
		SysCards* card = details.add_syscard();
		card->set_cards(&(m_cbCurrentRoundTableCards)[i], 1);
	}

	{
		//和龙虎: 1 2 3
		int32_t nWinIndex = 0;
        for (int betIdx = 0;betIdx < Max; betIdx++)
		{
			// 结算时记录
			int32_t winScore = userInfo->dAreaWin[betIdx];
			int64_t betScore = userInfo->m_lUserJettonScore[betIdx];
			nWinIndex = 0;
			if (betIdx == m_nWinIndex)
			{
                nWinIndex = 1;
			}
			{
                wxhh::BetAreaRecordDetail* detail = details.add_detail();
				//下注区域
				detail->set_dareaid(betIdx);
				//本区域是否赢[0:否;1:赢]
                detail->set_dareaiswin(nWinIndex);
				//投注积分
				detail->set_dareajetton(betScore);
				//区域输赢
				detail->set_dareawin(winScore);
			}
		}
	}
	//对局记录详情json
	if (!m_replay.saveAsStream) {
	}
	else {
		strDetail = details.SerializeAsString();
	}
	m_replay.addDetail(userid, chairId, strDetail);
}

