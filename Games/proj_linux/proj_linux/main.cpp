#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <pthread.h>
#include <cstdlib>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <map>
#include <unistd.h>

typedef  void (*Callback)();

typedef std::map<int32_t, Callback> Handlers;

Handlers CallbackFunc;

void foo1() {
	printf("call foo1 ...\n");
}
void foo2() {
	printf("call foo2 ...\n");
}
void init()
{
	CallbackFunc[1001] = foo1;
	CallbackFunc[1002] = foo2;
}

void* thread_func(void* arg)
{
    //printf("Ïß³Ìº¯Êý: %s\n", static_cast<char*>(arg));
    //return nullptr;
	do {
		sleep(5);
		int id[2] = { 1001,1002 };
		Callback handler = CallbackFunc[id[rand() % 2]];
		handler();
	} while (1);

}

//#pragma pack(1)
struct packet {
	int8_t i8;
	int32_t i32;
	int16_t	i16;
	int64_t i64;
};
//#pragma pack()

int main()
{	
	int size = sizeof(packet);
	init();
	int i = 0;
	int errnum;
	pthread_t tid[100];
    for (;;)
    {
		if (i >= 20) {
			break;
		}
		errnum = pthread_create(&tid[i], nullptr, thread_func,
			const_cast<char*>("Hello Linux!"));
		if (errnum != 0) {
			fprintf(stderr, "pthread_create error: %s\n", strerror(errnum));
			exit(1);
		}
		++i;
    }
	i = 0;
	for (;;)
	{
		if (i >= 20) {
			break;
		}
		errnum = pthread_join(tid[i], nullptr); //wait()
		if (errnum != 0) {
			fprintf(stderr, "pthread_join error: %s\n", strerror(errnum));
			exit(1);
		}
	}
	printf("********************************************************* exit .\n");	
	return 0;
}