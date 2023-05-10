
#include <stdio.h>
#include "iocpheader.h"

int main()
{

	/* // test iocp server.
	int port = 18000;
 	IIOCPServer* sp = CreateIOCPServer();
 	int32 status = sp->Initialize("192.168.1.171",port);
 	printf("server listen on port:[%d] status:[%d]!\n",port,status);
	while (1)
	{
		sp->update();
	}
	*/

	while (1)
	{
		// test iocp client.
		IIOCPClient* sp = CreateIOCPClient();

		int32 status = sp->Connect("134.175.242.15",8000);
		printf("connect status:[%d]\n",status);
	}

	/*
	char bufsend[] = "GET /\r\n\r\n";
	char bufrecv[8192]={0};	// read the buffer.
	uint32 dwBytesToRead = sizeof(bufrecv);

	status = sp->SendReceive((BYTE*)bufsend,sizeof(bufsend),(BYTE*)bufrecv,&dwBytesToRead);
	printf("send received status:[%d],received length:[%d],data:[%s]\n",status,dwBytesToRead,bufrecv);
	*/

	return 0;
}
