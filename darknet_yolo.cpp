#include"stdafx.h"
#pragma once

#include"darknet_yolo.h"
#include "yolo_v2_class.hpp"

#include <Connection.h>
#include "map"
#include "JSONProduce.h"


using namespace cv;
using namespace std;

string url_front = "http://192.168.14.31:8080/com.bimcenter.transfer/dataPush?method=push&data=";//发往网址的前半部分
string url_all;																					 //整个的网址


cv::Mat darknet_yolo::draw_boxes(cv::Mat mat_img, std::vector<bbox_t> result_vec, std::vector<std::string> obj_names, unsigned int wait_msec) {
	int person_num = result_vec.size();
	for (auto &i : result_vec) {

		cv::Scalar color(60, 160, 260);

		//if (obj_names.size() > i.obj_id)
		if (0 == i.obj_id)
		{
			cv::rectangle(mat_img, cv::Rect(i.x, i.y, i.w, i.h), color, 3);
			//putText(mat_img, obj_names[i.obj_id], cv::Point2f(i.x, i.y - 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color);
		}

		if (i.track_id > 0)

			putText(mat_img, std::to_string(i.track_id), cv::Point2f(i.x + 5, i.y + 15), cv::FONT_HERSHEY_COMPLEX_SMALL, 1, color);

	}
	//putText(mat_img, num2str(person_num), cv::Point2f(20, 30), cv::FONT_HERSHEY_COMPLEX_SMALL, 1,(60, 160, 260));
	//cv::imshow("window name", mat_img);
	return mat_img;
	//cv::waitKey(wait_msec);
	//cv::waitKey(30);

}



void LoadFromTXT(const char* txtFilename, CvPoint* cooConer)
{
	fstream fileDataSheet(txtFilename);
	fileDataSheet >> cooConer[0].x >> cooConer[0].y
		>> cooConer[1].x >> cooConer[1].y
		>> cooConer[2].x >> cooConer[2].y
		>> cooConer[3].x >> cooConer[3].y;
	fileDataSheet.close();
}

Mat solve_perspective(){
//获取虚拟线圈角点和世界坐标系点

	CvPoint VirtualLoopPiont[4];
	CvPoint WorldPlane[4];
	LoadFromTXT("E:/exam/710/Firstphase/Firstphase/ViutualLoopPoint.txt", VirtualLoopPiont);
	LoadFromTXT("E:/exam/710/Firstphase/Firstphase/WorldPlane.txt", WorldPlane);

	vector<Point2f> pFP;
	vector<Point2f> pHP;
	pFP.push_back(Point2f(VirtualLoopPiont[0].x, VirtualLoopPiont[0].y));
	pFP.push_back(Point2f(VirtualLoopPiont[1].x, VirtualLoopPiont[1].y));
	pFP.push_back(Point2f(VirtualLoopPiont[2].x, VirtualLoopPiont[2].y));
	pFP.push_back(Point2f(VirtualLoopPiont[3].x, VirtualLoopPiont[3].y));

	pHP.push_back(Point2f(WorldPlane[0].x, WorldPlane[0].y));
	pHP.push_back(Point2f(WorldPlane[1].x, WorldPlane[1].y));
	pHP.push_back(Point2f(WorldPlane[2].x, WorldPlane[2].y));
	pHP.push_back(Point2f(WorldPlane[3].x, WorldPlane[3].y));

	//求解透视变换矩阵
	 Mat homograph = findHomography(pFP, pHP, RANSAC);
	return homograph;
}

vector<Point2f> darknet_yolo::get_worldcoordinate(std::vector<bbox_t> result_vec, Mat homograph)
{
	int person_num = result_vec.size();
	vector<Point2f> person_frame;
	vector<Point2f> person_world;
	for (auto &i : result_vec)
	{
		person_frame.push_back(Point2f((float)(i.x + i.w / 2), (float)(i.y + i.h)));
	}
	
	perspectiveTransform(person_frame, person_world, homograph);

	return person_world;
}

/*
void darknet_yolo::transmit_result(std::vector<bbox_t> result_vec,vector<Point2f> worldcoordinate=NULL,int id) {
	
	JSONProduct json;
	json.addMember("b", num2str(id));
	int person_num = result_vec.size();
	json.addMember("person_num", person_num);
	for (auto &person : worldcoordinate)
	{
		string obj[] = {"longitude ",num2str(person.x) ,"latitude",num2str(person.y) };//添加对象
		json.addObj("people", obj, 4);
	}
	string jsonstr = json.toString();

	CWininetHttp cw;
	url_all = url_front + jsonstr;

	cw.RequestJsonInfo(url_all);
	
}
*/
void darknet_yolo::transmit_result(std::vector<bbox_t> result_vec) {


	JSONProduct json;
	
	//string id = "id";
	//json.addMember("id", id);

	int person_num = result_vec.size();
	json.addMember("person_num", person_num);
	string jsonstr = json.toString();

	CWininetHttp cw;
	url_all = url_front + jsonstr;
	cw.RequestJsonInfo(url_all);

}
string num2str(int num) {
	string res;
	stringstream ss;
	ss << num;
	ss >> res;
	return res;
}
string num2str(float num) {
	string res;
	stringstream ss;
	ss << num;
	ss >> res;
	return res;
}
long long getTimeStampMs()
{
	SYSTEMTIME sys;
	GetLocalTime(&sys);
	struct tm stm;
	memset(&stm, 0, sizeof(stm));
	stm.tm_year = sys.wYear - 1900;
	stm.tm_mon = sys.wMonth - 1;
	stm.tm_mday = sys.wDay;
	stm.tm_hour = sys.wHour;
	stm.tm_min = sys.wMinute;
	stm.tm_sec = sys.wSecond;
	long long int timeStampS = mktime(&stm);
	long long int timeStampMs = 1000 * timeStampS + sys.wMilliseconds;
	return timeStampMs;
}
/*
CvMat* darknet_yolo::solve_perspective(std::vector<bbox_t> result_vec) {
//获取虚拟线圈角点和世界坐标系点

CvPoint VirtualLoopPiont[4];
CvPoint WorldPlane[4];
LoadFromTXT("E:/exam/710/Firstphase/Firstphase/ViutualLoopPoint.txt", VirtualLoopPiont);
LoadFromTXT("E:/exam/710/Firstphase/Firstphase/WorldPlane.txt", WorldPlane);
float pFP[] = {
float(VirtualLoopPiont[0].x), float(VirtualLoopPiont[0].y),
float(VirtualLoopPiont[1].x), float(VirtualLoopPiont[1].y),
float(VirtualLoopPiont[2].x), float(VirtualLoopPiont[2].y),
float(VirtualLoopPiont[3].x), float(VirtualLoopPiont[3].y) };
float pHP[] = {
float(WorldPlane[0].x), float(WorldPlane[0].y),
float(WorldPlane[1].x), float(WorldPlane[1].y),
float(WorldPlane[2].x), float(WorldPlane[2].y),
float(WorldPlane[3].x), float(WorldPlane[3].y) };
//求解透视变换矩阵
CvMat* pFramePoints = new CvMat;
CvMat* pWorldPoints = new CvMat;
CvMat* homograph = new CvMat;
cvInitMatHeader(pFramePoints, 4, 2, CV_32F, pFP);
cvInitMatHeader(pWorldPoints, 4, 2, CV_32F, pHP);
cvInitMatHeader(homograph, 3, 3, CV_32F);
cvFindHomography(pFramePoints, pWorldPoints, homograph);

return homograph;
}
*/
/*
void darknet_yolo::get_worldcoordinate(std::vector<bbox_t> result_vec, CvMat* homograph)
{

	int person_num = result_vec.size();

	double h11 = cvmGet(homograph, 0, 0); double h12 = cvmGet(homograph, 0, 1); double h13 = cvmGet(homograph, 0, 2);
	double h21 = cvmGet(homograph, 1, 0); double h22 = cvmGet(homograph, 1, 1); double h23 = cvmGet(homograph, 1, 2);
	double h31 = cvmGet(homograph, 2, 0); double h32 = cvmGet(homograph, 2, 1); double h33 = cvmGet(homograph, 2, 2);
	//获取人的世界坐标
	vector<CvPoint>person_frame, person_world;
	vector<CvPoint>::iterator iter1;
	//i.x i.y i.w  i.h

	CvPoint pf;
	for (auto &i : result_vec) {
		pf.x = i.x + i.w / 2;
		pf.y = i.y + i.h;
		person_frame.push_back(pf);
	}

	for (int iter = 0; iter<person_num; iter++)
	{
		person_world[iter].x = int((h11*double(person_frame[iter].x) + h12*double(person_frame[iter].y) + h13) / (h31*double(person_frame[iter].x) + h32*double(person_frame[iter].y) + h33));
		person_world[iter].y = int((h21*double(person_frame[iter].x) + h22*double(person_frame[iter].y) + h23) / (h31*double(person_frame[iter].x) + h32*double(person_frame[iter].y) + h33));
	}

}
*/
