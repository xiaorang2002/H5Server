

#include <stdio.h>
#include <unistd.h>
//#include "Aics/publics/IAicsdragon.h"
//#include "Aics/publics/IAicsRedxBlack.h"
#include <iostream>
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#define xsleep(t) Sleep(t*1000)
#define clscr() system("cls")
#else
#include <unistd.h>
#define xsleep(t) sleep(t)
#define clscr() system("reset")
#endif

extern int testAicsDragon();
extern int testAicsDragon2();
extern int testAicsRedBlack();

int main(int argc,char* argv[])
{
	//测试龙虎
	testAicsDragon2();
	//testAicsRedBlack();
	return 0;
}
