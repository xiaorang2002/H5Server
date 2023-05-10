#ifndef SGJ_CONFIGMANAGER_H
#define SGJ_CONFIGMANAGER_H

#include "types.h"
#include <vector>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <list>
#include <time.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <glog/logging.h>
#include "INIHelp.h"

//宏定义
#ifndef _UNICODE
#define myprintf	_snprintf
#define mystrcpy	strcpy
#define mystrlen	strlen
#define myscanf		_snscanf
#define	myLPSTR		LPCSTR
#define mystchr		strchr
#define myatoi		atoi
#define mychar		char
#else
#define myprintf	swprintf
#define mystrcpy	wcscpy
#define mystrlen	wcslen
#define myscanf		_snwscanf
#define	myLPSTR		LPWSTR
#define mystchr		wcschr
#define myatoi		_wtoi
#define mychar		TCHAR
#endif

//配置刷新时间
#define		ConfigRenewTime			30
#define		Max_Assembly_Num		30

#define     INT_MAX         2147483647    // maximum (signed) int value
#define     FRUIT_MAX		13
#define     LINE_MAX        9
#define     ICON_MAX        15
#define     Table_Row_Num	3
#define     Table_Col_Num	5

#define  AssertReturn(x,y) \
    if(!(x)){\
        y;\
    }

using namespace std;

//个人概率
struct UserProbabilityItem { 
    int32_t		nFruitPro[FRUIT_MAX];   //图标概率数组 
    int32_t		nBounsPro[FRUIT_MAX];   //Bouns的概率 
    UserProbabilityItem() {
        memset(nFruitPro,	0, sizeof(nFruitPro));
        memset(nBounsPro,	0,	sizeof(nBounsPro));
    }
};

struct tagAllConfig
{
     //库存权重数组
    UserProbabilityItem vProbability[3];    //三组净分概率组
    int32_t		nPullInterval;              //拉动时间间隔
    int32_t     nKickOutTime;               //踢出时间
    int32_t		m_MaxBet;                   //最大押注
    int32_t		m_MinBet;                   //最小押注
    int64_t		m_CtrlVal;                  //控制线
    int32_t		m_testCount;                //测试次数
    int32_t		m_testWeidNum;              //测试次数
    int32_t		m_runAlg;                   //是否开启测试

    int32_t		m_testLine;                 //是否开启测试
    int32_t		m_line[3][5];                 //是否开启测试


    int32_t		m_PullJackpotRecInterval;   //拉取奖池记录的频率（多少秒，以秒为单位）
    int32_t		m_MiniJackPotId;            //奖池中奖Scatter数量
    int32_t		m_MsgItemCount;             //奖池中奖显示记录
    int32_t		m_BetRateVal;               //奖池押分抽水率(百分比)
    int32_t		m_MaxAndroidCount;          //最大机器人数量
    int32_t		m_MinBoardCastOdds;         //最小播报倍数条件

    int32_t		m_ScatterJackPot[3];        //获Scatter对应获奖比例(百分比)

    int64_t		m_AndroidUserId[10];        //机器人使用的UserID
    int64_t		m_AndroidWeight[10];        //机器人对应UserID的使用权重
    double      stockweak;

    int64_t     m_AgentId;
    double      m_AndroidRatio;
	tagAllConfig()
    {
        memset(vProbability,0,sizeof(vProbability));
        nPullInterval = 2;
        nKickOutTime = 120;
        stockweak=1.0;
        m_AgentId = 10000;
    }
};
 
class ConfigManager
{
public:
    static ConfigManager * m_Instance;
    //上次读取时间
    time_t	    m_LastReadTime;
    int			m_CurrentRound;
    vector<int> vBetItemList; // 获取押分列表
    vector<int> vRewardList; // 派奖获奖比例(千分比)
  
    //配置
    tagAllConfig	m_Config;
    //初始化
    int Init(const	char AppName[],std::string szConfigName);//const char szConfigName[]);
public:
    ConfigManager();
    ~ConfigManager();

    static ConfigManager * GetInstance()
    {
        if (m_Instance == NULL)
        {
            m_Instance = new ConfigManager();
        }
        return m_Instance;
    }
    //检查读取
    int CheckReadConfig();
    //获取配置
    void GetConfig(tagAllConfig &config);
 
    int getKickOutTime();

    vector<int> GetBetItemList();
    vector<int> GetRewardList();

    //轮次
    int	 GetCurrentRound();
    //重置轮次
    void ResetCurrentRound();

    int InitTest(tagAllConfig config);

    void *mutex;
};

#endif // CONFIGMANAGER_H
