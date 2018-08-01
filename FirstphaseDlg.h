
// FirstphaseDlg.h : ͷ�ļ�

#pragma once
#include "PLANE.h"
#include "TRACK.h"
#include "afxcmn.h"
#include<vector>
#include"stdafx.h"
#define OPENCV

//HK

#ifndef _HKCAMDRIVER_H_
#define _HKCAMDRIVER_H_
 
#include "plaympeg4.h"                                         //  Software decoder header                 
#include "HCNetSDK.h"                                          //  HCNet Camera SDK header               

#include "CircleQueue.h"

using namespace cv;                                            
using namespace std;                                            

 
#define MaxCameraNum  8
#define threadBatch 1										  //һ���߳��а���������ͷ��Ŀ��threadBatchΪ�ܱ�16��������													
#define Time 1												  //ʵʱԤ����֡������												

typedef long CamHandle;                                        //Camera Handle 
static HANDLE hMutex[MaxCameraNum][2];						   //Ϊÿ������ͷ���������з��以����
static HANDLE hMutexDetector[3];							   //Ϊ3��detector���以����
static long nPort[MaxCameraNum];							   //Ϊÿ������ͷ����һ������ͨ��

//static IplImage* pImg[MaxCameraNum];
static int trigger[MaxCameraNum] = { 0 };					   //�л��������еĿ���
static CircleQueue pImg[MaxCameraNum][2];					   //�洢ÿ������ͷ���������е�ͷָ��

//static int reduceThread[MaxCameraNum] = { 0 };
static float Scalefactor;									   //��yv12תΪIplImage�����ű���
static int isStop[MaxCameraNum / threadBatch] = { 0 };		   //�߳��Ƿ�رյı�־

class HKCamDriver {
public:

	HKCamDriver();
	~HKCamDriver();

	//��ʼ��sdk������������ͷ������һ�μ���
	void InitHKNetSDK(void);
    //��ʼ������ͷ
	CamHandle InitCamera(char *sIP, char *UsrName, char *PsW, int Port = 8000);
	//ע���豸
	int ReleaseCamera(void);
	//�쳣�ص�����
	static void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser);
	//���뺯������yv12תΪrgb
	static void CALLBACK DecCBFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nReserved1, long nReserved2);
	//ʵʱ���뺯��������������DecCBFun��Ϊ��ص�����
	static void CALLBACK fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser);
	//����yv12��IplImage�����ű���
	static void SetScaleFactor(float factor);
	//ʵʱ���ž��
	LONG lRealPlayHandle;
private:
	//�� yv12תΪYUV������DecCBFun��һ����  
	static void yv12toYUV(char *outYuv, char *inYv12, int width, int height, int widthStep);
	//Camera User ID                                        
	LONG lUserID;
};
#endif



// CFirstphaseDlg �Ի���
class CFirstphaseDlg : public CDialogEx
{
// ����
public:
	CFirstphaseDlg(CWnd* pParent = NULL);	// ��׼���캯��
	~CFirstphaseDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTPHASE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeCombo1();
private:
	CPLANE *m_pCPLANE;
	CTRACK *m_pCTRACK;
public:
	afx_msg void OnBnClickedButton5();
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButtonAddall();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};


vector<string> objects_names_from_file(string const filename);
