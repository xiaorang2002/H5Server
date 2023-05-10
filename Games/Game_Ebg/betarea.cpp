#include "betarea.h"

BetArea::BetArea()
{
    for(int i=0;i<MAX_JETTON_AREA;i++)
    {
        mifourAreajetton[i]=0.0;
        for(int j=0;j<5;j++)
        {
            AreaJettonCoin[i][j]=0;
        }
    }
    for(int i=0;i<MAX_JETTON_AREA;i++)
    {
        AndroidmifourAreajetton[i]=0.0f;
    }

}
BetArea::~BetArea()
{

}

void BetArea::SetJettonScore(int index, double Score, int64_t bankerScore,bool isanzhuo)
{

   mifourAreajetton[index]+=Score;
   AllAreaJetton+=Score;
   if(!isanzhuo)
   {
       AndroidmifourAreajetton[index]+=Score;
   }
   if(Score==1)
   {
       AreaJettonCoin[index][0]+=1;
   }
   else if(Score==10)
   {
       AreaJettonCoin[index][1]+=1;
   }
   else if(Score==50)
   {
       AreaJettonCoin[index][2]+=1;
   }
   else if(Score==100)
   {
       AreaJettonCoin[index][3]+=1;
   }
   else if(Score==500)
   {
       AreaJettonCoin[index][4]+=1;
   }
   else
   {
        AreaJettonCoin[index][0]+=1;
   }
}
