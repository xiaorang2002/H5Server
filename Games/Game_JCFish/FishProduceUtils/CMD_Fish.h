#ifndef __CMD_Fish_h__
#define __CMD_Fish_h__

#include <types.h>
#include <sys/types.h>
#include <string>
#include <vector>
using namespace std;
#ifdef  _WIN32
#include <tchar.h>
#else

//产品版本
#define BULID_VER					0							//授权版本
#define PRODUCT_VER					6							//产品版本

// defien the special process version data for later user under linux code.
#define PROCESS_VERSION(cbMainVer,cbSubVer,cbBuildVer)					\
		(DWORD)(														\
		(((BYTE)(PRODUCT_VER))<<24)+									\
		(((BYTE)(cbMainVer))<<16)+										\
		((BYTE)(cbSubVer)<<8)+											\
		(BYTE)(cbBuildVer))


#endif//_WIN32

// declare the FLOATT.
typedef double FLOATT;

enum FishProduceSideConstant {
	fpscLeft,       // 左侧出鱼.
	fpscTop,        // 顶上出鱼.
	fpscRight,      // 右侧出鱼.
	fpscBottom,     // 底部出鱼.
	fpscMax,        // 最大值.
};

enum ShoalType {
	stSingle,       // 单条鱼.
	stFollow,       // 跟随.
	stLump,         // 集结.
	stSchool,       // 鱼阵.
};

enum GameplayType {
	gptCollect = 0, // 收集玩法.
	gptMsyq,        // 马上有钱.
	gptPpl,         // 拍拍乐
	gptSameBoom,    // 同类炸弹.
	gptMax,         // 效果的最大值.
};
enum GameplayCommandType {
	gctNone,        // 
	gctStart,       // 
	gctPlaying,     // 
	gctStep,        // 
	gctEnd,         // 
	gctTimeEvent,   // 
	gctStartSync,   //
	gctPlayingSync, //
	gctEndSync,     //
	gctStepSync,    //
};

enum CMD_Type {
	// 客户端命令.
    SUB_C_EXCHANGE_SCORE=1, //进入游戏时分值同步
    SUB_C_USER_FIRE,        // 玩家开火.
	SUB_C_FISH_HIT,         // 用户击中鱼.
	SUB_C_FISH_LOCK,        // 玩家锁定.
	SUB_C_FISH_UNLOCK,      // 锁定解锁.
	SUB_C_SCORE_UP,         // 玩家上分.
	SUB_C_GAMEPLAY,         // 游戏玩法.
	SUB_C_DYNAMIC_CHAIR,    // 动态换坐.
	SUB_C_FROZEN,           // 定屏炸弹.
    SUB_C_ADDLEVEL,           // 定屏炸弹.
	SUB_C_SETSPECIALROTATE, // 设置特殊炮的方式 CMD_C_SpecialRotate
	// 服务端命令.
	SUB_S_GAME_CONFIG = 100,
	SUB_S_GAME_SCENE,       // 同步场景.
	SUB_S_SCENE_SWITCH,     // 切换场景.
	SUB_S_FISH_PRODUCE,     // 游戏出鱼
	SUB_S_FISH_ARRAY_PRODUCE, // 鱼阵出鱼.
	SUB_S_FISH_CATCH,       // 捕获某鱼
	SUB_S_FISH_PAUSE,       // 定屏炸弹.
	SUB_S_FISH_LOCK,        // 锁定某鱼
	SUB_S_FISH_UNLOCK,      // 解锁某鱼.
	SUB_S_USER_FIRE,        // 玩家开火.
	SUB_S_SCORE_UP,         // 玩家上分.
	SUB_S_WASH_PERCENT,     // 能量条累积功能.
	SUB_S_GAMEPLAY,         // 游戏玩法专用指令.
	SUB_S_EXCHANGE_SCORE,   // 进入游戏时分值同步.
	SUB_S_SCORE_UP2,        // 玩家上分2.
	SUB_S_DYNAMIC_CHAIR,    // 动态换坐.
	SUB_S_FROZEN,           // 冰冻效果
    SUB_S_ADDLEVEL,           // 冰冻效果
	SUB_S_SPECIALFIREEND,	//特殊炮发射结束
	SUB_S_ZTPSTARTROTATE,  // 钻头炮开始旋转飞行
	SUB_S_SETSPECIALROTATE,  // 设置特殊炮的方式 CMD_S_SpecialRotate
	SUB_MAX,
};

enum ScoreUpType {
	sutExchangeScore,
	sutFishScore,
	sutBulletScore,
	sutReturnedBulletScore,
	sutHandsel,
	sutGameplayCollectScore,
	sutGameplayMsyqScore,
	sutGameplayPplScore,
	sutHandselSameBomb,
	sutFishScore2,
	sutHandselBoss, //Boss Or GlobalBomb
	sutHlwBoss, //海龙王
};

enum PathType {
	ptNone,
	ptLine,
	ptCurve,
	ptCircle,
	ptDelayTime,
	ptDelayTimeWithHidden,
	ptEaseOut,
	ptRotation,
	ptMax,
};

enum FishKingType {
	fktNormal,
	fktSameBomb,		//同类炸弹    
	fktGlobalBomb,		//全屏炸弹
	fktPauseBomb,
	fktAreaBomb,		//局部炸弹
	fktCombination,		//组合鱼
	fktMSYQ,
	fktYWDJ,			//一网打尽
	fktMax,
};

typedef enum enumCustomGameMessageType {
    SMT_CUSTOM_WAITING_START = 60,  // 显示 正在同步游戏场景，请稍候...
    SMT_CUSTOM_WAITING_END,         // 关闭 正在同步游戏场景，请稍候...
    SMT_CUSTOM_WELCOME = 64,        // 显示 欢迎进入XX游戏，祝您游戏愉快！.
	SMT_CUSTOM_CONGRATULATION = 65, // 显示 恭喜玩家获得奖励.
} CustomGameMessageType;

#define INVALID_FISH_ID 0
#define MIN_FISH_ID 100
#define MAX_FISH_ID 1000
#define MAX_BULLETS 15
#define MAX_FISH_SPECIES 50
#define MAX_FISH_SPECIES_WITH_PROPS 20
#define MAX_FISH_PROPS 10

#define CMD_BUFFER_SIZE_MAX 9000
#define DECLARE_CMD_BUFFER(buffer) unsigned char buffer[CMD_BUFFER_SIZE_MAX]
#define DECLARE_FISH_ARRAY(type, fishIDs) type fishIDs[MAX_FISH_ID + 1]
#define IS_VALID_BUFFER_SIZE(size) (size <= CMD_BUFFER_SIZE_MAX)
typedef unsigned short FISH_ID; // 从1开始.
typedef unsigned char WashPercent;
typedef int BulletScoreT;

#define MAX_PLAYERS 8
#define PLAYERS_MAX 8
#define MAX_KINDID  (100)

// set the max screen width height.
#define GAME_RESOLUTION_WIDTH 1280
#define GAME_RESOLUTION_HEIGHT 720

#define FISH_VERSION_SERVER        PROCESS_VERSION(6,0,3)

#define AICS_ENGINE

#pragma pack(1)

#define COLLECT_COUNT 6
struct CollectData {
	unsigned char collectCount; // 收集boss个数(1~8).
	unsigned char collectFishId[COLLECT_COUNT]; // 需收集的所有鱼类型.
	unsigned char collectStats[MAX_PLAYERS]; // 每个玩家当前的收集状态.
};
struct MsyqData {
	WashPercent msyqPercent[MAX_PLAYERS]; // 每个台位洗码量完成情况.
};
#define FISH_ID_MSYQ 1
#define APM_PAIPAILE 10.0f

#define PALYER_SCORE_SCALE  100
struct PplXmlConfig {
	unsigned char fishType;
	unsigned short fishLife; // 打死拍拍乐先得多少分.
};
struct PplRuntimeData {
	unsigned short life;
	BulletScoreT bulletScore;
};
struct PplData {
	unsigned char isPlaying; // 玩法状态.
	unsigned short hits; // 连击数.
};

//////////////////////////////////////////////////////////////////////////
struct FishType {
	unsigned char type;
	unsigned char king;
    unsigned char Onlytype;
};

struct FishKind {
	FISH_ID id;	 // id
	FishType fishType;
	unsigned short life; // 生命值.
#ifdef __USE_LIFE2__
	unsigned short life2; // 生命值(得分牌用).
#endif
};

struct FishArrayParam1 {
	unsigned char small_fish_type;
	short small_fish_count;
	short move_speed_small;
	short move_speed_big;
	short space;
	unsigned char big_fish_kind_count[2];
	FishType big_fish[2][15];
	short big_fish_time;
};
struct FishArrayParam2 {
	unsigned char small_fish_type;
	short small_fish_count;
	short move_speed_small;
	short move_speed_big;
	unsigned char big_fish_count;
	unsigned char big_fish_kind_count;
	FishType big_fish[15];
	unsigned short big_fish_time_interval;
	unsigned short small_fish_time_interval;
};
struct CirboomParam {
	FLOATT delay;
	unsigned char fish_type;
	unsigned char count;
	unsigned short move_speed;
	unsigned short radius;
};
struct FishArrayParam3 {
	short rot_angles, rot_speed;
	FLOATT time_interval;
	unsigned char param_count;
	CirboomParam param[10];
};
struct FishArrayParam4 {
	unsigned char param_count;
	CirboomParam param[10];
};
struct Circle2Param {
	unsigned char fish_type;
	unsigned char count;
	unsigned short move_speed;
	unsigned short radius;
	unsigned char order;
};
struct FishArrayParam5 {
	short rot_angles;
	short rot_speed;
	FLOATT time_interval;
	unsigned char param_count[2];
	Circle2Param param[2][5];
};
struct CrossoutParam {
	unsigned char fish_type;
	unsigned char count;
	unsigned short radius;
};
struct FishArrayParam6 {
	short move_speed;
	short max_radius[2];
	unsigned char param_count[2];
	CrossoutParam param[2][5];
};
struct CenterOutParam {
	unsigned char fish_type;
	unsigned char count;
	unsigned char dirs;
	unsigned short move_speed;
	FLOATT interval;
};
struct FishArrayParam7 {
	unsigned char param_count;
	CenterOutParam param[8];
	FLOATT fish_time_delay, small_fish_time_interval;
};
struct FishArrayParam8 {
	unsigned char small_fish_type;
	unsigned short small_fish_count;
	unsigned short move_speed_small;
	short move_speed_big;
	unsigned char big_fish_kind_count[4];
    FishType big_fish[4][15];
	FLOATT big_fish_time_interval, small_fish_time_interval;
};
struct FishArrayParam {
	FishArrayParam1 param1;
	FishArrayParam2 param2;
	FishArrayParam3 param3;
	FishArrayParam4 param4;
	FishArrayParam5 param5;
	FishArrayParam6 param6;
	FishArrayParam7 param7;
	FishArrayParam8 param8;
};

struct CMD_C_UserFire {
	FISH_ID lockedFishID;
	BulletScoreT cannonPower;
	short angle;
	char bulletID;
	unsigned int fireType; //子弹:0 激光炮:1 钻头炮:2;
};
struct CMD_C_LockFish {
	FISH_ID fishID;
};
struct CMD_C_ScoreUp {
	char inc;
};
struct CMD_C_HitFish {
	unsigned char bulletID;
	unsigned int  hitType; //子弹:0 激光炮:1 钻头炮:2;
}; // + fishID list
struct CMD_C_Gameplay {
	unsigned char gameplayID;
	unsigned char commandType;
};
struct CMD_C_AddLevel {
    unsigned int userid;
    unsigned int powerlevel;
};
struct CMD_S_GameConfig {
    unsigned int    version;                // 版本号.
    unsigned short  ticketRatio;            // 当前机器的投币比例(渔币).
    BulletScoreT    bulletStrengtheningScale;//加炮幅度.
    BulletScoreT    cannonPowerMin;         // 最小炮数.
    BulletScoreT    cannonPowerMax;         // 最大炮数.
    unsigned short  bulletSpeed;            // 子弹飞行速度.
    unsigned short  consoleType;            // 机台类型.
    unsigned int    prizeScore;             // 爆机分数.
    unsigned int    exchangeRatioUserScore; // 兑换比率(金币).
    unsigned int    exchangeRatioFishScore; // 兑换比率(渔币).
    unsigned char   decimalPlaces;          // 小数位数
    float           revenue_ratio;          //shuishou,bili.

//	unsigned long exchangeRatio;            // 1金币=N渔币.
    long long exchangeFishScore[MAX_PLAYERS];//玩家当前上分分值.
    long long fishScore[MAX_PLAYERS];       // 玩家当前绝对分值.

    // 第二套货币系统(勋章).
#ifdef __USE_LIFE2__
	unsigned short exchangeRatioIngot; // 兑换比率(元宝)
	unsigned short exchangeRatioScore2; // 兑换比率(奖券)
	long long fishScore2[MAX_PLAYERS];
	long long exchangeFishScore2[MAX_PLAYERS];
#endif

    unsigned short broadcast_minval;  // broadcast.
	unsigned char isExperienceRoom;
	unsigned char isAntiCheatRoom;
	unsigned short timeOutQuitCheck;
	unsigned short timeOutQuitMsg;
	unsigned char fishLockable;
	FishArrayParam fishArrayParam;
	unsigned char supportedGameplays; // 支持的玩法ID，最多八个玩法

    // 动态换坐位玩法.
#ifdef __DYNAMIC_CHAIR__
	unsigned short dynamicChair[MAX_PLAYERS]; // 游戏中允许换座
#endif

// shouji wan fang.
#ifdef __GAMEPLAY_COLLECT__
    CollectData collectData;
#endif

#ifdef __GAMEPLAY_MSYQ__
	MsyqData msyqData;
#endif

#ifdef __GAMEPLAY_PAIPAILE__
	PplData pplData[MAX_PLAYERS];
	PplXmlConfig pplXmlConfig;
#endif

	unsigned int	nBaseLaserOdds;				//激光炮的基础倍数
	unsigned int	nBaseBorebitOdds;			//钻头炮的基础倍数
	unsigned int	nFlyMaxFireTime;			//钻头炮飞行的最大时间s
    unsigned int	nAreaBoomOdds;			//钻头炮的基础倍数

};

// 服务端下发玩家上分.
struct CMD_S_ScoreUp {
	long long playerScore;
	int score;
	char type;
	char playerID;
	int  specialScore; //特殊炮当前累计赢的总分
	int  hlwScore; //海龙王的得分
};
#ifdef __USE_LIFE2__
struct CMD_S_ScoreUp2 {
	int score;
	int score2;
	char type;
	char playerID;
};
#endif

// 带换算一次性全部上分指令.
struct CMD_C_Exchange_Score {
    int userID;
};

// 带换算一次性全部上分指令.
struct CMD_S_ExchangeScore {
	long long score;
#ifdef __USE_LIFE2__
	long long ingot;
#endif
	char inc;
	char playerID;
};

// 切换场景.
struct CMD_S_SwitchScene {
	unsigned char sceneID;
	unsigned char switchTimeLength;
};

// 捕获某条鱼.
struct CMD_S_CatchFish {
	char bulletID;
	char playerID;
	BulletScoreT cannonPower;
	unsigned int  hitType; //子弹:0 激光炮:1 钻头炮:2;
}; // + fishID list
struct CMD_S_PauseFish {
	unsigned char pause;
};
struct CMD_S_LockFish {
	CMD_C_LockFish c;
	char playerID;
};
struct CMD_S_UnlockFish {
	char playerID;
};
struct CMD_S_UserFire {
	CMD_C_UserFire c;
	char playerID;
};
struct CMD_S_ProduceFish {
	unsigned int randomSeed;
	short elapsed; // 单位：0.01秒.
	// id-12bits type-5 king-4 shoalType-3 isAlive-1
	FISH_ID fishID;
	FishType fishType;
	unsigned char isAlive;
};
struct CMD_S_ProduceFishShoal {
	// id-12bits shoalType-3
	FISH_ID fishID;
	short elapsed; // 单位：0.01秒.
	char shoalType;
};
struct CMD_S_ProduceFishArray {
	unsigned int randomSeed;
	// fishID(first)-12bits fishArrayID-4bits
	FISH_ID firstFishID;
    unsigned char fishArrayID;
};
struct CMD_S_PlayerInfo
{
    LONGLONG score;
    unsigned int userid;
    unsigned int chairid;
    string nikename;
    unsigned int status;
    bool isExist;
};
struct CMD_S_SpecialFire
{
    unsigned int userid;
    unsigned int chairid;
    int  FireType;				//激光炮:1 钻头炮:2
    bool bHaveSpecialFire;		//是否获得特殊炮
    bool bHaveFire;				//是否已发射
    int  FireCountTime;			//发射倒计时
    short  FireAngle;			//发射的初始角度
    double FirePassTime;		//发射已过的时间
	int    specialScore;		//特殊炮当前累计赢的总分
	double FireEndCountTime;	//钻头炮发射结束倒计时
};
struct CMD_S_HaiLongWangInfo
{
    unsigned int status;		//生命状态(0:初级  1:中级  2:高级)
	unsigned int nlife;			//当前倍数(初级300-500倍，中级500-800倍，高级800-1500倍)
	unsigned int deathstatus;	//打死的状态(0:初级  1:中级  2:高级)
	unsigned int deathlife;		//打死的倍数
	bool		 bremove;		//是否清理当前鱼
};
struct CMD_S_GameScene {
	CMD_S_GameConfig gameConfig;
	CMD_S_ProduceFishArray fishArray;
	// pause-1bit sceneID-3bits
	unsigned char sceneID;
    unsigned char pause;
    CMD_S_PlayerInfo players[8];
    float sceneTime;
    std::vector<int> fishLiveArray;
	CMD_S_SpecialFire specialFire[8];
	CMD_S_HaiLongWangInfo longwangInfo;	//海龙王信息
}; // + 头鱼数量 + 头鱼信息(CMD_S_ProduceFish) + 鱼群信息.
struct CMD_S_WashPercent {
	WashPercent percent;
	char playerID;
};
struct CMD_S_Gameplay {
	CMD_C_Gameplay c;
	char playerID;
};
#ifdef __DYNAMIC_CHAIR__ // 游戏中允许换座
struct CMD_C_DynamicChair {
	char newChairID;
};
struct CMD_S_DynamicChair {
	char newChairID;
	char playerID;
};
#endif

struct CMD_S_SpecialFireEnd {
	int  hitType; //子弹:0 激光炮:1 钻头炮:2;
	char playerID;
};

struct CMD_S_StartRotate {
	bool bStartRotate; //钻头炮开始旋转
	int  chairid;
};

struct CMD_C_SpecialRotate {
    short angle;
    unsigned int hitType; //子弹:0 激光炮:1 钻头炮:2;
};
struct CMD_S_SpecialRotate {
	unsigned int chairid;
	short angle;
	unsigned int hitType; //子弹:0 激光炮:1 钻头炮:2;
};


#pragma pack()

struct FishShoalInfo {
	FishType fishType;
	unsigned short genRatio;
	unsigned short amountMin;
	unsigned short amountMax;
	char allowFollow; // 跟随.
	char allowLump; // 鱼群.
};
struct FishSwimmingParam {
	char pathType;
	short speed;
	unsigned short minLength;
	unsigned short maxLength;
	short minRotation;
	short maxRotation;
};
struct FishSwimmingPattern {
	FishType fishType;
	unsigned char paramDataCount;
	FishSwimmingParam swimmingParam[5];
};
struct FishBasicData {
	FishType fishType;
	unsigned short lifeMin;
	unsigned short lifeMax;
#ifdef __USE_LIFE2__
	unsigned short life2Min;
	unsigned short life2Max;
#endif
	unsigned short boundingWidth;
	unsigned short boundingHeight;
	TCHAR fishName[200];
};

struct FishPathCommonData {
	unsigned char fishPathDataSize;
	short initPositionX, initPositionY;
	unsigned char pathCount;
};

struct FishPoint {
	short x, y;
};
struct PathLineInfo {
	FishPoint deltaPoint;
};
#define MAX_CURVE_POINTS 100
struct PathCurveInfo {
	FishPoint point[MAX_CURVE_POINTS];
	unsigned char pointCount; // 坐标点个数.
};
struct PathCircleInfo {
	FishPoint deltaPoint;
	short rotateSpeed; // 旋转速度.
};
struct PathEaseOutInfo {
	FishPoint deltaPoint;
};
struct PathRotationInfo {
	short initAngle;
	short rotateSpeed; // 旋转速度.
};
#define MAX_FISH_PATH 10
struct FishPathParam {
	PathType pathType;
	FLOATT duration;
	union {
		PathLineInfo pathLine;
		PathCurveInfo pathCurve;
		PathCircleInfo pathCircle;
		PathEaseOutInfo pathEaseOut;
		PathRotationInfo pathRotation;
	} data;
};

struct PathCurveParam {
	unsigned char pointCount; // 坐标点最大个数.
	short moveLengthMin, moveLengthMax; // 最短(长)移动距离.
	short deltaAngleMin, deltaAngleMax; // 最小(大)转弯角度.
};
struct PathCircleParam {
	short deltaAngle; // 旋转度数.
	FishPoint deltaPoint;
};
struct PathEaseOutParam {
	FishPoint point;
};

#define SET_GAMEPLAY(supportedGameplays, gameplayType) (supportedGameplays |= 1 << (gameplayType))
#define GET_GAMEPLAY(supportedGameplays, gameplayType) (((supportedGameplays) >> (gameplayType)) & 1)
#define getKeyWithFishType(fishType) ((fishType).king * 60 + (fishType).type)
#define isValidFishID(fishID) (fishID != INVALID_FISH_ID)
#define CheckFishKing(king) ((king) > fktNormal)
#define CheckFishBomb(king) ((king) >= fktGlobalBomb && (king) <= fktAreaBomb)
#define CheckPlayerID(id) ((id) >= 0 && (id) < MAX_PLAYERS)
#define CheckBulletID(id) ((id) >= 0 && (id) < MAX_BULLETS)
#define CheckCannonPower(cannonPower, cannonPowerMin, cannonPowerMax) ((cannonPower) >= (cannonPowerMin) && (cannonPower) <= (cannonPowerMax))
#define CreateSignleton(className, pInstance) \
private: \
	static className* pInstance; \
public: \
	static className* getInstance() {		\
		if (!pInstance) {					\
			pInstance = new className();	\
		}									\
		return pInstance;					\
	}										\
	static void destroyInstance() {	\
		if (pInstance) {		\
			delete pInstance;	\
			pInstance = 0;		\
		}						\
	}
#define InitilaizeSignletonInstance(className, pInstance) className* className::pInstance = 0;
#endif // __CMD_Fish_h__
