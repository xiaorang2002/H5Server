/*
	how to use:
		MD5Hash(in,out,outstr);
*/


#ifndef __MD5LIB_HEADER__
#define __MD5LIB_HEADER__

#include <openssl/md5.h>

inline int MD5(const char* str,unsigned char* outbytes,char* outhexstr)
{
	int status = -1;
	unsigned char tmpbytes[33]={0};
	do
	{
		// try to check all the parameter content now.
		if ((!str)||(!outbytes)||(!outhexstr)) break;
		MD5_CTX ctx;
		MD5_Init(&ctx);
		MD5_Update(&ctx,str,strlen(str));
		MD5_Final(tmpbytes,&ctx);
		// try to get the content.
		if (outbytes) {
			memcpy(outbytes,tmpbytes,32);
		}

		// output string.
		if (outhexstr)
		{
			char* ptr = outhexstr;
			for (int i=0;i<16;i++) {
				sprintf(ptr,"%02X",tmpbytes[i]);
					ptr+=2;
			}
		}
		
		status=0;

	} while (0);
//Cleanup:
	return (status);	
}

#endif//__MD5LIB_HEADER__

