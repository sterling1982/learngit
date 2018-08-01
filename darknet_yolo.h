#pragma once
#include"stdafx.h"
#include "yolo_v2_class.hpp"

using namespace std;
using namespace cv;

class darknet_yolo {
public:
	//用方框框住检测到的人
	Mat draw_boxes(Mat mat_img, vector<bbox_t> result_vec, vector<std::string> obj_names, unsigned int wait_msec = 0);
	//CvMat* solve_perspective(std::vector<bbox_t> result_vec);
	//void get_worldcoordinate(std::vector<bbox_t> result_vec, CvMat* homograph);
	//得到人的世界坐标
	vector<Point2f> get_worldcoordinate(std::vector<bbox_t> result_vec, Mat homograph);
	//void transmit_result(std::vector<bbox_t> result_vec, vector<Point2f> worldcoordinate,int id);
	//传输检测结果
	void transmit_result(std::vector<bbox_t> result_vec);

};
//求解透视变换矩阵
Mat solve_perspective();
//读取区域坐标
void LoadFromTXT(const char* txtFilename, CvPoint* cooConer);
//将数字转为string
string num2str(int num);
string num2str(float num);
//获取毫秒级别的时间戳
long long getTimeStampMs();

