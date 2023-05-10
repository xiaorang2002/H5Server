#ifndef CMD_SGJ_HEAD_FILE
#define CMD_SGJ_HEAD_FILE


#define GAME_NAME                               "水果机"                        //游戏名字
#define KIND_ID                                 910								//游戏 I D

#define MIN_GAME_PLAYER							1								//最小游戏人数

#define TO_DOUBLE                               1

//游戏状态
#define GAME_STATUS_INIT                        0                               //INIT status
#define GAME_STATUS_READY						100								//空闲状态
#define GAME_STATUS_END                         105								//游戏结束

#define DEBUG_INFO                               0
#define RUN_ALGORITHM                            1
#include <vector>
using namespace std;
enum enErrorCode
{
    ERROR_BET_SCORE         = 1,				//下注错误
    ERROR_LOW_SCORE		    = 2,			    //下注太大筹码错误
    ERROR_GET_FRUIT		    = 3,				//获取玛丽水果错误
    ERROR_SCORE_NOT_ENOUGH  = 4,				//金币不足
    ERROR_INI_ERROR		    = 5,				//服务器配置错误
    ERROR_SIT_CHAIR_ID		= 6,				//椅子id错误
    ERROR_STATUS			= 7,				//当前状态下不允许操作
    ERROR_MESSAGE			= 8,				//消息错误
    ERROR_OPERATE			= 9,				//操作错误
    ERROR_OPERATE_TIMEOUT   = 10,				//长时间不操作长时间不操作退出游戏
};

struct CMD_S_Result
{
    CMD_S_Result()
    {
        fruitIndex=0;
        fruitNum=0;
        mutical=0;
        isAll=0;
    }
    int fruitIndex;
    int fruitNum;
    int mutical;
    int isAll;
};
//消息
struct CMD_S_RoundResult_Tag
{
    CMD_S_RoundResult_Tag()
    {

    }
	int64_t	Score;							//中了多少分
	int64_t	lCurrentScore;					//当前携带金币
    int32_t	nMarryNum;						//剩余玛丽数量
    int32_t nResultIndex;                   //跑灯结果下标
};

#endif // CMD_SGJ_HEAD_FILE
