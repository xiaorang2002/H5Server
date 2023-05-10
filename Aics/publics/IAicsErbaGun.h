

#ifndef __IAOCSERBAGUN_HEADER__
#define __IAOCSERBAGUN_HEADER__

#ifdef  _WIN32
#include <windows.h>
#include <types.h>
#else
#include <types.h>
#include <dlfcn.h>
#include <string.h>
#include <unistd.h>
#endif//_WINe32

#ifdef _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32


#define sOk         (0)
#define sDllError   (-1001)
#define AICSERBA_SIGN (0x2E8EBF28)

// server sign for remote plus hard.
#define AICS_SERVER_SIGN (0xA293F6DB)

#ifdef  _WIN32
#define AICSAPI int32 __stdcall
#define STDCALL	__stdcall
#else
#define AICSAPI int32
#define STDCALL
#endif//_WIN32

// 远程加难数据结构.
#define MAX_ROOMTABLE (2000)	// 一次性获取加难的最大桌台数量(因为通信缓冲区只有这么大).
typedef struct tagRemotePlusHard
{
	int32 sign;		// 数据有效性.
	int32 tabid;	// 加难桌台号.
	int32 hard;		// 是否有加难.
	int32 resv;		// 保留数据值. 
} RemotePlusHard,*PRemotePlusHard;

// 错误代码.
enum eErbaErrCodes
{
    AICSERBAERR_UNK	 = -1,		// 未知错误.
    AICSERBAERR_OK	 =  0,		// 没有错误.
    AICSERBAERR_NOKILL =  0,	// 算法默认没打死鱼状态.
    AICSERBAERR_KILLED =  0x90,	// 算法打死鱼返回值.
};

// 每一门下标. 
enum eErbaGateCode
{
    ERBAGATE_0 = 0,               //庄 
    ERBAGATE_1,                   //天
    ERBAGATE_2,                   //地
    ERBAGATE_3,                   //玄

    ERBAGATE_MAX
};

// 每一门参数配置.
struct tagErbaGateIn
{
    int32  sign;	             // 数据有效性(in).
    double lscoreplaced;         // 玩家下了多少分(in).
};

struct tagErbaGateOut
{
    int32  sign;
    double lscorewined;          // 玩家总嬴分(out).
    byte   multi;                // 当前牌型倍数[赔率].
    byte   iswin;	             // 是否中这一门(out).
    byte   isDirty;              // 数据是否被算法干预(脏数据)
    byte   pocket;               // pocket value.  
    byte   resv[3];              // 保留数据.
};

// 所有门信息汇总.
// size = 12 + 16 + 4 + 32 = 64
struct tagErbaGateInfo
{
    tagErbaGateIn     in[ERBAGATE_MAX];	// 玩家总下注.
    tagErbaGateIn  botin[ERBAGATE_MAX];	// 机器人总下注.
    tagErbaGateOut   out[ERBAGATE_MAX]; // 输赢总赔分.
    uint32               bisUserBanker; // 是否玩家坐庄(玩家坐庄时,in请传入机器人总押分值).
    uint32                     resv[7];
};

// 桌台配置信息.

// ox游戏内部参数配置.
struct tagErbaGs
{
    uint32       nToubbili;         //转换比例(1:1).           
    uint32       nMaxPlayer;        //原指桌面的玩家数量,现在改成天地玄黄四门数量.
    uint32       nMaxTable;         //当前房间的桌子数量.
};

// OX引擎接口.
struct IAicsErbaEngine
{
public: // 初始化
    virtual AICSAPI Init(tagErbaGs* pGs) = 0;
    virtual AICSAPI Term() = 0;

public://选择房间.
    virtual AICSAPI SetRoomID(int32 roomid) = 0;

public: // 出牌计算.
    virtual AICSAPI erbaOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagErbaGateInfo* pInfo) = 0;

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plAllLost, double* plTaxedLost, double* plPresent, double* plWincoin) = 0;
	virtual AICSAPI GetTexPresent(uint32 tabid, double* plPresent,BOOL bSubit=FALSE) = 0;
    virtual AICSAPI OnGivenScore(uint32 uid, uint32 tabid, uint32 chairid, double nGivescore) = 0;

    // 用户状态
    virtual AICSAPI OnUserSitdown(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserStandup(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserUpgrade(uint32 uid, uint32 tabid, uint32 chairid, uint32 lstStat, uint32 newStat, bool bromfree) = 0;

};


// 简单接口类,如果dll不存在,那么返回这个接口.
class CAicsErbaEngineNUL : public IAicsErbaEngine
{
public: // 初始化
    virtual AICSAPI Init(tagErbaGs* pGs)
    {
        return (sDllError);
    }

    virtual AICSAPI Term()
    {
        return (sDllError);
    }

public://选择房间.
    virtual AICSAPI SetRoomID(int32 roomid)
    {
        return (sDllError);
    }

public: // 出牌计算.
    virtual AICSAPI erbaOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagErbaGateInfo* pInfo)
    {
        return (sDllError);	
    }

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plALost, double* plTaxedLost, double* plPresent, double* plWincoin)
    {
        return (sDllError);
    }

	// get out the tex present value now.
	virtual AICSAPI GetTexPresent(uint32 ntabid,double* plPresent,BOOL bGetout/* =FALSE */)
	{
		return (sDllError);
	}

    virtual AICSAPI OnGivenScore(uint32 uid, uint32 tabid, uint32 chairid, double nGivescore)
    {
        return (sDllError);
    }

    // 用户状态
    virtual AICSAPI OnUserSitdown(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree)
    {
        return (sDllError);
    }

    virtual AICSAPI OnUserStandup(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree)
    {
        return (sDllError);
    }

    virtual AICSAPI OnUserUpgrade(uint32 uid, uint32 tabid, uint32 chairid, uint32 lstStat, uint32 newStat, bool bromfree)
    {
        return (sDllError);
    }

};

typedef IAicsErbaEngine* (WINAPI* PFNCREATEAICSERBAENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsErbaEngine* WINAPI GetAicsErbaEngine()
{
    char buf[300]={0};
    static IAicsErbaEngine* aics = 0;
    do 
    {
        // pointer.
        if (!aics)
        {
#ifdef _WIN32
			GetCurrentDirectory(sizeof(buf),buf);
			strcat(buf, TEXT("\\AicsErbaGunEngine.dll"));
#else
			getcwd(buf,sizeof(buf));
			strcat(buf, TEXT("/libAicsErbaGunEngine.so"));
#endif//_WIN32
            HINSTANCE hInst=LoadLibrary(buf);
            if (!hInst) {
                break;
            }

            // try to get the special function to create the interface now create the function now.
            PFNCREATEAICSERBAENGINE  pfn = (PFNCREATEAICSERBAENGINE)GetProcAddress(hInst,"fun0001");
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
        aics = new CAicsErbaEngineNUL();
    }
//Cleanup:
    return (aics);
}


#ifdef  _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32



#endif//__IAOCSERBAGUN_HEADER__
