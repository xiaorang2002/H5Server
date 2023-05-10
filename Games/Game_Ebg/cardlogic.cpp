#include "cardlogic.h"
CardLogic::~CardLogic()
{

}
CardLogic::CardLogic()
{
    for(int i=0;i<2;i++)
        shuaizi[i]=0;

    srand(time(NULL));
    mVecCar.clear();
    for(int i=0;i<40;i++)
    {
        mVecCar.push_back(miMaJiang[i]);
    }
    RandomVec(mVecCar);
    InitFirstCard();
}
