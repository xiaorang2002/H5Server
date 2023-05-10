
#include <stdio.h>
//#include <console/ConsoleTimecheck.h>
#include <console/Consoledump.h>

int main(int argc,char* argv[])
{
	CConsoledump::Instance()->init();

	CConsoledump::Instance()->dump("dump info.");
	CConsoledump::Instance()->dumpWarn("dump warn.");
	CConsoledump::Instance()->dumpError("dump error.");

/*
	CConsoleTimecheck::Instance().start();

	printf("started!\n");

	usleep(1000000);

	CConsoleTimecheck::Instance().check("test");

	printf("checked!\n");

*/

	getchar();

	return 0;
}


