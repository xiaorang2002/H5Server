/*
    Usage:

	注意: 开优化后有可能会导致变量内容变乱,这是正常的,不必惊慌!
	
    1.关优化.
    #pragma optimize( "g", off )

    2. 开始保护.
    VMProtectImplBegin("Test marker");

    3. 结束保护.
    VMProtectImplEnd()

    4.开优化
    #pragma optimize( "g", on )


	// disable optimize.
	#pragma GCC push_options
	#pragma GCC optimize ("O0")

	// restore optimize.
	#pragma GCC pop_options


*/

#ifdef  _WIN32
#include <Windows.h>
// agma comment(lib,"VMProtectSDK32.lib")
#pragma comment(lib,"VMProtectSDK32.lib")


#else

#endif//_WIN32

#include "VMProtectSDK.h"

#define VMProtectImplBegin(name) VMProtectBegin(name)
#define VMProtectImplEnd()       VMProtectEnd()
