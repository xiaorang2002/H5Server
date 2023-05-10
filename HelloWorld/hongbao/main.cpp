#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"

#include "algorithmlogic.h"
#include <stdio.h>
#include <unistd.h>  // usleep
#include "redblackalgorithm.h"
shared_ptr <EventLoopThread>     timelooper;
void threadFun()
{


}
//int main()
//{
////    AlgorithmLogic alt;

////    while(alt.InItAgorithm())
////    {
////        sleep(10000);
////    }

//}
#include <stdio.h>
#include <unistd.h>
#include "IAicsEngine.h"
#include "time.h"
using namespace std;
int RandomInt(int min,int max)
{
    if(min>max)
    {
        int res=min;
        min=max;
        max=res;
    }
    if(min==max)
    {
        return min;
    }
    return min+rand()%(max-min)+1;
}
int firetimes=0;
int randomdds=10;
int main(int argc,char* argv[])
{
      srand(time_t(NULL));
      AlgorithmLogic *redBlack= new AlgorithmLogic();
      while(redBlack)
      {

      }


//      timelooper(new EventLoopThread(EventLoopThread::ThreadInitCallback(),"hhe"));
//      AlgorithmLogic   *arith=new AlgorithmLogic();
//      while(arith)
//      {

//      }

//    do
//    {
//        // initialize the special library.
//        IAicsEngine* sp = GetAicsEngine();
//        printf("sp:[%x]\n",sp);
//        if (!sp) break;

//        // 1. init aics engine.
//        GameSettings gs={0};
//        gs.nCurrToubiBiLi = 100;
//        gs.nMaxPlayer = 4;
//        int status = sp->Init(&gs);
//        printf("initialize status:[%d]\n",status);


//        // 2. select room.
//        int nRoomID = 5004;
//        status = sp->SetRoomID(nRoomID);
//        printf("select room id:[%d] status:[%d]\n",nRoomID,status);

//        // wait for the load Static succeeded.
//        usleep(1000 );

//        const int addslist[22]={2,2,3,4,5,6,7,8,9,10,12,15,18,20,25,30,35,40,50,80,100,300};

//        // try to do one bullet kill.



//        sp->OnUserSitdown(16519,1,2,false,false);
//        printf("onUserSitdown called!\n");

//    // for (int i=0;i<10;i++)
//    while (1)
//    {

//        // 参数配置.
//        int nTableID = 1;
//        int nChairID = 2;
//        int nUserID  = 16519;
//        int nBulletScore = 1000;

//                // 调用开炮通知.
//        status = sp->OnPlayerFired(nTableID,nChairID,nBulletScore);
//        printf("call on player fired status:[%d]\n",status);

//        ONEBKILLINFO onebk[4]={0};
//        firetimes++;
//        if(firetimes>10)
//        {
//             randomdds= 100;//addslist[RandomInt(0,21)];	// 2倍.
//            firetimes=0;
//        }
//        onebk[0].sign = AICS_DATA_SIGN;
//        onebk[0].nId  = 1;
//        onebk[0].nOdds =randomdds;
//        onebk[0].nResult = 0;	// AICSERR_KILLED为捕获成功.
//        // 判断是否捕获
//        status = sp->OneBulletKill(nTableID,nChairID,nUserID,nBulletScore,onebk,1);
//        printf("onebulletkill status:[%d],result:[%d]\n",status,onebk[0].nResult);

//        // 如果捕获，调用得分通知.
//        if ((1==status) /*&& (AICSERR_KILLED==onebk[0].nResult)*/)
//        {
//            int nCatchScore = nBulletScore * onebk[0].nOdds;
//            status = sp->OnFishCatched(nTableID,nChairID,nCatchScore);
//            printf("call OnFishCatched status:[%d]\n",status);
//        }

//        usleep(1000*1);
//    }


//    } while(0);

//Cleanup:
    printf("all done!\n");
    return 0;
}
