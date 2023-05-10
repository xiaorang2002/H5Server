
#include "SLua.h"

#include <fstream>

SVar::SVar()
{
	m_type=TP_NULL;
	m_dbNumber=0;
	m_pTable=NULL;
}
SVar::~SVar()
{
	if(m_pTable)
	{
		for(int i=0;i<(int)m_dbNumber;++i)
			delete m_pTable[i];
		delete[] m_pTable;
	}
}
SVar::SVar(int n)
{
	m_type=TP_NUMBER;
	m_dbNumber=n;
	m_pTable=NULL;
}
SVar::SVar(unsigned int n)
{
	m_type=TP_NUMBER;
	m_dbNumber=n;
	m_pTable=NULL;
}
SVar::SVar(int64_t n)
{
	m_type=TP_NUMBER;
	m_dbNumber=(double)n;
	m_pTable=NULL;
}
SVar::SVar(double n)
{
	m_type=TP_NUMBER;
	m_dbNumber=n;
	m_pTable=NULL;
}
SVar::SVar(const char *pStr)
{
	m_type=TP_STRING;
	m_dbNumber=0;
	m_pTable=NULL;

	if(pStr)
		m_str=pStr;
}
SVar::SVar(const string &str)
{
	m_type=TP_STRING;
	m_dbNumber=0;
	m_pTable=NULL;

	m_str=str;
}
SVar::SVar(const SVar &s)
{
	m_type=s.m_type;
	m_dbNumber=0;
	m_pTable=NULL;

	switch(s.m_type)
	{
	case TP_NUMBER:
		m_dbNumber=s.m_dbNumber;
		break;
	case TP_STRING:
		m_str=s.m_str;
		break;
	case TP_TABLE:
		for(int i=0;i<(int)s.m_dbNumber;i+=2)
			Push(new SVar(*s.m_pTable[i]),new SVar(*s.m_pTable[i+1]));
		break;
	}
}
SVar&					SVar::operator=(int n)
{
	Clear();
	m_type=TP_NUMBER;
	m_dbNumber=n;    
	return *this;
}
SVar&					SVar::operator=(unsigned int n)
{
	Clear();
	m_type=TP_NUMBER;
	m_dbNumber=n;    
	return *this;
}
SVar&					SVar::operator=(int64_t n)
{
	Clear();
	m_type=TP_NUMBER;
	if (CheckNumber(n)==false)	n=0;
	m_dbNumber=(double)n;    
	return *this;
}
SVar&					SVar::operator=(double n)
{
	Clear();	
	m_type=TP_NUMBER;
	if (CheckNumber(n)==false)	n=0.0000;
	m_dbNumber=n;    
	return *this;
}
SVar&					SVar::operator=(const char *pStr)
{
	Clear();
	m_type=TP_STRING;
	if(pStr)
		m_str=pStr;  
	return *this;  
}
SVar&					SVar::operator=(const string &str)
{
	Clear();
	m_type=TP_STRING;
	m_str=str;  
	return *this;  
}
SVar&					SVar::operator=(const SVar &s)
{
	if (&s==this)  //相同SVar赋值
		return *this;	
	Clear();
	m_type=s.m_type;

	switch(m_type)
	{
	case TP_NUMBER:
		m_dbNumber=s.m_dbNumber;
		break;
	case TP_STRING:
		m_str=s.m_str;
		break;
	case TP_TABLE:
		for(int i=0;i<(int)s.m_dbNumber;i+=2)
			Push(new SVar(*s.m_pTable[i]),new SVar(*s.m_pTable[i+1]));
		break;
	}

	return *this;
}
bool					SVar::IsEmpty()
{
	return m_type==TP_NULL;
}
void					SVar::Clear()
{
	switch(m_type)
	{
	case TP_NUMBER:
		m_dbNumber=0;
		break;
	case TP_STRING:
		m_str.clear();
		break;
	case TP_TABLE:
		if(m_pTable)
		{
			for(int i=0;i<(int)m_dbNumber;++i)
				delete m_pTable[i];
			delete[] m_pTable;
			m_pTable=NULL;
			m_dbNumber=0;
		}
		break;
	}

	m_type=TP_NULL;
}
void					SVar::ClearTable()
{
	if(m_pTable)
	{
		for(int i=0;i<(int)m_dbNumber;++i)
			delete m_pTable[i];
		delete[] m_pTable;
		m_pTable=NULL;
		m_dbNumber=0;
	}
}
SType					SVar::Type()
{
	return m_type;
}
void					SVar::SetType(SType tp)
{
	Clear();
	m_type=tp;
}
bool					SVar::IsNumber()
{
	return m_type==TP_NUMBER;
}
bool					SVar::IsString()
{
	return m_type==TP_STRING;
}
bool					SVar::IsTable()
{
	return m_type==TP_TABLE;
}
bool					SVar::IsNil()
{
	return m_type==TP_NIL;
}
string					SVar::ToString()  
{
	return m_str;  
}
bool					SVar::Find(int n)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return true;
	}
	return false;
}
bool					SVar::Find(unsigned int n)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return true;
	}
	return false;
}
bool					SVar::Find(int64_t n)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return true;
	}
	return false;
}
bool					SVar::Find(double n)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return true;
	}
	return false;
}
bool					SVar::Find(const char *p)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_STRING&&strcmp(m_pTable[i]->m_str.c_str(),p)==0)
			return true;
	}
	return false;
}
bool					SVar::Find(const string &str)
{
	if(m_type!=TP_TABLE)
		return false;

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_STRING&&m_pTable[i]->m_str==str)
			return true;
	}
	return false;
}
SVar&					SVar::operator[](int n)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(n);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
SVar&					SVar::operator[](unsigned int n)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(n);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
SVar&					SVar::operator[](int64_t n)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}
	if(CheckNumber(n)==false) n=0.0000;
	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(n);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
SVar&					SVar::operator[](double n)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}
	if(CheckNumber(n)==false) n=0.0000;
	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER&&m_pTable[i]->m_dbNumber==n)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(n);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
SVar&					SVar::operator[](const char *p)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_STRING&&strcmp(m_pTable[i]->m_str.c_str(),p)==0)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(p);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
SVar&					SVar::operator[](const string &str)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_STRING&&m_pTable[i]->m_str==str)
			return *m_pTable[i+1];
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=new SVar(str);
	m_pTable[(int)m_dbNumber++]=new SVar;
	return *m_pTable[(int)m_dbNumber-1];
}
int								SVar::GetSize()
{
	return (int)m_dbNumber/2;
}
SVar&							SVar::GetKey(int i)
{
	
	if (m_pTable)
		return *m_pTable[i*2];
	static SVar sNil;
	sNil["0"]=0;
	return sNil;
}
SVar&							SVar::GetValue(int i)
{
	
	if (m_pTable)
		return *m_pTable[i*2+1];
	static SVar sNil;
	sNil["0"]=0;
	return sNil;

}
bool							SVar::Push(SVar *pKey,SVar *pValue)
{
	if(m_type!=TP_TABLE)
	{
		Clear();
		m_type=TP_TABLE;
	}

	switch(pKey->m_type)
	{
	case TP_NULL:
	case TP_NIL:
	case TP_TABLE:
		return false;
	}

	switch(pValue->m_type)
	{
	case TP_NULL:
	case TP_NIL:
		return false;
	}

	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(pKey->m_type==m_pTable[i]->m_type)
		{
			if(pKey->m_type==TP_NUMBER&&pKey->m_dbNumber==m_pTable[i]->m_dbNumber)
				return false;
			else if(pKey->m_type==TP_STRING&&pKey->m_str==m_pTable[i]->m_str)
				return false;
		}
	}

	if(m_pTable)
	{
		if(((int)m_dbNumber)%10==0)
		{
			SVar **pTemp=new SVar*[(int)m_dbNumber+10];
			memset(pTemp+(int)m_dbNumber+2,0,8*sizeof(void*));
			memcpy(pTemp,m_pTable,(int)m_dbNumber*sizeof(void*));
			delete[] m_pTable;
			m_pTable=pTemp;
		}
	}
	else
		m_pTable=new SVar*[10];

	m_pTable[(int)m_dbNumber++]=pKey;
	m_pTable[(int)m_dbNumber++]=pValue;
	return true;
}
bool							SVar::Push(SVar *pValue)
{
	SVar *pKey=new SVar(1);
	for(int i=0;i<(int)m_dbNumber;i+=2)
	{
		if(m_pTable[i]->m_type==TP_NUMBER)
		{
			if(m_pTable[i]->m_dbNumber>=pKey->m_dbNumber)
			{
				pKey->m_dbNumber=(long long)(m_pTable[i]->m_dbNumber+1);
			}
		}
	}

	if(Push(pKey,pValue)==false)
	{
		delete pKey;
		return false;
	}

	return true;
}




