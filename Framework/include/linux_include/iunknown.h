#pragma once

#include <uuid/uuid.h>

// typedef the guid value now.
typedef unsigned int DWORD;
typedef uuid_t   GUID;
typedef GUID* REFGUID;

// clas IUnknown.
class IUnknownEx
{
public:
	virtual void Release() = 0;
	virtual void* QueryInterface(REFGUID Guid, DWORD dwQueryVer) = 0;

};
