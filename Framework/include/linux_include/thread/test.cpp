
#include <stdio.h>
#include <kernel/Tthread.h>

void* ThreadTest(void* args)
{
 while (1)
 {
  printf("fuck linux!\n");
  usleep(1000 * 1000);
 }
}

int main()
{
  HANDLE_THREAD hThread = 0;
  TThread::CreateThreadT(&hThread,ThreadTest);


  getchar();
  printf("hello world!\n");

  TThread::TerminateThreadT(hThread);

  getchar();
  printf("all done!\n");
  return 0;
}
