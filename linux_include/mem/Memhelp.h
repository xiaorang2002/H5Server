

#ifndef __MEMHELP_HEADER__
#define __MEMHELP_HEADER__

#include <stdlib.h>
#include <string.h>
class CMemhelp
{
public:
	static void memset(void* _dst,unsigned char dat,int size)
	{
		unsigned char* dst=(unsigned char*)_dst;
		for (int i=0;i<size;i++) {
			dst[i]=dat;
		}
	}

	static void memcpy(void* _dst,const void* _src,long size)
	{
		unsigned char* src = (unsigned char*)_src;
		unsigned char* dst = (unsigned char*)_dst;
		for (long nIndex=0;nIndex<size;nIndex++) {
			*dst = *src;
			src++;dst++;
		}
	}

	static int memcmp(const void* _dst,const void* _src,long size)
	{
		int sum=0;
		unsigned char* dst = (unsigned char*)_dst;
		unsigned char* src = (unsigned char*)_src;
		for (long nIndex=0;nIndex<size;nIndex++) {
			sum += src[nIndex] ^ dst[nIndex];
		}
	//Cleanup:
		return (sum);
	}

	static int lstrlen(const char* lpsz)
	{
		int len=0;
		if (!lpsz) return len;
		while (*lpsz){
			len++;
			lpsz++;
		}
	//Cleanup:
		return (len);
	}

	static void lstrcat(char* dst,const char* val,int maxlen)
	{
		int len=0;
		char* tmp=dst;
		maxlen=maxlen-1;
		while (*tmp){
			tmp++;
			len++;
		}

		// try to copy the string value.
		while ((len<maxlen) && (*val)){
			*tmp=*val;
			tmp++;val++;
			len++;
		}

		*tmp='\0';
	}

	static void lstrcpyn(char* dst,const char* src,int maxlen)
	{
		int len = 0;
		while (*src)
		{
			*dst=*src;
			src++;dst++;
			len++;
			if (len>=maxlen) {
				break;
			}
		}

		*dst='\0';
	}

	/// try to check if all the string value item content.
	static int lstrcmp(const char* _dst,const char* _src)
	{
		int   sum = -1;
		char* src = lstrupr(lstrdup(_src));
		char* dst = lstrupr(lstrdup(_dst));
		int  len1 = lstrlen(src)+1;
		int  len2 = lstrlen(dst)+1;
		int   len = (len1>len2)?len2:len1;
		// loop to check all the.
		for (int i=0;i<len;i++){
			sum = *dst-*src;
			if (0!=sum) break;
			dst++;src++;
		}
	//Cleanup:
		return (sum);
	}

	/// try to check if all the string value item content value now.
	static int lstrcmpn(const char* _dst,const char* _src,long len)
	{
		int   sum = -1;
		char* src = lstrupr(lstrdup(_src));
		char* dst = lstrupr(lstrdup(_dst));
		// loop to check all the.
		for (int i=0;i<len;i++){
			sum = *dst-*src;
			if (0!=sum) break;
			dst++;src++;
		}
		//Cleanup:
		return (sum);
	}



	static char* lstrdup(const char* dst)
	{
		int len=lstrlen(dst)+1;
		char* tmp=new char[len];
		lstrcpyn(tmp,dst,len);
	//Cleanup:
		return (tmp);
	}

	static char* lstrupr(char* dst)
	{
		char* tmp=dst;
		while (*dst) {
			if ((*dst>='a') && (*dst<='z')) {
				*dst -= 0x20;
			}

			dst++;
		}
	//Cleanup:
		return (tmp);
	}

	static void lstrlow(char* dst)
	{
		while (*dst) {
			if ((*dst>='A') && (*dst<='Z')) {
				*dst += 0x20;
			}

			dst++;
		}
	}

	// try to find the sub string for buffer content item value.
	static const char* lstrstr(const char* buf,const char* key)
	{
		const char* result=0;
		char* tmpbuf=lstrdup(buf);
		char* keybuf=lstrdup(key);
		lstrupr(tmpbuf);lstrupr(keybuf);
		int alen=lstrlen(tmpbuf);
		int klen=lstrlen(keybuf);
		int kpos=0;
		for (int i=0;i<alen;i++) {
			if (tmpbuf[i]==keybuf[kpos]){
				kpos++;
				if (kpos>=klen) {
					result=&buf[i-(kpos-1)];
					break;
				}
			}   else
			{
				kpos=0;
			}
		}
	//Cleanup:
		if (keybuf) delete[] keybuf;
		if (tmpbuf) delete[] tmpbuf;
		return (result);
	}

	// str=input,key=replace rpl=replaced replace string with string.
	static char* lstrrpl(char* str,const char* key,const char* rpl)
	{
		char* out = 0;
		do 
		{
			// try to check if all the input parameter has validate.
			if ((!lstrlen(str))||(!lstrlen(key))||(!lstrlen(rpl))){
				break;
			}

			char* p = str;
			int len = lstrlen(str);
			int outlen = len + 1024;
			out = (char*)malloc(outlen);
			CMemhelp::memset(out,0,outlen);
			while((*p!='\0') && (len > 0))  
			{
				if(strncmp(p,key,lstrlen(key))!= 0)  
				{
					int n  = lstrlen(out);  
					out[n] = *p;  
					out[n+1] = '\0';  
					p++;  
					len--;  
				}  
				else  
				{  
					lstrcat(out,rpl,outlen);  
					p += lstrlen(key);  
					len -= lstrlen(rpl);  
				} 
			}
			
			// try to get the result string value now.
			CMemhelp::lstrcpyn(str,out,lstrlen(str));
			free(out);
		}   while (0);
	//Cleanup:
		return (str);
	}

	// delete special extension name.
	static bool delExtname(char* lpsz)
	{
		bool bSucc=false;
		do 
		{
			int len = lstrlen(lpsz);
			char* p = lpsz+len-1;
			if (!len) {
				break;
			}

			// loop to delete the name now.
			for (int i=0;i<len;i++,p--){
				if (*p=='.') {
					*p='\0';
					break;
				}
			}

		} while (0);
	//Cleanup:
		return (bSucc);
	}

};



#endif//__MEMHELP_HEADER__