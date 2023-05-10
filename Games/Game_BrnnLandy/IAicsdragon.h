#ifndef __IAOCSDRAGON_HEADER__
#define __IAOCSDRAGON_HEADER__


#ifdef  _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <dlfcn.h>
#endif//_WIN32

#ifdef _WIN32
#pragma pack(push,2)
#else
#pragma pack(2)
#endif//_WIN32


#define sOk         (0)
#define sDllError   (-1001)
#define AICSDG_SIGN (0xDA33E374)

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
}RemotePlusHard, *PRemotePlusHard;

// 错误代码.
enum eDragonErrCodes
{
    AICSDGERR_UNK	 = -1,		// 未知错误.
    AICSDGERR_OK	 =  0,		// 没有错误.
    AICSDGERR_NOKILL =  0,	    // 算法默认没打死鱼状态.
    AICSDGERR_KILLED =  0x90,	// 算法打死鱼返回值.
};

// 三门押注. 
enum eDGGateCode
{
    DGGATE_0 = 0,               //和
    DGGATE_1 = 1,               //long
    DGGATE_2 = 2,               //hu

    DGGATE_MAX = 3,             //
    DGGATE_3  = 3,              //庄/双大
    DGGATE_4  = 4,              //闲/双大
    DGGATE_FIVEMAX = 5          //五门最大值.
};

// 牌型标示
enum enumPocketType
{
    PKTYPE_A  = 0,          //A最小
    PKTYPE_2,               //2
    PKTYPE_3,               //3
    PKTYPE_4,               //4
    PKTYPE_5,               //5
    PKTYPE_6,               //6
    PKTYPE_7,               //7
    PKTYPE_8,               //8
    PKTYPE_9,               //9
    PKTYPE_10,              //10
    PKTYPE_J,               //J
    PKTYPE_Q,               //Q
    PKTYPE_K,               //K
    //最大牌型.
    PKTYPE_MAX,
};

// 每一门参数配置.
// size = 4 + 8 = 12
struct tagDGGateIn
{
    int32   sign;		         // 数据有效性(in).
    double  lscorePlaced;        // 玩家下了多少分(in).
};

// 输出返回值列表.
// size = 4 + 8 + 1 + 1 + 1 + 1 = 16;
struct tagDGGateOut
{
    int32   sign;                 // 数据有效性(in).
    double  lscoreWinned;         // 玩家总嬴分(out).
    uint8   pocket;               // 牌类型(11种中的一种)
    double  multi;                // 当前牌型倍数[赔率].
    uint8   iswin;	              // 是否中这一门(out).
    uint8   resv;
};

// 所有输入信息汇总.
// size = 12 + 16 + 4 + 32 = 64
struct tagDGGateInfo
{
    tagDGGateIn   in[DGGATE_FIVEMAX];
    tagDGGateOut  out[DGGATE_FIVEMAX];
    uint32        outwingate; // 中奖类型(0=和).
    uint32        resv[8];
};

// ox游戏内部参数配置.
struct tagDGSettings
{
    uint32       nToubbili;         //转换比例(1:1).           
    uint32       nMaxPlayer;        //原指桌面的玩家数量,现在改成天地玄黄四门数量.
    uint32		 nMaxTable;	        //当前房间的桌子数量.
};

// OX引擎接口.
struct IAicsDragonEngine
{
public: // 初始化
    virtual AICSAPI Init(tagDGSettings* pGs) = 0;
    virtual AICSAPI Term() = 0;

public://选择房间.
    virtual AICSAPI SetRoomID(int32 nRoomID) = 0;

public: // 出牌计算.
    virtual AICSAPI dragonOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagDGGateInfo* pInfo) = 0;

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
class CAicsDGEngineNUL : public IAicsDragonEngine
{
public: // 初始化
    virtual AICSAPI Init(tagDGSettings* pGs)
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

public:
	virtual AICSAPI dragonOnebk(uint32 uid, uint32 tabid, uint32 chairid, tagDGGateInfo* pInfo)
	{
		return (sDllError);
	}

    // 获取嬴亏参数.
    virtual AICSAPI GetGainLost(uint32 tabid, double* plGain, double* plALost, double* plTaxedLost, double* plPresent, double* plWincoin)
    {
        return (sDllError);
    }

	virtual AICSAPI GetTexPresent(uint32 tabid,double *plPresent,BOOL bGetout)
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

typedef IAicsDragonEngine* (WINAPI* PFNCREATEAICSDRAGONENGINE)();
// 加载算法引擎,并初始化需要的相关参数以备用.
inline IAicsDragonEngine* WINAPI GetAicsDragonEngine()
{
    char buf[300]={0};
    static IAicsDragonEngine* aics = 0;
    do 
    {
        // pointer.
        if (!aics)
        {
            getcwd(buf,sizeof(buf));
            strcat(buf,TEXT("/libAicsDragonEngine.so"));
            HINSTANCE hInst=LoadLibrary(buf);
            if (!hInst)
            {
                cout<<"load library libAicsDragonEngine.so failed!";
                break;
            }

            // try to get the special function to create the interface now create the function item now.
            PFNCREATEAICSDRAGONENGINE  pfn = (PFNCREATEAICSDRAGONENGINE)GetProcAddress(hInst,"fun0002");
            if (!pfn)
            {
                cout<<"get function of fun0002 failed";
                FreeLibrary(hInst);
                break;
            }

            // interface.
            aics = pfn();
        }
    } while (0);

    // check now.
    if (!aics)
    {
        aics = new CAicsDGEngineNUL();
    }
//Cleanup:
    return (aics);
}



#ifdef _WIN32
#pragma pack(pop)
#else
#pragma pack()
#endif//_WIN32



#endif//__IAOCSDRAGON_HEADER__
