#ifndef CMD_SGJ_HEAD_FILE
#define CMD_SGJ_HEAD_FILE


#define GAME_NAME                               "财神到"                        //游戏名字
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

struct fruitWin
{
    fruitWin(){}
    int32_t lie;    //在哪一列
    int32_t heng;   //在哪一横
    int32_t fruit;  //是什么水果,财神到或者是本尊
    int32_t index;  //在十五格的那个位置
};


struct resultStr
{
    resultStr()
    {
        fruit = 0;
        mutic = 0;
        count = 0.0;
        isFree = false;
        winScore = 0;
        isWin = false;
        fruitVct.clear();
    }
    void Clear()
    {
        fruit = 0;
        mutic = 0.0;
        count = 0;
        isFree = false;
        winScore = 0;
        isWin =false;
        fruitVct.clear();
    }
    int32_t fruit; //开奖的水果
    double_t mutic; //开奖倍数
    int32_t count; //连续中水果的个数
    bool    isFree;//是否是免费开奖
    bool    isWin;//是否中普通奖
    int64_t winScore;//中奖金额
    vector<fruitWin> fruitVct;

};
//消息
struct Results
{
    int32_t fruit;
    vector<int32_t>  indexs;
    int32_t score;
};
struct CMD_S_RoundResult_Tag
{
    CMD_S_RoundResult_Tag()
    {
        lFreeRoundWinScore = 0;
        nFreeRoundLeft = 0;
        lCurrentScore = 0;
        Score=0;
        vResults.clear();
    }
	int		m_Table[15];					//表盘水果
    vector<Results>  vResults;              //中奖项容器
	int64_t	Score;							//中了多少分
	int64_t	lCurrentScore;					//当前携带金币
	int		nFreeRoundLeft;					//剩余免费旋转
	int64_t	lFreeRoundWinScore;				//免费旋转总赢分
};

#endif // CMD_SGJ_HEAD_FILE
