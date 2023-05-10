#ifndef BETAREA_H
#define BETAREA_H
#include "../../public/IServerUserItem.h"
#include <stdint.h>
#include <string>
#include <glog/logging.h>
#define MAX_JETTON_AREA   3
#define MAX_JETTON_MULTIPLE   8


class BetArea
{

public:
    BetArea();
    ~BetArea();
void SetJettonScore(int index,double Score,int64_t bankerScore,bool isanzhuo);//chou ma he ya zhu
void  ClearAllJettonScore()
{
    for(int i=0;i<MAX_JETTON_AREA;i++)
    {
        mifourAreajetton[i]=0;
        AndroidmifourAreajetton[i]=0;
    }
}
void ThisTurnClear()
{
    for(int i=0;i<MAX_JETTON_AREA;i++)
    {
        ClearAllJettonScore();
        mifourAreajetton[i]=0;//每个区域的总下分
        AndroidmifourAreajetton[i]=0;//
        mdCunrrentWinScores[i]=0;//纯赢的分数
        for(int j=0;j<5;j++)
        {
            AreaJettonCoin[i][j]=0;
        }
    }
    AllAreaJetton=0;
}
double GetCurrenAreaJetton(int index)
{
    return mifourAreajetton[index];
}
double GetCurrenAnroid(int index)
{
    return AndroidmifourAreajetton[index];
}
public:
    int AreaJettonCoin[MAX_JETTON_AREA][5];//chou ma
    double mifourAreajetton[MAX_JETTON_AREA];//每个区域的总下分
    double AndroidmifourAreajetton[MAX_JETTON_AREA];//每个区域的总下分 not Android
    double CanJettongScore[MAX_JETTON_AREA];//下分区域可以下分最大限制
    double mdCunrrentWinScores[MAX_JETTON_AREA];//纯赢的分数
    double AllAreaJetton;
};

#endif // BETAREA_H
