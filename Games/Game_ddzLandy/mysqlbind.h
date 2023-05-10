
#ifndef __MYSQLBIND_HEADER__
#define __MYSQLBIND_HEADER__

#include <vector>
#include <stdlib.h>
#include <memory.h>
#include <lsm/types.h>
#include <mysql/mysql.h>
#include <lsm/smartmem.h>

#ifdef  WIN32
#define strcasecmp	strcmp
#endif//WIN32

class MysqlBind
{
public:
	MysqlBind()
	{
		memset(&bdat,0,sizeof(bdat));
	}

	// int value.
	MysqlBind(const int target)
	{
        PLONG pBuffer = (PLONG)malloc(sizeof(LONG));
		if   (pBuffer)
		{
			 *pBuffer=target;
			 /// try to update the data field data.
			 bdat.buffer_type   = MYSQL_TYPE_LONG;
			 bdat.buffer        = (char*)pBuffer;
			 bdat.buffer_length = sizeof(LONG);
		}
	}

	// string value.
	MysqlBind(const char* target)
	{
		size = strlen(target)+1;
		char* pBuffer = (char*)malloc(size);
		if   (pBuffer)
		{
			strncpy(pBuffer,target,size);
			bdat.buffer_type   = MYSQL_TYPE_STRING;
			bdat.buffer        = (char*)pBuffer;
			bdat.buffer_length = 1024;
			bdat.length        = &size;
			bdat.is_null       = 0;
		}
	}

	// blob value.
	MysqlBind(Smartmem target)
	{
		size = target.size();
		LPBYTE pBuffer = (LPBYTE)malloc(size+1);
		if    (pBuffer)
		{
			memcpy(pBuffer,target.data(),size);
			bdat.buffer_type   = MYSQL_TYPE_BLOB;
			bdat.buffer        = (char*)pBuffer;
			bdat.buffer_length = size;
		}
	}

public:	
	MysqlBind operator=(const int target)
	{
        PLONG pBuffer = (PLONG)malloc(sizeof(LONG));
		if   (pBuffer)
		{
			*pBuffer=target;
			/// try to update the data field data.
			bdat.buffer_type   = MYSQL_TYPE_LONG;
			bdat.buffer        = (char*)pBuffer;
			bdat.buffer_length = sizeof(LONG);
		}
	}

	MysqlBind operator=(const char* target)
	{
		size = strlen(target)+1;
		char* pBuffer = (char*)malloc(size);
		if   (pBuffer)
		{
			strncpy(pBuffer,target,size);
			bdat.buffer_type   = MYSQL_TYPE_STRING;
			bdat.buffer        = (char*)pBuffer;
			bdat.buffer_length = 1024;
			bdat.length        = &size;
			bdat.is_null       = 0;
		}
	}

	MysqlBind operator=(Smartmem target)
	{
		size = target.size();
		LPBYTE pBuffer = (LPBYTE)malloc(size+1);
		if    (pBuffer)
		{
			memcpy(pBuffer,target.data(),size);
			bdat.buffer_type   = MYSQL_TYPE_BLOB;
			bdat.buffer        = (char*)pBuffer;
			bdat.buffer_length = size;
		}
	}

public:// get data item.
	MYSQL_BIND* bind()
	{
		return &bdat;
	}

public:
	char*      name; // field name.
protected:
	ULONG      size; // data  size.
	MYSQL_BIND bdat; // bdat  data.
};

class MysqlBindList
{
public:
	MysqlBindList()
	{
	}

public:
	// try to get the special value item now.
	MysqlBind& operator[](const char* name)
	{
		MysqlBind* pResult=NULL;
		std::vector<MysqlBind*>::iterator iter;
		for (iter=vec.begin();iter!=vec.end();iter++) {
			MysqlBind* p = *iter;
			if (strcasecmp(p->name,name)==0) {
				pResult=p;
				break;
			}
		}

		// check if now.
		if (!pResult) {
			 pResult = new MysqlBind();
			 pResult->name=strdup(name);
			 vec.push_back(pResult);
		}
	//Cleanup:
		return *pResult;
	}

	// try to get the special index.
	MysqlBind* operator[](int index)
	{
		MysqlBind* pResult=NULL;
		if (index<vec.size()) {
			pResult=vec[index];
		}
	//Cleanup:
		return pResult;
	}

	///d clear()
	void clear()
	{
		std::vector<MysqlBind*>::iterator iter;
		for (iter=vec.begin();iter!=vec.end();iter++) {
			MysqlBind* p = *iter;
			if (p) {
				free(p->name);
				delete p;
			}
		}
	//Cleanup:
		vec.clear();
	}

	//ng size()
	long size()
	{
		return (vec.size());
	}

protected:
	std::vector<MysqlBind*> vec;
};

#endif//__MYSQLBIND_HEADER__
