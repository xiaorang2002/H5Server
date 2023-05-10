
#include <stdio.h>
#include <logger/LogAsync.h>
#include <logger/Logflags.h>
//#include <logger/LogWarn.h>

#include <logger/TraceInfo.h>

int main(int argc,char* argv[])
{
	CLogflags::Singleton().init("test.txt");

	/*
	CTraceInfo::Instance().start();

	while (1)
{

	for (int i=0;i<1000;i++) {
		CTraceInfo::Trace("hello world:[%d]\n",i);
	}

	printf("log written!\n");
	usleep(1000 * 1000);

}
	*/

	printf("LogAsync start\n");
	char file[MAX_PATH]={0};
	CLogAsync::Instance().start();
	while (1)
	{
		for (int i=0;i<10000;i++)
		{
			snprintf(file,sizeof(file),"./aicslog/test%d.txt",i%1000);
			CLogAsync::Instance().LogInfo(file,"test this is my test log!:[%d]",i);
		}
		
		printf("one loop dump done,size:[10000]\n");
		usleep(1000 * 1000);
	}
	printf("LogAsync end.\n");


	/*

	CLogflags::Singleton().init();

	printf("hello world!\n");

	*/
	/*

	logWarn("warning!\n");

	logInfo("info\n");

	logError("error!\n");

	logError("alasdjf;lsajfasjf;lfaj");

	*/

	// getchar();

	return 0;
}


