
// Firstphase.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������
#include"FirstphaseDlg.h"

using std::vector;

// CFirstphaseApp: 
// �йش����ʵ�֣������ Firstphase.cpp
//

class CFirstphaseApp : public CWinApp
{
public:
	CFirstphaseApp();


public:
	virtual BOOL InitInstance();
	HKCamDriver m_CamDriver[MaxCameraNum];//��������ͷ
	//int reducefps[MaxCameraNum] = { 0 };
    int time[MaxCameraNum];//ʵʱԤ��ʱ����֡��
	int mark_track = 1;//�����Ƿ���ټ��
	int test = 0;//����ʵʱԤ����֡����Ҳ���������ô�
	DECLARE_MESSAGE_MAP();
};
extern CFirstphaseApp theApp;
