// fish_server.h : fish_server DLL 的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// 主符号


// Cfish_serverApp
// 有关此类实现的信息，请参阅 fish_server.cpp
//

class Cfish_serverApp : public CWinApp
{
public:
	Cfish_serverApp();

// 重写
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
