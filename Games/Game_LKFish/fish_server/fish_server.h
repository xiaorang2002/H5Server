// fish_server.h : fish_server DLL ����ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ������


// Cfish_serverApp
// �йش���ʵ�ֵ���Ϣ������� fish_server.cpp
//

class Cfish_serverApp : public CWinApp
{
public:
	Cfish_serverApp();

// ��д
public:
	virtual BOOL InitInstance();

	DECLARE_MESSAGE_MAP()
};
