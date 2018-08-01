
// FirstphaseDlg.h : 头文件

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
#define threadBatch 1										  //一个线程中包含的摄像头数目，threadBatch为能被16整除的数													
#define Time 1												  //实时预览的帧数限制												

typedef long CamHandle;                                        //Camera Handle 
static HANDLE hMutex[MaxCameraNum][2];						   //为每个摄像头的两个队列分配互斥量
static HANDLE hMutexDetector[3];							   //为3个detector分配互斥量
static long nPort[MaxCameraNum];							   //为每个摄像头分配一个播放通道

//static IplImage* pImg[MaxCameraNum];
static int trigger[MaxCameraNum] = { 0 };					   //切换两个队列的开关
static CircleQueue pImg[MaxCameraNum][2];					   //存储每个摄像头的两个队列的头指针

//static int reduceThread[MaxCameraNum] = { 0 };
static float Scalefactor;									   //由yv12转为IplImage的缩放倍数
static int isStop[MaxCameraNum / threadBatch] = { 0 };		   //线程是否关闭的标志

class HKCamDriver {
public:

	HKCamDriver();
	~HKCamDriver();

	//初始化sdk，即便多个摄像头，调用一次即可
	void InitHKNetSDK(void);
    //初始化摄像头
	CamHandle InitCamera(char *sIP, char *UsrName, char *PsW, int Port = 8000);
	//注销设备
	int ReleaseCamera(void);
	//异常回调函数
	static void CALLBACK ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser);
	//解码函数，将yv12转为rgb
	static void CALLBACK DecCBFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nReserved1, long nReserved2);
	//实时解码函数，里面设置了DecCBFun作为其回调函数
	static void CALLBACK fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser);
	//设置yv12到IplImage的缩放倍数
	static void SetScaleFactor(float factor);
	//实时播放句柄
	LONG lRealPlayHandle;
private:
	//将 yv12转为YUV，属于DecCBFun的一部分  
	static void yv12toYUV(char *outYuv, char *inYv12, int width, int height, int widthStep);
	//Camera User ID                                        
	LONG lUserID;
};
#endif



// CFirstphaseDlg 对话框
class CFirstphaseDlg : public CDialogEx
{
// 构造
public:
	CFirstphaseDlg(CWnd* pParent = NULL);	// 标准构造函数
	~CFirstphaseDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_FIRSTPHASE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
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
