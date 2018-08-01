
// Firstphase.h : PROJECT_NAME 应用程序的主头文件
#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号
#include"FirstphaseDlg.h"

using std::vector;

// CFirstphaseApp: 
// 有关此类的实现，请参阅 Firstphase.cpp
//

class CFirstphaseApp : public CWinApp
{
public:
	CFirstphaseApp();


public:
	virtual BOOL InitInstance();
	HKCamDriver m_CamDriver[MaxCameraNum];//海康摄像头
	//int reducefps[MaxCameraNum] = { 0 };
    int time[MaxCameraNum];//实时预览时限制帧数
	int mark_track = 1;//控制是否跟踪检测
	int test = 0;//估测实时预览的帧数。也可做其他用处
	DECLARE_MESSAGE_MAP();
};
extern CFirstphaseApp theApp;
