/*
	客户引擎接口文件.

History:
	2016-05-27 17:00 
	增加OnUserSitdown/OnUserStandup,跟踪新用户.
*/

#ifndef __IAICSFISHENGINE_HEADER__
#define __IAICSFISHENGINE_HEADER__

#include <types.h>
#include "logger/LogWarn.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif//_WIN32

#define AICS_DATA_SIGN (0xA293F6DB)

enum enumAicsErrorCode
{
	AICSERR_UNK		= -1,		// 未知错误.
	AICSERR_OK		=  0,		// 没有错误.
	AICSERR_NOKILL  = 0x8000,	// 算法默认没打死鱼状态.
	AICSERR_KILLED	= 0x9000,	// 算法打死鱼返回值.
};

#ifdef _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32

#define  ONBKMAX  (8)	// 一次扩散打死的鱼不超过8条.
// 子弹打中鱼时,到底死不死在算法库.
typedef struct tagOnBulletKillInfo
{
	int32 sign;		// 数据有效性.
	int32 nId;		// 对象的唯一标示. 
	int32 nOdds;	// 当前鱼的倍率.
	int32 nResult;	// 鱼有没有死,TRUE/FALSE.
}   ONEBKILLINFO,*PONEBKINFO;

// 远程加难数据结构.
#define MAX_ROOMTABLE (2000)	// 一次性获取加难的最大桌台数量(因为通信缓冲区只有这么大).
typedef struct tagRemotePlusHard
{
	int32 sign;		// 数据有效性.
	int32 tableid;	// 加难桌台号.
	int32 hard;		// 是否有加难.
	int32 resv;		// 保留数据值. 
} RemotePlusHard,*PRemotePlusHard;

// 用户上下分记录.
typedef struct tagPlayerAddsubScore
{
	int32	    sign;	// 数据有效性.
	int32	   isadd;	// 是否是上分(1:上分,0:下分)
	int32	   seqid;	// 包的流水号(用来跟踪丢包).
	int32	   score;	// 用户操作分.
	int32	    coin;	// 用户上下分币值.
	int32	 tableid;	// 用户的桌号.
	int32	  charid;	// 用户的椅号.
	char	name[32];	// 用户的用户名.
	int32	 resv[2];	// 保留数据长度.
} PlayerAddsubScore;

// 洗码量同步数据结构.
typedef struct tagTableGainLostInfo
{
	int32			 sign;	// 数据有效性.
	int32			seqid;	// 包的流水号(用来跟踪丢包).
	int32		  tableid;	// 当前桌台号.
	int32		 gaincoin;	// 总进币数.
	int32		 lostcoin;	// 总出币数.
	int32		  resv[4];	// 保留字节(16).
	__int64		gainscore;	// 总进分数.
	__int64		lostscore;	// 总出分数.
}   TableGainLostInfo;

// 外部游戏设置参数定义.
typedef struct tagGameSettings
{
	int32			cbSize;	 // 当前数据结构的大小(版本信息).
	int32    nTotalAddScore; //当前打码周期内机器总上分[币].
	int32    nTotalSubScore; //当前打码周期内机器总下分[币].
	int32    nTotallsProfit; //总利润根据上下分算出来的.
	int32    nPreviouProfit; //上次总利润是打码时总利润[币].
	int32    nCurrentProfit; //当前打码周期内机器的赢利.
	int32    nTotalToubiCnt; //当前周期内投币数量.
	int32    nTotalTuibiCnt; //当前周期内退币数量.
	int32    nTotalTuicpCnt; //当前周期退彩票数量.
	int32    nTotalRunnTime; //当前周期内运行时间.
	int32    nCurrToubiBiLi; //当前机器的投币比例.
	int32    nCurrCaiPoBiLi; //当前机器的彩票比例.
	int32    nbIsTuiCaiPiao; //设置是退币还是彩票.[1:彩票]
	int32       nDifficulty; //当前对应的游戏难度.
	int32         nAreaType; //当前对应的场地类型.
	int32       nJiapaoFudu; //当前加炮的幅度数值.
	int32           nPaoMin; //当前炮的最小炮数值.
	int32           nPaoMax; //当前炮的最大炮数值.
	int32       nDamaTiansu; //当前设置的打码天数.
	int32           nLineId; //当前机器的线号信息.
	int32            nTaiId; //当前机器的台号信息.
	int32   nAddScoreMultip; //当前上下分的倍率值.
	int32        nMaxPlayer; //当前桌面的玩家数量.
	int32		  nMaxTable;	 //当前房间的桌子数量.
}   GameSettings;


#ifdef  _WIN32
#define AICSAPI int32 __stdcall
#else
#define AICSAPI int32
#endif//_WIN32

// declare the callback type for data arrived.
typedef void (WINAPI* pfnNetCallback)(WORD seqid, WORD mainid, WORD subid, WORD status, LPVOID data);
struct IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet) = 0;
	virtual AICSAPI Term() = 0;

public://设置房间ID,设置了ROOMID,才会加载配置.
	virtual AICSAPI SetRoomID(int32 nRoomID) = 0;

public://用户开炮跟得分记录.
	virtual AICSAPI OnPlayerFired(int32 nTableID,int32 nCharID,int32 nBulletScore) = 0;
	virtual AICSAPI OnFishCatched(int32 nTableID,int32 nCharID,int32 nScore) = 0;
	virtual AICSAPI OnSyncData(BOOL bForceSync) = 0;

public://用户上下分记录.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,int32 nTableID,int32 nCharID,int32 nScore) = 0;
	virtual AICSAPI OnSubScore(LPCSTR lpszName,int32 nTableID,int32 nCharID,int32 nScore) = 0;

public: // 判断当前鱼到底死不死.
	virtual AICSAPI OneBulletKill(int32 nTableID,int32 nCharID,int32 nUserID,int32 nBulletScore,PONEBKINFO pList,int32 nCount) = 0;

public: // 获取总进总出.
	virtual AICSAPI GetGainLostEx(int32 nTableId,int32* plGain,int32* plLost,int32* plPresent,int32* plWincoin) {return 0;}

public: // 跟踪用户坐下/离开.
	virtual AICSAPI OnUserSitdown(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserStandup(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bIsOldUser,BOOL bFreeroom) {return 0;}
	virtual AICSAPI OnUserUpgrade(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bOldState,BOOL bNewState,BOOL bFreeroom) {return 0;}

public: // 更新网络状态
    virtual AICSAPI Update() {return 0;}
};

class CAicsEngineNUL : public IAicsEngine
{
public:
	virtual AICSAPI Init(GameSettings* pSet)
	{
        logWarn("CAicsEngineNUL init called.");
		return (AICSERR_OK);
	}
	
	virtual AICSAPI Term()
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI SetRoomID(int32 nRoomID)
	{
		return (0);
	}

public://用户开炮跟得分记录.
	virtual AICSAPI OnPlayerFired(int32 nTableID,int32 nCharID,int32 nBulletScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnFishCatched(int32 nTableID,int32 nCharID,int32 nScore) 
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSyncData(BOOL bForceAsync)
	{
		return (AICSERR_OK);
	}

public://用户上下分记录.
	virtual AICSAPI OnAddScore(LPCSTR lpszName,int32 nTableID,int32 nCharID,int32 nScore)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnSubScore(LPCSTR lpszName,int32 nTableID,int32 nCharID,int32 nScore)
	{
		return (AICSERR_OK);
	}

public: // 判断当前鱼到底死不死.
	virtual AICSAPI OneBulletKill(int32 nTableID,int32 nCharID,int32 nUserID,int32 nBulletScore,PONEBKINFO pList,int32 nCount)
	{
		return (AICSERR_OK);
	}

public:	// 跟踪用户坐下/离开.
	virtual AICSAPI OnUserSitdown(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserStandup(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bOlduser,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}

	virtual AICSAPI OnUserUpgrade(int32 nUserID,int32 nTableID,int32 nCharID,BOOL bOldState,BOOL bNewState,BOOL bFreeroom)
	{
		return (AICSERR_OK);
	}
};

typedef IAicsEngine* (WINAPI* PFNCREATEAICSENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsEngine* WINAPI GetAicsEngine()
{
	static IAicsEngine* aics = 0;
	do 
	{
		// pointer.
		if (!aics)
		{
			HINSTANCE hInst=LoadLibrary(TEXT("libAicsEngineLinux.so"));
			// dump log for load library if success or failed.
            logWarn("Load libAicsEngineLinux.so hInst:[%x]",hInst);
			// check load.
			if (!hInst) {
                logError("Load libAicsEngineLinux.so failed!");
				break;
			}

			//// try to get the special function to create the interface now create the now.
			PFNCREATEAICSENGINE  pfn = (PFNCREATEAICSENGINE)GetProcAddress(hInst,"fun0000");
            logInfo("get fun0000:[%x]",pfn);
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

#ifdef  _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32


#endif//__IAICSENGINE_HEADER__
