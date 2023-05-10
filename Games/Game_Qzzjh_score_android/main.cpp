
#include <iostream>
#include <stdlib.h>
//#include "../QPAlgorithm/zjh.h"

#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

int main()
{
	//ZJH::CGameLogic::TestCards("./conf/zjh_cardList.ini");
	return 0;
}
