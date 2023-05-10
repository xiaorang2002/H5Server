
#include <types.h>
#include <stdio.h>
#include <stdlib.h>
#include <vmprotect/VMProtectImpl.h>

int main(int argc,char* argv[])
{
	char data[] = "12345678";

	int n = atoi(data);

	VMProtectImplBegin("Test1");
	
	printf("all done,n:[%d]!\n",n);

	VMProtectImplEnd();

	return 0;
}


