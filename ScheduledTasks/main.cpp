#include "muduo/base/ThreadPool.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/CurrentThread.h"
#include "muduo/base/Logging.h"




#include "algorithmlogic.h"
#include <stdio.h>
#include <unistd.h>  // usleep

muduo::net::EventLoop *gloop = NULL;
shared_ptr <EventLoopThread>     timelooper;

#include <stdio.h>
#include <unistd.h>
#include "time.h"
using namespace std;
int main(int argc,char* argv[])
{
    srand(time_t(NULL));
    muduo::net::EventLoop loop;
    gloop = &loop;
    AlgorithmLogic redBlack;
    gloop->loop();
    printf("all done!\n");  
    return 0;
}
