#ifndef BETAREA_H
#define BETAREA_H
#include "../../public/IServerUserItem.h"
#include <stdint.h>
#include <string>
#include <glog/logging.h>

#define MAX_JETTON_AREA 12

class BetArea
{

public:
    BetArea();
    ~BetArea();
    void SetBetItem(int32_t idx,int32_t betValue);
    int32_t GetBetItem(int32_t idx);
    void SetJettonScore(int index, int32_t Score, int64_t bankerScore, bool isanzhuo); //chou ma he ya zhu
    void ClearAllJettonScore()
    {
        for (int i = 0; i < MAX_JETTON_AREA; i++)
        {
            mifourAreajetton[i] = 0;
            AndroidmifourAreajetton[i] = 0;
        }
    }
    void ThisTurnClear()
    {
        for (int i = 0; i < MAX_JETTON_AREA; i++)
        {
            ClearAllJettonScore();
            mifourAreajetton[i] = 0;        //每个区域的总下分
            AndroidmifourAreajetton[i] = 0; //
            mdCunrrentWinScores[i] = 0;     //纯赢的分数
            for (int j = 0; j < 5; j++)
            {
                AreaJettonCoin[i][j] = 0;
            }
        }
        AllAreaJetton = 0;
    }
    int64_t GetCurrenAreaJetton(int index)
    {
        return mifourAreajetton[index];
    }
    int64_t GetCurrenAnroid(int index)
    {
        return AndroidmifourAreajetton[index];
    }

public:
    int m_BetItemValues[5];           //chou ma

    int AreaJettonCoin[MAX_JETTON_AREA][5];           //chou ma
    int64_t mifourAreajetton[MAX_JETTON_AREA];        //每个区域的总下分
    int64_t AndroidmifourAreajetton[MAX_JETTON_AREA]; //每个区域的总下分 not Android 
    int64_t CanJettongScore[MAX_JETTON_AREA];         //下分区域可以下分最大限制
    int64_t mdCunrrentWinScores[MAX_JETTON_AREA];     //纯赢的分数
    int64_t AllAreaJetton;
};

#endif // BETAREA_H
