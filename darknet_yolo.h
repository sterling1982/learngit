#pragma once
#include"stdafx.h"
#include "yolo_v2_class.hpp"

using namespace std;
using namespace cv;

class darknet_yolo {
public:
	//�÷����ס��⵽����
	Mat draw_boxes(Mat mat_img, vector<bbox_t> result_vec, vector<std::string> obj_names, unsigned int wait_msec = 0);
	//CvMat* solve_perspective(std::vector<bbox_t> result_vec);
	//void get_worldcoordinate(std::vector<bbox_t> result_vec, CvMat* homograph);
	//�õ��˵���������
	vector<Point2f> get_worldcoordinate(std::vector<bbox_t> result_vec, Mat homograph);
	//void transmit_result(std::vector<bbox_t> result_vec, vector<Point2f> worldcoordinate,int id);
	//��������
	void transmit_result(std::vector<bbox_t> result_vec);

};
//���͸�ӱ任����
Mat solve_perspective();
//��ȡ��������
void LoadFromTXT(const char* txtFilename, CvPoint* cooConer);
//������תΪstring
string num2str(int num);
string num2str(float num);
//��ȡ���뼶���ʱ���
long long getTimeStampMs();

