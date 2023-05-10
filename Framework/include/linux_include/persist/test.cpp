
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <persist/ThreadPersistAgent.h>

CThreadPersistAgent* m_pThreadPersistAgent=0;

int main(int argc,char* argv[])
{
	CThreadPersistAgent::Instance().start();

	printf("started!\n");

	int id=1;
	while (1)
	{
		char path[256]={0};
		for (int i=0;i<10;i++)
		{
			snprintf(path,256,"./log/test%d.txt",i%1000);
			CThreadPersistAgent::Instance().write(path,(unsigned char*)"lsm",3);
		}	

		printf("one time written done, id:[%d]!\n", id++);
		// usleep(5 * 1000000);
		usleep(1000 * 1000);
	}

	// try to write the special content value for later user content item data.
	CThreadPersistAgent::Instance().write("./test.txt",(unsigned char*)"lsm",3);
	printf("write called!\n");
	getchar();

//Cleanup:
	return 0;
}


