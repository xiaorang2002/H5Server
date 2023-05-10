
#include "random_engine.h"
#include <stdlib.h>

#ifdef WIN32
#include <windows.h>
#else
#endif 

/*-------------------------------------------------------------------------------------------------------------------*/
random_engine::random_engine()
{
#ifdef WIN32
	srand(GetTickCount());
#elif defined(LINUX)
	srand(time(0));
#endif
	_seed = rand();
	_tinymt.mat1 = rand();
	_tinymt.mat2 = rand();
	_tinymt.tmat = rand();

	tinymt32_init(&_tinymt, _seed);
}

/*-------------------------------------------------------------------------------------------------------------------*/
unsigned int random_engine::get_random()
{
	return tinymt32_generate_uint32(&_tinymt);
}

