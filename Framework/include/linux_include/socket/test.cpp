
#include <stdio.h>
#include <socket/IpAddress.h>

int main(int argc,char* argv[])
{
	/*
	CIpAddress ip;
	int num = ip.GetMacAdapterNum();
	printf("adapter number:[%d]\n",num);

	const char* addr = ip.GetIp(1);
	printf("ip:[%s]\n",addr);

	const char* ip2 = ip.Domain2ip("www.baidu.com");
	printf("baidu ip:[%s]\n",ip2);

	const char* mac = ip.GetMac(1);
	printf("mac:[%s]\n",mac);

	uint32 intip = ip.GetIpInt(1);
	printf("int ip:[%08x]\n",intip);
	*/

	int n = inet_addr("127.0.0.1");
	printf("inet addr:[%x]\n",n);

	return 0;
}


