#pragma once

#include <stdlib.h>
#include <string.h>
#include <string>

// class help for atl string. 
class CT2CA
{
public:
	CT2CA(const char* src)
	{
		data = strdup(src);
	}

	virtual ~CT2CA()
	{
		if (data) {
			free(data);
			data=0;
		}
	}

public:
	operator const char*() const
	{
		return (data);
	}

	operator std::string() const
	{
		std::string str = data;
		return str;
	}

protected:
	char* data;
};


// define all macro now.
#define CT2CW	CT2CA
#define CW2CT 	CT2CA

#define CA2CT	CT2CA











