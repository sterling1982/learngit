
// FirstphaseDlg.cpp : 实现文件
//
#include "stdafx.h"
#include "afxdialogex.h"
#include "Firstphase.h"
#include "FirstphaseDlg.h"
//#include"Clogininfo.h"

#include "yolo_v2_class.hpp"
#include"darknet_yolo.h"

#include <Connection.h>
#include "map"
#include "JSONProduce.h"

#include "CircleQueue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



using namespace cv;
using namespace std;

typedef struct                                                           //传往线程函数的数据格式
{
	HKCamDriver m_CamDriver[threadBatch];
	int idex_cam[threadBatch];
	int idex_thread;
}caminfo;																

HANDLE thread[MaxCameraNum / threadBatch] = { NULL };					//线程句柄
//int CurrentFrameRate;
Mat homograph;															//透视变换矩阵
Mat video[MaxCameraNum];												//存储转换之后的帧
Size s;																	//video中帧的尺寸
double scale_opencv = 1;												//调节显示窗口的缩放
double factor = 1;														//IplImage到mat的缩放倍数
Detector detector1("yolov3.cfg", "yolov3.weights");						//检测用。不知道为什么最多开到3个，只好将摄像头分成三组，每组逐个使用
Detector detector2("yolov3.cfg", "yolov3.weights");
Detector detector3("yolov3.cfg", "yolov3.weights");

HKCamDriver::HKCamDriver()
{
	for (int idex = 0; idex < 3; idex++)
	{
		hMutexDetector[idex] = CreateMutex(NULL, FALSE, NULL);
	}
}

HKCamDriver::~HKCamDriver() {
	ReleaseCamera();
}

int HKCamDriver::ReleaseCamera(void)
{
	if (!NET_DVR_StopRealPlay(lRealPlayHandle)) {
		printf("NET_DVR_StopRealPlay error! Error number: %d\n", NET_DVR_GetLastError());
		return 0;
	}
	NET_DVR_Logout(lUserID);
	NET_DVR_Cleanup();
	return 1;
}

void HKCamDriver::InitHKNetSDK(void)
{
	/*  SDK Init                                 */
	NET_DVR_Init();
	/*  Set the Connect Time and Reconnect time  */
	NET_DVR_SetConnectTime(200, 1);
	NET_DVR_SetReconnect(10000, true);

	for (int i = 0; i < MaxCameraNum; i++) {
				for (int j = 0; j < 2; j++) 
				{
					hMutex[i][j] = CreateMutex(NULL, FALSE, NULL);
					pImg[i][j]._pQue->imageData = NULL;
				}
		nPort[i] = -1;
	}
	Scalefactor = 1.0f;

}

CamHandle HKCamDriver::InitCamera(char *sIP, char *UsrName, char *PsW, int Port)
{
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;

	lUserID = NET_DVR_Login_V30(sIP, Port, UsrName, PsW, &struDeviceInfo);

	if (lUserID < 0) {
		printf("Login error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return -1;
	}

	NET_DVR_SetExceptionCallBack_V30(0, NULL, ExceptionCallBack, NULL);

	NET_DVR_CLIENTINFO ClientInfo;
	ClientInfo.lChannel = 1;                    /* Channel number Device channel number. */
	ClientInfo.hPlayWnd = NULL;                 /* Window is NULL                        */
	ClientInfo.lLinkMode = 1;                   /* 子码流                            */
	ClientInfo.sMultiCastIP = NULL;

	lRealPlayHandle = NET_DVR_RealPlay_V30(lUserID, &ClientInfo, fRealDataCallBack, NULL, TRUE);

	if (lRealPlayHandle<0)return 0;

	return lRealPlayHandle;
}

void CALLBACK  HKCamDriver::DecCBFun(long nPort, char * pBuf, long nSize, FRAME_INFO * pFrameInfo, long nReserved1, long nReserved2)
{
	long lFrameType = pFrameInfo->nType;
	char WindowName[15];
	static IplImage* pImgYCrCb[MaxCameraNum];
	static IplImage* pImgTemp[MaxCameraNum];
	//sprintf_s(WindowName, "Windows:%d", nPort);

	if (lFrameType == T_YV12)
	{
		if (pImgYCrCb[nPort] == NULL) {
			pImgYCrCb[nPort] = cvCreateImage(cvSize(pFrameInfo->nWidth, pFrameInfo->nHeight), 8, 3);
		}
		if (pImgTemp[nPort] == NULL) {
			pImgTemp[nPort] = cvCreateImage(cvSize((int)(pFrameInfo->nWidth*Scalefactor), (int)(pFrameInfo->nHeight*Scalefactor)), 8, 3);
		}
		if (theApp.time[nPort]-->0)
		{
			yv12toYUV(pImgYCrCb[nPort]->imageData, pBuf, pFrameInfo->nWidth,
				pFrameInfo->nHeight, pImgYCrCb[nPort]->widthStep);

			cvResize(pImgYCrCb[nPort], pImgTemp[nPort], CV_INTER_LINEAR);

			cvCvtColor(pImgTemp[nPort], pImgTemp[nPort], CV_YCrCb2RGB);

			if (pImg[nPort][0]._pQue[0].imageData == NULL && pImg[nPort][1]._pQue[0].imageData == NULL)
			{
				for (int i = 0; i < 2; i++)
					for (int j = 0; j < queueSize; j++)
					{
						pImg[nPort][i]._pQue[j] = *cvCreateImage(cvSize(pImgTemp[nPort]->width, pImgTemp[nPort]->height), 8, 3);
					}
			}
			int triggerTemp = trigger[nPort];
			if (pImg[nPort][triggerTemp].full() || pImg[nPort][triggerTemp]._rear == 0)
			{
				DWORD RET = WaitForSingleObject(hMutex[nPort][triggerTemp], 2);
				//DWORD RET = WaitForSingleObject(hMutex[nPort][triggerTemp], INFINITE);
				switch (RET)
				{
				case WAIT_OBJECT_0:
				{
					pImg[nPort][triggerTemp].addQue(pImgTemp[nPort]);
					//测试用代码
					/*
					char winname[20] = "E:\\test\\";
					char name[4] = "";
					_itoa(nPort, name, 10);
					strcat(winname, name);
					strcat(winname, ".jpg");
					//cvNamedWindow(name);
					cvShowImage(name, &pImg[nPort][triggerTemp]._pQue[0]);
					//cvSaveImage(winname, &pImg[nPort][triggerTemp]._pQue[0]);
					waitKey(2);
					*/
					break;
				}
				default:
					cvReleaseImage(&pImgYCrCb[nPort]);
					ReleaseMutex(hMutex[nPort][triggerTemp]);
					return;
				}
			}
			else
			{
				pImg[nPort][triggerTemp].addQue(pImgTemp[nPort]);
				//测试用代码
				//cvShowImage("test1", &pImg[nPort][triggerTemp]._pQue[0]);
				//waitKey(2);

			}
			//测试用代码
			char winname[20] = "E:\\test\\";
			char name[4] = "";
			_itoa(nPort, name, 10);
			strcat(winname, name);
			strcat(winname, ".jpg");
			cvNamedWindow(name);
			cvShowImage(name, &pImg[nPort][triggerTemp]._pQue[0]);
			//cvSaveImage(winname, &pImg[nPort][triggerTemp]._pQue[0]);
			waitKey(2);
			
			cvReleaseImage(&pImgYCrCb[nPort]);
			//cvReleaseImage(&pImgTemp[nPort]);
			ReleaseMutex(hMutex[nPort][triggerTemp]);
			//theApp.test++;
			return;
		}
		else
		{
			//Sleep(150);
		}
	}
}
void CALLBACK  HKCamDriver::ExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
	char tempbuf[256] = { 0 };
	switch (dwType)
	{
	case EXCEPTION_RECONNECT:    /* Reconnet when Error Happen */
		break;
	default:
		break;
	}
}

void HKCamDriver::yv12toYUV(char *outYuv, char *inYv12, int width, int height, int widthStep)
{
	int col, row;
	unsigned int Y, U, V;
	int tmp;
	int idx;
	for (row = 0; row<height; row++) {
		idx = row * widthStep;
		int rowptr = row*width;
		for (col = 0; col<width; col++) {
			tmp = (row / 2)*(width / 2) + (col / 2);
			Y = (unsigned int)inYv12[row*width + col];
			U = (unsigned int)inYv12[width*height + width*height / 4 + tmp];
			V = (unsigned int)inYv12[width*height + tmp];
			outYuv[idx + col * 3] = Y;
			outYuv[idx + col * 3 + 1] = U;
			outYuv[idx + col * 3 + 2] = V;
		}
	}
}
/*     Realtime Steam Callback           */
void CALLBACK HKCamDriver::fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
	DWORD dRet;
	DWORD CameraIndex = 0;

	CameraIndex = lRealHandle;
	//printf("lRealHandle = %ld\n", CameraIndex);
	switch (dwDataType)
	{
		/*  System Head    */
	case NET_DVR_SYSHEAD:
		if (!PlayM4_GetPort(&nPort[CameraIndex]))break;  //获取播放库未使用的通道号

		if (dwBufSize > 0) {
			if (!PlayM4_SetStreamOpenMode(nPort[CameraIndex], STREAME_REALTIME))  //设置实时流播放模式
			{
				break;
			}
			if (!PlayM4_OpenStream(nPort[CameraIndex], pBuffer, dwBufSize, 1024 * 1024)) {
				dRet = PlayM4_GetLastError(nPort[CameraIndex]);
				break;
			}

			if (!PlayM4_SetDecodeFrameType(nPort[CameraIndex], 3)) {
				dRet = PlayM4_GetLastError(nPort[CameraIndex]);
				break;
			}

			/* Setting the Decode function*/
			if (!PlayM4_SetDecCallBack(nPort[CameraIndex], DecCBFun)) {
				dRet = PlayM4_GetLastError(nPort[CameraIndex]);
				break;
			}
			if (!PlayM4_Play(nPort[CameraIndex], NULL)) {
				dRet = PlayM4_GetLastError(nPort[CameraIndex]);
				break;
			}
			if (!PlayM4_SkipErrorData(nPort[CameraIndex], FALSE))
			{
				dRet = PlayM4_GetLastError(nPort[CameraIndex]);
				break;
			}

		}
		break;
		/* Code steam data    */
	case NET_DVR_STREAMDATA:
		if (dwBufSize > 0 && nPort[CameraIndex] != -1) {
			BOOL inData = PlayM4_InputData(nPort[CameraIndex], pBuffer, dwBufSize);
			//if (!PlayM4_InputData(nPort[CameraIndex], pBuffer, dwBufSize))
			//{
			//break;
			//}
			while (!inData) {
				Sleep(10);
				inData = PlayM4_InputData(nPort[CameraIndex], pBuffer, dwBufSize);
			}
		}
		break;

	default: //其他数据
		if (dwBufSize > 0 && nPort[CameraIndex] != -1)
		{
			if (!PlayM4_InputData(nPort[CameraIndex], pBuffer, dwBufSize))
			{
				break;
			}
		}
		break;
	}
}


vector<string> objects_names_from_file(string const filename)
{
	ifstream file(filename);
	vector<std::string> file_lines;
	if (!file.is_open()) return file_lines;
	for (string line; file >> line;) file_lines.push_back(line);
	cout << "object names loaded \n";
	return file_lines;
}

DWORD WINAPI threadproc(LPVOID lpParam)
//UINT threadproc(LPVOID lpParam)
{
	darknet_yolo darknet_yolo1;
	auto obj_names1 = objects_names_from_file("coco_czx.names");
	caminfo* pInfo = (caminfo*)lpParam;
	while (theApp.mark_track)
	{
		for (int idexCamera = 0; idexCamera < threadBatch; idexCamera++)
		{
			int iPort = nPort[pInfo->m_CamDriver[idexCamera].lRealPlayHandle];
			//  Check the iPort is vaild     
			if (iPort != -1) {
				/*
				if (isStop[pInfo->idex_thread])
				{
					CloseHandle(thread[pInfo->idex_thread]);
					thread[pInfo->idex_thread] = NULL;
					return 0;
				}
				*/
				Mat Imgtemp;
				//if (pImg[iPort][trigger[iPort]].full() && theApp.time[iPort]-->0)
				if (pImg[iPort][trigger[iPort]].full())
				{
					trigger[iPort] = !trigger[iPort];
					DWORD RET = WaitForSingleObject(hMutex[iPort][!trigger[iPort]], 2);
					//DWORD RET = WaitForSingleObject(hMutex[iPort][trigger[iPort]], INFINITE);

					switch (RET)
					{
					case WAIT_OBJECT_0:
						if (!pImg[iPort][!trigger[iPort]].empty())
						{
							//测试用代码
							//cvShowImage("test0", &pImg[iPort][!trigger[iPort]]._pQue[0]);
							//waitKey(2);
							Imgtemp = cvarrToMat(&pImg[iPort][!trigger[iPort]]._pQue[0]);

						}
						break;
					default:
						ReleaseMutex(hMutex[iPort][!trigger[iPort]]);
						continue;
					}
					if (Imgtemp.data == NULL)
					{
						ReleaseMutex(hMutex[iPort][!trigger[iPort]]);
						continue;
					}
					Mat Img;
					Imgtemp.copyTo(Img);
					ReleaseMutex(hMutex[iPort][!trigger[iPort]]);

					resize(Img, video[iPort], cv::Size(), factor, factor);
					s = video[iPort].size();
					char idex[3];
					_itoa(pInfo->idex_cam[idexCamera], idex, 10);
					char win[20] = "windows::im";
					//imshow(strcat(win, idex ), video[iPort]);
					vector<bbox_t> result_vec;
					if (pInfo->idex_cam[idexCamera] >= 0 && pInfo->idex_cam[idexCamera] < 5)
					{
						DWORD RET0 = WaitForSingleObject(hMutexDetector[0], 10);
						switch (RET0)
						{
						case WAIT_OBJECT_0:
							result_vec = detector1.detect(video[iPort]);
							ReleaseMutex(hMutexDetector[0]);
							break;
						default:
							ReleaseMutex(hMutexDetector[0]);
							continue;
						}
					}
					if (pInfo->idex_cam[idexCamera] >= 5 && pInfo->idex_cam[idexCamera] < 10)
					{
						DWORD RET1 = WaitForSingleObject(hMutexDetector[1], 10);
						switch (RET1)
						{
						case WAIT_OBJECT_0:
							result_vec = detector2.detect(video[iPort]);
							ReleaseMutex(hMutexDetector[1]);
							break;
						default:
							ReleaseMutex(hMutexDetector[1]);
							continue;
						}
					}
					if (pInfo->idex_cam[idexCamera] >= 10 && pInfo->idex_cam[idexCamera] < 16)
					{
						DWORD RET2 = WaitForSingleObject(hMutexDetector[2], 10);
						switch (RET2)
						{
						case WAIT_OBJECT_0:
							result_vec = detector3.detect(video[iPort]);
							ReleaseMutex(hMutexDetector[2]);
							break;
						default:
							ReleaseMutex(hMutexDetector[2]);
							continue;
						}
					}
					Mat detectImg;
					detectImg = darknet_yolo1.draw_boxes(video[iPort], result_vec, obj_names1);
					imshow(strcat(win, idex), detectImg);
					int width = s.width*scale_opencv*(iPort % 4);
					int height = s.height*(scale_opencv + 0.06) * (0 * (iPort >= 0 && iPort < 4) + 1 * (iPort >= 4 && iPort < 8) + 2 * (iPort >= 8 && iPort < 12) + 3 * (iPort >= 12 && iPort < 16));
					cvMoveWindow(strcat(win, idex), width, height);
					//测试代码
					//imwrite("E:\\test\\test.jpg", detectImg);

					waitKey(2);
					//theApp.test++;
					//vector<Point2f> worldcoordinate = darknet_yolo1.get_worldcoordinate(result_vec, homograph);
					//darknet_yolo1.transmit_result(result_vec, worldcoordinate, pInfo->idex_cam);
					//darknet_yolo1.transmit_result(result_vec);

					//ReleaseMutex(hMutex[iPort]);
					//Sleep(50);
				}
				else
				{
					Sleep(50);
				}
			}
		}
		
	}

	return 1;
}
/*
void threadprocMain( HKCamDriver m_CamDriver, int winNum)
//UINT threadproc(LPVOID lpParam)
{
	//Detector detector1("yolov3.cfg", "yolov3.weights");
	darknet_yolo darknet_yolo1;
	auto obj_names1 = objects_names_from_file("coco_czx.names");

	int iPort = nPort[m_CamDriver.lRealPlayHandle];
	//  Check the iPort is vaild     
	if (iPort != -1) {
		Mat Imgtemp;
		trigger[iPort] = !trigger[iPort];

		DWORD RET = WaitForSingleObject(hMutex[iPort][!trigger[iPort]], 2);
		//DWORD RET = WaitForSingleObject(hMutex[iPort][trigger[iPort]], INFINITE);

		switch (RET)
		{
		case WAIT_OBJECT_0:
			if (!pImg[iPort][!trigger[iPort]].empty())
			{
				Imgtemp = cvarrToMat(&pImg[iPort][!trigger[iPort]]._pQue[0]);
			}
			break;
		default:
			ReleaseMutex(hMutex[iPort][!trigger[iPort]]);
			return;
		}
		if (Imgtemp.data == NULL)
		{
			ReleaseMutex(hMutex[iPort][!trigger[iPort]]);
			return;
		}
		
		Mat Img;
		Imgtemp.copyTo(Img);
		ReleaseMutex(hMutex[iPort][!trigger[iPort]]);

		resize(Img, video[iPort], cv::Size(), factor, factor);
		s = video[iPort].size();
		char idex[3];
		_itoa(winNum, idex, 10);
		char win[20] = "windows::im";
		//imshow(strcat(win, idex), video[iPort]);

		WaitForSingleObject(hMutexDetector[2], INFINITE);
		vector<bbox_t> result_vec = detector3.detect(video[iPort]);
		ReleaseMutex(hMutexDetector[2]);
		imshow(strcat(win, idex), darknet_yolo1.draw_boxes(video[iPort], result_vec, obj_names1));
		int width = s.width*scale_opencv*(iPort % 4);
		int height = s.height*(scale_opencv + 0.06) * (0 * (iPort >= 0 && iPort < 4) + 1 * (iPort >= 4 && iPort < 8) + 2 * (iPort >= 8 && iPort < 12) + 3 * (iPort >= 12 && iPort < 16));
		cvMoveWindow(strcat(win, idex), width, height);
		waitKey(2);
		//darknet_yolo1.show_result(result_vec);
		//vector<Point2f> worldcoordinate = darknet_yolo1.get_worldcoordinate(result_vec, homograph);
		//darknet_yolo1.transmit_result(result_vec, worldcoordinate, pInfo->idex_cam);
		//darknet_yolo1.transmit_result(result_vec);

		//ReleaseMutex(hMutex[iPort]);
		//Sleep(50);
	}

}
*/
void HKCamDriver::SetScaleFactor(float factor)
{
	Scalefactor = factor;
}



// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

														// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
	//afx_msg void OnLoginCopy();
	afx_msg void OnBnClickedButton1();
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CFirstphaseDlg 对话框



CFirstphaseDlg::CFirstphaseDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_FIRSTPHASE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_pCPLANE = NULL;
	m_pCTRACK = NULL;

	theApp.m_CamDriver[0].InitHKNetSDK();
	theApp.m_CamDriver[0].SetScaleFactor(0.25f);
}

void CFirstphaseDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CFirstphaseDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(TVN_SELCHANGED, IDC_DEVICE_TREE, &CFirstphaseDlg::OnTvnSelchangedTree1)
	ON_CBN_SELCHANGE(IDC_COMBO1, &CFirstphaseDlg::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON5, &CFirstphaseDlg::OnBnClickedButton5)
	ON_BN_CLICKED(IDC_BUTTON6, &CFirstphaseDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON_ADDALL, &CFirstphaseDlg::OnBnClickedButtonAddall)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CFirstphaseDlg 消息处理程序

BOOL CFirstphaseDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

									// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFirstphaseDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFirstphaseDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFirstphaseDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CFirstphaseDlg::OnTvnSelchangedTree1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CFirstphaseDlg::OnCbnSelchangeCombo1()
{
	// TODO: 在此添加控件通知处理程序代码
}


void CFirstphaseDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	if (NULL == m_pCPLANE)
	{
		// 创建非模态对话框实例   
		m_pCPLANE = new CPLANE();
		m_pCPLANE->Create(IDD_DIALOG_PLANE, this);
	}
	// 显示非模态对话框   
	m_pCPLANE->ShowWindow(SW_SHOW);
}


CFirstphaseDlg::~CFirstphaseDlg()
{
	// 如果非模态对话框已经创建则删除它   
	if (NULL != m_pCPLANE || NULL != m_pCTRACK)
	{
		// 删除非模态对话框对象   
		delete m_pCPLANE;
		delete m_pCTRACK;
	}
}

void CFirstphaseDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	/*
	CString tempstr;
	double scale_opencv = 1;
	for (int idex = 0; idex < MaxCameraNum; idex++)
	{
		theApp.time[idex] = Time;
	}
	int timerId = 1;
	unsigned int timerInterval = 1000;
	caminfo percaminfo[MaxCameraNum / threadBatch];
	homograph = solve_perspective();
	SetTimer(timerId, timerInterval, NULL);
	GetDlgItem(IDC_BUTTON_TRACK)->GetWindowText(tempstr);
	if (tempstr == _T("显示实时跟踪"))
	{
		GetDlgItem(IDC_BUTTON_TRACK)->SetWindowText(_T("停止实时跟踪"));
		theApp.mark_track = 1;
	}
	else
	{
		GetDlgItem(IDC_BUTTON_TRACK)->SetWindowText(_T("显示实时跟踪"));
		theApp.mark_track = 0;
	}
	//SetPriorityClass(GetCurrentProcess(), THREAD_PRIORITY_TIME_CRITICAL);
	//for (int idex_thread = 0; idex_thread < MaxCameraNum - 4; idex_thread++)

	for (int idex_thread = 0; idex_thread < MaxCameraNum/threadBatch; idex_thread++)
	{
		for (int idexCamera = 0; idexCamera < threadBatch; idexCamera++)
		{
			percaminfo[idex_thread].m_CamDriver[idexCamera] = theApp.m_CamDriver[threadBatch * idex_thread + idexCamera];
			percaminfo[idex_thread].idex_cam[idexCamera] = threadBatch * idex_thread + idexCamera;
		}
		percaminfo[idex_thread].idex_thread = idex_thread;
		thread[idex_thread] = CreateThread(NULL, 0, threadproc, &percaminfo[idex_thread], 0, NULL);
		//SetThreadPriority(thread[idex_thread], THREAD_PRIORITY_TIME_CRITICAL);
		SetThreadPriority(thread[idex_thread], THREAD_PRIORITY_HIGHEST);
		//CloseHandle(thread[idex_thread]);
		//AfxBeginThread(threadproc, &percaminfo[idex_thread], THREAD_PRIORITY_TIME_CRITICAL, 0, 0, NULL);
		//SetThreadPriorityBoost(thread[idex_thread], TRUE);
		//SetThreadPriorityBoost(thread[idex_thread], FALSE);
		//Sleep(200);
	}
	*/
	long long timeStamp = getTimeStampMs();
	long long timeRenew = timeStamp;

	while (1)
	{
		if (getTimeStampMs() - timeStamp > 250)
		{
			timeStamp = 0;
			for (int idex = 0; idex < MaxCameraNum; idex++)
			{
				theApp.time[idex] = Time;
			}
		}
		/*
		if ((getTimeStampMs() - timeRenew)  > 20000)
		{
 			for (int idex1 = 0; idex1 < MaxCameraNum / threadBatch; idex1++)
			{
				isStop[idex1] = 1;
				Sleep(10);
				while (thread[idex1] != NULL);
				isStop[idex1] = 0;
				thread[idex1] = CreateThread(NULL, 0, threadproc, &percaminfo[idex1], 0, NULL);
				SetThreadPriority(thread[idex1], THREAD_PRIORITY_HIGHEST);
			}
			timeRenew = getTimeStampMs();
		}
		*/
		else
		{
			Sleep(100);
		}

	}
	
	/*
	for (int idex_thread = MaxCameraNum - 4; idex_thread < MaxCameraNum; idex_thread++)
	{
		percaminfo[idex_thread].m_CamDriver = theApp.m_CamDriver[idex_thread];
		percaminfo[idex_thread].idex_cam = idex_thread;
		
	}
	
	while (theApp.mark_track)
	{
		for (int idex_thread = MaxCameraNum - 4; idex_thread < MaxCameraNum; idex_thread++)
		{
			//threadprocMain(theApp.m_CamDriver[idex_thread], idex_thread);
		}
	}
	*/
	for (int idex_camera = 0; idex_camera < MaxCameraNum; idex_camera++)
	{
		//char* winname = winname_connet(idex_camera);
		char idex[3];
		_itoa(idex_camera, idex, 10);
		char win[20] = "windows::im";
		cvDestroyWindow(strcat(win, idex));
	}
}



void CAboutDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码

}


void CFirstphaseDlg::OnBnClickedButtonAddall()
{
	// TODO: 在此添加控件通知处理程序代码

	ifstream in("C:/Users/c1/Desktop/IP.txt");
	string line_IP;
	int idex = 0;
	if (in) // 有该文件
	{
		char c[20] = { 0 };
		while (getline(in, line_IP) && idex < MaxCameraNum) // line中不包括每行的换行符
		{
			strcpy(c, line_IP.c_str());
			theApp.m_CamDriver[idex++].InitCamera(c, "admin", "wls771102", 8000);
		}
	}
	Sleep(500 * MaxCameraNum);
	MessageBox(_T("设备已加载完"), _T("加载情况"), MB_OK);
}





void CFirstphaseDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	switch (nIDEvent)
	{
	case 1:
		for (int idex = 0; idex < MaxCameraNum; idex++)
		{
			theApp.time[idex] = Time;
		}
	default:
		break;
	}

	CDialogEx::OnTimer(nIDEvent);
}
