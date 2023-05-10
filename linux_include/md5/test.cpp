
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <md5/MD5lib.h>

int main(int argc,char* argv[])
{
	char instr[] = "123456";

	unsigned char outbytes[33]={0};
	
	char outstr[64]={0};

	MD5(instr,outbytes,outstr);

	printf("out sring:[%s]\n",outstr);


	return 0;
}


