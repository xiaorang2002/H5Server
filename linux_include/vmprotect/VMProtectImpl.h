/*
    Usage:

	ע��: ���Ż����п��ܻᵼ�±������ݱ���,����������,���ؾ���!
	
    1.���Ż�.
    #pragma optimize( "g", off )

    2. ��ʼ����.
    VMProtectImplBegin("Test marker");

    3. ��������.
    VMProtectImplEnd()

    4.���Ż�
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
