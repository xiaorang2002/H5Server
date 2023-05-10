#ifndef ERRORCODESET_H
#define ERRORCODESET_H

/************************************************
                   请求的返回码
*************************************************/
enum class eErrorCode
{
    /************************************************
         TransferServer  请求的返回码
    *************************************************/
    // 执行成功
    NoError                     = 0,
    // 服务停止中
    ServerStopError             = 1, 
    // 传输参数错误
    ParamsError                 = 2,
    // 玩家账号不存在
    UserNotExistsError          = 3,
    // 玩家余額不足，不能进入游戏
    UserInsufficientBalance     = 4,
    // 玩家不能进入自家的游戏
    UserCanNotBeEnterGame       = 5,  
    // 非白名单
    WhiteIPError                = 6,
    // 并发过高
    MaxConnError                = 7,
    // 试玩访问频率太高
    HighFreqError               = 8,
    // 试玩访问频率太高
    MaxFreqError                = 9,
    // 内部异常或未执行任务
    InsideErrorOrNonExcutive    = 10,
    // 获取Json出错
    GetJsonInfoError            = 11,
    // 此代理没开放视讯功能
    PermissionDenied            = 12,
    // 玩家在线
    PlayerOnLine                = 13,
    // 游戏暂未开放
    NotOpen                     = 14,
    // 游戏分数被锁定
    LockScore                   = 15,
    // 游戏分数解锁失败
    UnLockFailer                = 16,
    // 游戏不存在
    NotExist                    = 17, //does not exist
    // 余额为零，无需取回
    NotNeedUpdate               = 18, //does not exist
    // 请求超时
    TimeOut                     = 19, //does not exist
    // 操作过于频繁
    ReqFreqError                = 20, //does not exist

    /*****************LotteryServer*********************/ 
    // 平台信息不正确
    PlatframCfgErr              = 100,
    // 转帐分数值不正确
    OperateScoreErr             = 101,
    // 玩家分数不足
    ScoreInsufficient           = 102,
    /****************************************/ 
    // 网络错误
    StatusCodeNot200Error       = 400,

    /************************************************
         LoginServer  请求的返回码
    *************************************************/





};

/************************************************
                请求的相关枚举
*************************************************/
 
enum class eReqType
{
    OpUnknown = -1,
	OpTransfer = 0
};

enum class eOrderType
{
	OpDoing = 0,
	OpFinish = 1,
	OpTiemOut = 2,//其它错误
	OpJsonErr = 3,
	OpUndefind = 4
};

enum class eLogLvl {
    LVL_0 = 0,
    LVL_1,
    LVL_2,
    LVL_3,
    LVL_4,
    LVL_5
};

enum class eRecycleType
{
    all  = 0,         
    one  = 1,          
};

#endif // ERRORCODESET_H
