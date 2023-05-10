#ifndef __RANDOMENGINE_H__
#define __RANDOMENGINE_H__

#include "tinymt32.h"

class random_engine
{
public:
	random_engine();
	~random_engine() {}

	unsigned int get_random();

private:
    tinymt32_t		_tinymt;
	int				_seed;
};

#endif