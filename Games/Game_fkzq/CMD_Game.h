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

enum enErrorCode
{
    ERROR_BET_SCORE         = 1,				//下注筹码错误
    ERROR_LOW_SCORE		    = 2,			    //下注筹码错误
    ERROR_GET_FRUIT		    = 3,				//获取玛丽水果错误
    ERROR_SCORE_NOT_ENOUGH  = 4,				//金币不足
    ERROR_INI_ERROR		    = 5,				//服务器配置错误
    ERROR_SIT_CHAIR_ID		= 6,				//椅子id错误
    ERROR_STATUS			= 7,				//当前状态下不允许操作
    ERROR_MESSAGE			= 8,				//消息错误
    ERROR_OPERATE			= 9,				//操作错误
    ERROR_OPERATE_TIMEOUT   = 10,				//长时间不操作长时间不操作退出游戏
};
//消息
struct CMD_S_RoundResult_Tag
{
	int		m_Table[15];					//表盘水果
	int		Line[9];					    //连线
	int		TableLight[15];					//亮起来的图标
    int     m_AllLine[25][5];               //25根线上中奖的水果
    float     m_AllLineMuti[25];               //每根线的倍数
	int64_t	Score;							//中了多少分
	int64_t	lCurrentScore;					//当前携带金币
    int		nFootballNum;					//桌面上足球数
	int64_t	lFreeRoundWinScore;				//免费旋转总赢分
	bool	bSpeedUp[5];					//哪一列要加速
    int     FootballNum;
};

#endif // CMD_SGJ_HEAD_FILE
