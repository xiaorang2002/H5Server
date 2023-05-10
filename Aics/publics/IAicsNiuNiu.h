

#ifndef __IAOCSOX_HEADER__
#define __IAOCSOX_HEADER__

#ifdef  _WIN32
#include <windows.h>
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
#define AICSOX_SIGN (0x3D7B5876)

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
enum eOxErrCodes
{
    AICSOXERR_UNK	 = -1,		// 未知错误.
    AICSOXERR_OK	 =  0,		// 没有错误.
    AICSOXERR_NOKILL =  0,	    // 算法默认没打死鱼状态.
    AICSOXERR_KILLED =  0x90,	// 算法打死鱼返回值.
};

// 每一门下标. 
enum eOxGateCode
{
    OXGATE_0 = 0,               //庄 
    OXGATE_1 = 1 ,              //天
    OXGATE_2 = 2,               //地
    OXGATE_MAX_2GATE = 3,       // 三门最大值.
    OXGATE_3 = 3,               //玄
    OXGATE_4,                   //黄

    OXGATE_MAX
};

// 牌型标示
enum enumPocketType
{
    OXTYPE_ERROR = -1,          //错误.
    OXTYPE_NOOX  = 0,           //没牛
    OXTYPE_OX1,                 //牛1
    OXTYPE_OX2,                 //牛2
    OXTYPE_OX3,                 //牛3
    OXTYPE_OX4,                 //牛4
    OXTYPE_OX5,                 //牛5
    OXTYPE_OX6,                 //牛6
    OXTYPE_OX7,                 //牛7
    OXTYPE_OX8,                 //牛8
    OXTYPE_OX9,                 //牛9
    OXTYPE_OX10,                //牛10
    //最大牌型.
    OXTYPE_MAX,

    OXTYPE_FIVE = 11,           //五花牛[可选]
    OXTYPE_FIVEMAX              //五花牛最大.

};

// 每一门参数配置.
struct tagOxGateIn
{
    long   sign;	             // 数据有效性(in).
    double lscoreplaced;         // 玩家下了多少分(in).
};

struct tagOxGateOut
{
    long   sign;
    double lscorewined;          // 玩家总嬴分(out).
    byte   pocket;               // 牌类型(11种中的一种)
    byte   multi;                // 当前牌型倍数[赔率].
    byte   iswin;	             // 是否中这一门(out).
    byte   isDirty;              // 数据是否被算法干预(脏数据)
    byte   resv[3];              // 保留数据.
};

// 所有门信息汇总.
struct tagOxGateInfo
{
    byte               bisBanker;   // 是否系统做庄(>0=是)
    tagOxGateIn   in[OXGATE_MAX];
    tagOxGateOut out[OXGATE_MAX];
};

// 桌台配置信息.

// ox游戏内部参数配置.
struct tagOXGs
{
    uint32       nToubbili;         //转换比例(1:1).           
    uint32       nMaxPlayer;        //原指桌面的玩家数量,现在改成天地玄黄四门数量.
    uint32       nMaxTable;         //当前房间的桌子数量.
};

// OX引擎接口.
struct IAicsOXEngine
{
public: // 初始化
    virtual AICSAPI Init(tagOXGs* pGs) = 0;
    virtual AICSAPI Term() = 0;

public://选择房间.
    virtual AICSAPI SetRoomID(int32 roomid) = 0;

public: // 出牌计算.
    virtual AICSAPI oxGetLastPocket(uint32 tabid, uint32 id,tagOxGateInfo* pInfo) = 0;
    virtual AICSAPI oxOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagOxGateInfo* pInfo) = 0;

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plAllLost, double* plTaxedLost, double* plPresent, double* plWincoin) = 0;
	virtual AICSAPI GetTexPresent(uint32 tabid, double* plPresent,BOOL bSubit=FALSE) = 0;
    virtual AICSAPI OnGivenScore(uint32 uid, uint32 tabid, uint32 chairid, double nGivescore) = 0;

    // 用户状态
    virtual AICSAPI OnUserSitdown(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserStandup(uint32 uid,uint32 tabid, uint32 chairid, bool boldplayer, bool bromfree) = 0;
    virtual AICSAPI OnUserUpgrade(uint32 uid, uint32 tabid, uint32 chairid, uint32 lstStat, uint32 newStat, bool bromfree) = 0;

    // 判断当前状态(0=外接库不存在,使用EngineNUL.).
    virtual AICSAPI getDllState() = 0;
};


// 记录上一手牌.
struct tagOXList
{
    uint32 uid;
    tagOxGateInfo tag;
};
#include <vector>
typedef std::vector<tagOXList> vtagOXList;

#define  _RAND_TEST_ 1

// 简单接口类,如果dll不存在,那么返回这个接口.
class CAicsOXEngineNUL : public IAicsOXEngine
{
protected:
    	vtagOXList m_vtagOXList;

public: // 初始化
    virtual AICSAPI Init(tagOXGs* pGs)
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
    virtual AICSAPI oxGetLastPocket(uint32 tabid, uint32 id,tagOxGateInfo* pInfo)
    {
        return (sDllError);
    }

    virtual AICSAPI oxOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagOxGateInfo* pInfo)
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

    // 判断当前状态(0=外接库不存在,使用EngineNUL).
    virtual AICSAPI getDllState()
    {
        return 0;
    }
};

typedef IAicsOXEngine* (WINAPI* PFNCREATEAICSOXENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsOXEngine* WINAPI GetAicsOXEngine()
{
    char buf[300]={0};
    static IAicsOXEngine* aics = 0;
    do 
    {
        // pointer.
        if (!aics)
        {
            getcwd(buf,sizeof(buf));
            strcat(buf,TEXT("/libAicsNiuNiuEngine.so"));
            HINSTANCE hInst=LoadLibrary(buf);
            if (!hInst) {
                break;
            }

            // try to get the special function to create the interface now create the function now.
            PFNCREATEAICSOXENGINE  pfn = (PFNCREATEAICSOXENGINE)GetProcAddress(hInst,"fun0001");
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
        aics = new CAicsOXEngineNUL();
    }
//Cleanup:
    return (aics);
}


#ifdef  _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32



#endif//__IAOCSOX_HEADER__
