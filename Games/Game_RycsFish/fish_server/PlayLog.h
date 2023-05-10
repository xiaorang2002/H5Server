
#pragma once

#include <string>
#include <map>
#include <vector>
using namespace std;

// 鱼的TypeID 标识最大下标
#define MAX_FISH_COUNT 60

// 鱼类型，倍数，数量，得分
struct HitFishInfo {
    int32_t		fishId;            //鱼类型
    int32_t		odds;              //倍数
    int32_t		count;             //数量
    int32_t		win;               //得分
};

struct PaoInfo {
    int32_t		paoValues;                  //鱼类型
    int32_t		fireCount;                  //倍数
    int32_t		allWin;                     //数量
    //HitFishInfo	hitFsInfo[MAX_FISH_COUNT];  //鱼数组
    std::map<int ,HitFishInfo> hitFsInfo;
};
struct FishRecord{
	int32_t		fishId; //鱼类型
	int32_t		odds;	//鱼倍数
	int32_t		count;	//数量
};
class PlayLog
{
public:
    PlayLog();
    ~PlayLog();
    // 获取开奖结果
    int AddHitRecord(int32_t paoValues,int32_t fishId,int32_t odds,int32_t nWinScore,bool bCatchSpecialFireInGlobalBomb = false);
    int GetRecordStr(string & recoreStr);
public:
    map<int, PaoInfo> m_mPaoInfo;
};

 
