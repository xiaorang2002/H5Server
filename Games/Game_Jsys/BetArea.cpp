#include "betarea.h"

BetArea::BetArea()
{
    for (int i = 0; i < MAX_JETTON_AREA; i++)
    {
        mifourAreajetton[i] = 0.0;
        for (int j = 0; j < 5; j++)
        {
            AreaJettonCoin[i][j] = 0;
        }
    }
    for (int i = 0; i < MAX_JETTON_AREA; i++)
    {
        AndroidmifourAreajetton[i] = 0;
    }

    memset(m_BetItemValues,0,sizeof(m_BetItemValues));
    // m_BetItemValues[0] = 0;
}
BetArea::~BetArea()
{
}

void BetArea::SetBetItem(int32_t idx,int32_t betValue)
{
    m_BetItemValues[idx % 5] = betValue;
}

int32_t BetArea::GetBetItem(int32_t idx)
{
    return m_BetItemValues[idx % 5];
}

void BetArea::SetJettonScore(int index, int32_t Score, int64_t bankerScore, bool isanzhuo)
{
    mifourAreajetton[index] += Score;
    AllAreaJetton += Score;
    if (!isanzhuo)
    {
        AndroidmifourAreajetton[index] += Score;
    }
    // if (Score == m_BetItemValues[0])//1)
    // {
    //     AreaJettonCoin[index][0] += 1;
    // }
    // else if (Score == m_BetItemValues[1])//10)
    // {
    //     AreaJettonCoin[index][1] += 1;
    // }
    // else if (Score == m_BetItemValues[2])//50)
    // {
    //     AreaJettonCoin[index][2] += 1;
    // }
    // else if (Score == m_BetItemValues[3])//100)
    // {
    //     AreaJettonCoin[index][3] += 1;
    // }
    // else if (Score == m_BetItemValues[4])//500)
    // {
    //     AreaJettonCoin[index][4] += 1;
    // }
    // else
    // {
    //     AreaJettonCoin[index][0] += 1;
    // }
}
