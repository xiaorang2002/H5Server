
#include <stdio.h>
#include <unistd.h>
#include <kernel/CriticalSection.h>
//#include <kernel/EventObject.h>
#include <kernel/MutexObject.h>
#include <string>
#include <vector>

CRITICAL_SECTION ctx;
CRITICAL_COND	cond;

using namespace std;

vector<string> veclist;

void* thread_customer(void* args)
{
	printf("customer init.\n");

	while (1)
	{
		printf("before customer lock!\n");

		CCriticalSectionEx lock;
		if (lock.LockWaitTime(&ctx,&cond,1000))
		{
			FILE* fp = fopen("test.txt","a");
			printf("before write,vec size:[%d]\n",veclist.size());
			while (veclist.size()) 
			{
				string content = veclist.back();
				veclist.pop_back();

				// write the special content value item now.
				fwrite(content.c_str(),content.size(),1,fp);
			}

			printf("file written.\n");
			fclose(fp);
		}
		else
		{
			printf("customer time out!\n");
		}

		lock.Unlock();
		printf("after customer unlock!\n");

		usleep(1000 * 1000);
	}
}


int main()
{
	printf("init critical section\n");

	pthread_t handle;;
	pthread_create(&handle,0,thread_customer,0);

	int id = 0;
	CCriticalSectionEx::Create(&ctx,&cond);
	while (1)
	{
		printf("before set signal!\n");

		/*
		char buf[32]={0};
		gets(buf);
		if (lstrcmpi(buf,"0")==0) {
			continue;
		}
		*/

		// lock item.
		for (int i=0;i<10000;i++)
		{
        	CCriticalSectionEx lock;
        	lock.Lock(&ctx);

			// printf("produce data!\n");

			char data[1024]={0};
			snprintf(data,sizeof(data),"string-%d",id++);	

			string content = data;
			veclist.push_back(content);
			// lock.Signal(&cond);
			lock.Unlock();
		}

		printf("out loop set signal!\n");
		CCriticalSectionEx::Signal(&cond);

		printf("after product unlocked!\n");

		usleep( 1000 * 1000);
	}

	


	/*
	CRITICAL_SECTION ctx;
	CCriticalSectionEx::Create(&ctx);
	{

		CCriticalSectionEx lock;
		lock.Lock(&ctx);
		printf("locked!\n");
		lock.Unlock();

		lock.TryLock(&ctx);

		printf("this is try locked!\n");
	}
	*/

	/*
	pthread_mutex_t ctx;

	CMutexObject::Create(&ctx);

	CMutexObject lock;
	lock.Lock(&ctx);

	printf("wait event.\n");
	*/

	return 0;
}
