#ifndef CARDLOGIC_H
#define CARDLOGIC_H

//#include "game_brnn_global.h"

#include <stdint.h>
#include <string>
//#include "Utility.h"
#include "time.h"
#include "vector"
#include "../../public/Globals.h"



static int  RandomInt(int a,int b)
{
    return rand() %(b-a+1)+ a;
}
const int miMaJiang[40]=
{
    1,2,3,4,5,6,7,8,9,10,
    1,2,3,4,5,6,7,8,9,10,
    1,2,3,4,5,6,7,8,9,10,
    1,2,3,4,5,6,7,8,9,10,
};

#define cardtype0   0
#define cardtype1   1
#define cardtype2   2
#define cardtype3   3
#define cardtype4   4
#define cardtype5   5
#define cardtype6   6
#define cardtype7   7
#define cardtype8   8
#define cardtype9   9
#define cardtype28  10
#define cardtypeZha 11
#define cardtypejinbaozi 12
class CardLogic
{
public:
    CardLogic();
    ~CardLogic();
public:
    struct  CarGroupMaJiang
    {
         CarGroupMaJiang()
         {
            for(int i=0;i<2;i++)
                card[i]=-1;
         }


         int getcardtype()
         {
            if(card[0]==-1||card[1]==-1)
                return -1;//fan hui shi bai
            if(card[0]==card[1])
            {
                if(card[0]==10)
                    return cardtypejinbaozi;
                else
                    return cardtypeZha;
            }
            else if(card[0]+card[1]==10&&(card[0]==2||card[0]==8))
            {
                return cardtype28;
            }
            else
            {
                return (card[0]+card[1])%10;
            }
         }
        int GetBigOne()
        {
            if(card[0]>=card[1])
            {
               return  card[0];
            }
            else
            {
                return card[1];
            }
        }
         int comparewithother(CarGroupMaJiang other)
         {

             if(this->getcardtype()>other.getcardtype())
             {
                 return 1;
             }
             else if(this->getcardtype()==other.getcardtype())
             {


                 if(getcardtype()==cardtype28)
                 {
                     return -1;// xiang deng
                 }
                 else if(getcardtype()==cardtypeZha)
                 {
                     if(this->GetBigOne()==other.GetBigOne())
                     {
                        return -1;
                     }
                     else if(this->GetBigOne()>other.GetBigOne())
                     {
                         return 1;
                     }
                     else
                     {
                         return -1;
                     }
                 }
                 else
                 {
                    if(getcardtype()==other.getcardtype())
                    {
                        if(this->GetBigOne()==other.GetBigOne())
                        {
                           return -1;
                        }
                        else if(this->GetBigOne()>other.GetBigOne())
                        {
                            return 1;
                        }
                        else
                        {
                            return -1;
                        }
                    }
                    else if(getcardtype()>other.getcardtype())
                    {
                        return 1;
                    }
                    else
                    {
                        return -1;
                    }
                 }
             }
             else
             {
                 return -1;
             }
         }
         void setCard(int val, int index)
         {
             if(index<0||index>1)
                 return;
             card[index]=val;
         }

         int card[2];
    };
void RandomVec(vector<int> &vec)
{
    if(vec.empty())
    {
        return;
    }
    std::vector<int>   vecindex;
    std::vector<int>   vepai;
    vepai.clear();
    vecindex.clear();
    for(int i=0;i<vec.size();i++)
    {
        vepai.push_back(vec[i]);
        vecindex.push_back(i);
    }
    vec.clear();
    for(int i=0;i<vepai.size();i++)
    {
        int xxx=RandomInt(0,(int)vecindex.size()-1);
        vec.push_back(vepai[vecindex[xxx]]);
        vecindex.erase(vecindex.begin()+xxx);
    }
}
void InitVector()//chongxin xi pai bing qie sheng cheng yi zhang sui ji pai
{
    mVecCar.clear();
    for(int i=0;i<40;i++)
    {
        mVecCar.push_back(miMaJiang[i]);
    }
    RandomVec(mVecCar);
    InitFirstCard();
    shuaizi[0]=RandomInt(1,6);
    shuaizi[1]=RandomInt(1,6);
    //LOG(INFO)<<"Group1car1="<<vecCardGroup[0].card[0]<<"     Group1car2"<<vecCardGroup[0].card[1];
    //LOG(INFO)<<"Group2car1="<<vecCardGroup[0].card[0]<<"     Group2car2"<<vecCardGroup[0].card[1];
    //LOG(INFO)<<"Group3car1="<<vecCardGroup[0].card[0]<<"     Group3car2"<<vecCardGroup[0].card[1];
    //LOG(INFO)<<"Group4car1="<<vecCardGroup[0].card[0]<<"     Group4car2"<<vecCardGroup[0].card[1];
}


void InitFirstCard()
{
    std::vector<int>::iterator it=mVecCar.begin();
    int index=0;
    for(;it!=mVecCar.end()&&index<4;)
    {
        vecCardGroup[index].setCard((*it),0); //value index
        vecCardGroup[index].setCard(-1,1);
        it=mVecCar.erase(it);
        index++;

    }
}


void GetCardGroup(int8_t val[])//chuan ru suan fa ying shu jie guo
{
    int countnum=0;

//    while(true)
//    {
//        countnum++;
//        vector<int> vallin;
//        vallin.clear();
//        std::vector<int>::iterator it=mVecCar.begin();
//        for(;it!=mVecCar.end();it++)
//        {
//            vallin.push_back(*it);
//        }
//        RandomVec(vallin);
//        //Free matching
//        for(int i=0;i<4;i++)
//        {
//            std::vector<int>::iterator it1=vallin.begin();
//            vecCardGroup[i].setCard((*it1),1);
//            it1=vallin.erase(it1);
//        }




//        int index1=vecCardGroup[1].comparewithother(vecCardGroup[0]);
//        int index2=vecCardGroup[2].comparewithother(vecCardGroup[0]);
//        int index3=vecCardGroup[3].comparewithother(vecCardGroup[0]);
//        //check restlt equal val
//        if(val[0]==index1&&val[1]==index2&&val[2]==index3)
//        {
//            break;
//        }
//        else if(countnum>5000) //avoid in loop I think it too big
//        {
//            break;
//        }
//    }
     //RandomVec(mVecCar);
      // it can traverse all
      for(int i =0;i<mVecCar.size();i++)
      {
          for (int j =0;i<mVecCar.size()&&i!=j;j++)
          {
              for(int m =0;i<mVecCar.size()&&m!=i&&m!=j ;m++)
              {
                  for(int n =0;i<mVecCar.size()&&n!=i&&n!=j&&n!=m;n++)
                  {
                       vecCardGroup[0].setCard((mVecCar[i]),1);
                       vecCardGroup[1].setCard((mVecCar[j]),1);
                       vecCardGroup[2].setCard((mVecCar[m]),1);
                       vecCardGroup[3].setCard((mVecCar[n]),1);

                       int index1=vecCardGroup[1].comparewithother(vecCardGroup[0]);
                       int index2=vecCardGroup[2].comparewithother(vecCardGroup[0]);
                       int index3=vecCardGroup[3].comparewithother(vecCardGroup[0]);
                       //check restlt equal val
                       if(val[0]==index1&&val[1]==index2&&val[2]==index3)
                       {
                           return;
                       }
                  }

              }
          }
      }

}
public:
    std::vector<int> mVecCar;  

    CarGroupMaJiang vecCardGroup[4];

    int   shuaizi[2];

};

#endif // CARDLOGIC_H
