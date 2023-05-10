/*
	客户引擎接口文件.

History:
	2016-05-27 17:00 
	增加OnUserSitdown/OnUserStandup,跟踪新用户.
*/

#ifndef __IAICSENGINE_HEADER__
#define __IAICSENGINE_HEADER__

#include <windows.h>
#define AICS_DATA_SIGN (0xA293F6DB)

enum enumAicsErrorCode
{
	AICSERR_UNK		= -1,		// 未知错误.
	AICSERR_OK		=  0,		// 没有错误.
	AICSERR_NOKILL  = 0x8000,	// 算法默认没打死鱼状态.
	AICSERR_KILLED	= 0x9000,	// 算法打死鱼返回值.
};

#include <PshPack2.h>
#define  ONBKMAX  (8)	// 一次扩散打死的鱼不超过8条.
// 子弹打中鱼时,到底死不死在算法库.
typedef struct tagOnBulletKillInfo
{
	long sign;		// 数据有效性.
	long nId;		// 对象的唯一标示. 
	long nOdds;		// 当前鱼的倍率.
	long nResult;	// 鱼有没有死,TRUE/FALSE.
}   ONEBKILLINFO,*PONEBKINFO;

// 远程加难数据结构.
#define MAX_ROOMTABLE (2000)	// 一次性获取加难的最大桌台数量(因为通信缓冲区只有这么大).
typedef struct tagRemotePlusHard
{
	long sign;		// 数据有效性.
	long tableid;	// 加难桌台号.
	long hard;		// 是否有加难.
	long resv;		// 保留数据值. 
} RemotePlusHard,*PRemotePlusHard;

// 用户上下分记录.
typedef struct tagPlayerAddsubScore
{
	long	    sign;	// 数据有效性.
	long	   isadd;	// 是否是上分(1:上分,0:下分)
	long	   seqid;	// 包的流水号(用来跟踪丢包).
	long	   score;	// 用户操作分.
	long	    coin;	// 用户上下分币值.
	long	 tableid;	// 用户的桌号.
	long	  charid;	// 用户的椅号.
	char	name[32];	// 用户的用户名.
	long	 resv[2];	// 保留数据长度.
} PlayerAddsubScore;

// 洗码量同步数据结构.
typedef struct tagTableGainLostInfo
{
	long			 sign;	// 数据有效性.
	long			seqid;	// 包的流水号(用来跟踪丢包).
	long		  tableid;	// 当前桌台号.
	long		 gaincoin;	// 总进币数.
	long		 lostcoin;	// 总出币数.
	long		  resv[4];	// 保留字节(16).
	__int64		gainscore;	// 总进分数.
	__int64		lostscore;	// 总出分数.
}   TableGainLostInfo;

// 外部游戏设置参数定义.
typedef struct tagGameSettings
{
	LONG			cbSize;	// 当前数据结构的大小(版本信息).
	LONG    nTotalAddScore; //当前打码周期内机器总上分[币].
	LONG    nTotalSubScore; //当前打码周期内机器总下分[币].
	LONG    nTotallsProfit; //总利润根据上下分算出来的.
	LONG    nPreviouProfit; //上次总利润是打码时总利润[币].
	LONG    nCurrentProfit; //当前打码周期内机器的赢利.
	LONG    nTotalToubiCnt; //当前周期内投币数量.
	LONG    nTotalTuibiCnt; //当前周期内退币数量.
	LONG    nTotalTuicpCnt; //当前周期退彩票数量.
	LONG    nTotalRunnTime; //当前周期内运行时间.
	LONG    nCurrToubiBiLi; //当前机器的投币比例.
	LONG    nCurrCaiPoBiLi; //当前机器的彩票比例.
	LONG    nbIsTuiCaiPiao; //设置是退币还是彩票.[1:彩票]
	LONG       nDifficulty; //当前对应的游戏难度.
	LONG         nAreaType; //当前对应的场地类型.
	LONG       nJiapaoFudu; //当前加炮的幅度数值.
	LONG           nPaoMin; //当前炮的最小炮数值.
	LONG           nPaoMax; //当前炮的最大炮数值.
	LONG       nDamaTiansu; //当前设置的打码天数.
	LONG           nLineId; //当前机器的线号信息.
	LONG            nTaiId; //当前机器的台号信息.
	LONG   nAddScoreMultip; //当前上下分的倍率值.
	LONG        nMaxPlayer; //当前桌面的玩家数量.
	LONG		 nMaxTable;	//当前房间的桌子数量.
}   GameSettings;

#include <PopPack.h>

#ifndef AICSAPI
#define AICSAPI long __stdcall
#endif//AICSAPI

struct IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet) = 0;
	virtual AICSAPI Term() = 0;

public://设置房间ID,设置了ROOMID,算法才会加载xml.
	virtual AICSAPI SetRoomID(LONG nRoomID) = 0;

public://用户开炮跟得分记录.
	virtual AICSAPI OnPlayerFired(LONG nTableID,LONG nCharID,LONG nBulletScore) = 0;
	virtual AICSAPI OnFishCatched(LONG nTableID,LONG nCharID,LONG nScore) = 0;
	virtual AICSAPI OnSyncData(BOOL bForceSync) = 0;

public://用户上下分记录.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore) = 0;
	virtual AICSAPI OnSubScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore) = 0;

public: // 判断当前鱼到底死不死.
	virtual AICSAPI OneBulletKill(LONG nTableID,LONG nCharID,LONG nUserID,LONG nBulletScore,PONEBKINFO pList,LONG nCount) = 0;

public: // 获取总进总出.
	virtual AICSAPI GetGainLostEx(LONG nTableId,LONG* plGain,LONG* plLost,LONG* plPresent,LONG* plWincoin) {return 0;}

public: // 跟踪用户坐下/离开.
	virtual AICSAPI OnUserSitdown(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserStandup(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserUpgrade(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOldState,BOOL bNewState,BOOL bFreeroom) {return 0;}
};

class CAicsEngineNUL : public IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet)
	{
		return (AICSERR_OK);
	}
	
	virtual AICSAPI Term()
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI SetRoomID(LONG nRoomID)
	{
		return (0);
	}

public://用户开炮跟得分记录.
	virtual AICSAPI OnPlayerFired(LONG nTableID,LONG nCharID,LONG nBulletScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnFishCatched(LONG nTableID,LONG nCharID,LONG nScore) 
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSyncData(BOOL bForceAsync)
	{
		return (AICSERR_OK);
	}

public://用户上下分记录.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSubScore(LPCSTR lpszName,LONG nTableID,LONG nCharID,LONG nScore)
	{
		return (AICSERR_OK);
	}

public: // 判断当前鱼到底死不死.
	virtual AICSAPI OneBulletKill(LONG nTableID,LONG nCharID,LONG nUserID,LONG nBulletScore,PONEBKINFO pList,LONG nCount)
	{
		return (AICSERR_OK);
	}

public:	// 跟踪用户坐下/离开.
	virtual AICSAPI OnUserSitdown(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserStandup(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserUpgrade(LONG nUserID,LONG nTableID,LONG nCharID,BOOL bIs2OldUser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}
};

typedef IAicsEngine* (WINAPI* PFNCREATEAICSENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsEngine* __stdcall GetAicsEngine()
{
	static IAicsEngine* aics = NULL;
	do 
	{
		// pointer.
		if (!aics)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("AicsEngine.dll"));
			if (!hInst) {
				break;
			}

			// try to get the special function to create the interface now create the now.
			PFNCREATEAICSENGINE  pfn = (PFNCREATEAICSENGINE)GetProcAddress(hInst,"fun0000");
			if (!pfn) {
				FreeLibrary(hInst);
				break;
			}

			// interface.
			aics = pfn();
		}
	}   while (0);

	// check now.
	if (!aics) {
		 aics = new CAicsEngineNUL();
	}
//Cleanup:
	return (aics);
}

#endif//__IAICSENGINE_HEADER__