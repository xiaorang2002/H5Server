#ifndef LUCKYGMAE_H
#define LUCKYGMAE_H
#include <map>
#include <string>
 
#define  AssertReturn(x,y) \
    if(!(x)){\
        y;\
    }

#define     AWARD_COUNT		12

using namespace std;

struct UserScoreInfo {
    int64_t		lUserScore;         //玩家分数
    int64_t		lUserJiFenScore;    //玩家积分 
    int32_t		agentId;            //代理ID
    bool        isEnoughJifen;      //是否够积分
    string      account;        //帐户名
    string      lineCode;        //玩家linecode
};

struct WeidghtItem {
    int32_t		Weight[AWARD_COUNT];    //Weight 
    int32_t		Gold[AWARD_COUNT];      //Gold 
    int32_t		Icon[AWARD_COUNT];      //Icon
};

struct OpenInfo {
    int32_t		startTime;              //开始时间
    int32_t		duration;               //持续时间
};

struct tagConfig
{
    WeidghtItem vItemArry[3];                   //权重数组
    OpenInfo    vOpenArry[3];                   //营业时间
    int32_t		nPullInterval;                  //拉动时间间隔 
    int32_t		nGameId;                        //游戏ID
    int32_t		nRoomId;                        //房间ID
    int32_t		nExChangeRate;                  //兑换比例
    int32_t		MsgItemCount;                   //消息列表数量
    int32_t		RecordDays;                     //读取游戏记录天数
    int32_t		RecordItemCount;                //读取游戏记录条数
    int32_t		bulletinMin;                    //电子公告最低金币

    vector<int> vBetItemList;                   //获取押分列表

	tagConfig()
    {
        memset(vItemArry,0,sizeof(vItemArry));
    }
};

class LuckyGame
{
public:
    LuckyGame();
    ~LuckyGame();

    // 加载配置
    int LoadConfig(const char* path);
	int LoadCurrencyConfig(const char* path, int32_t currency);
    // 获取开奖结果
    int GetGameResult(int32_t nBetIndex,int32_t & nBetScore,int64_t & nWinScore,int32_t currency);
    bool IsLoadOK(){ return m_bIsloadOK; }
    bool IsEnoughJF(int32_t userJf, int32_t currency){
        return (userJf >= m_ConfigMap[currency].vBetItemList[0]);
    }
    bool CheckIsOpen(int32_t currency);
    bool GetFakeResult(int32_t & nBetScore,int32_t & nFakeWinScore, int32_t currency);
    // 
    tagConfig GetConfig(){ return m_Config; }
	tagConfig GetCurrencyConfig(int32_t currency) {
		return m_ConfigMap[currency];
	}

private:

    // 
    int RandomArea(int typeindex,int32_t nArray[]);
    //测试
    int InitTest(tagConfig allConfig);

private:
    tagConfig	        m_Config;           //配置

    int32_t             m_nSum[3];          //权重值之和
    int32_t             m_nBetItemCount;    //押分项目数量
    int32_t             m_openItemCount;    //数量
    bool                m_bIsloadOK;

	std::map<int32_t, tagConfig> m_ConfigMap;
};


#endif // LUCKGMAE_H
