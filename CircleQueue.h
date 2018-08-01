#pragma once
#include"stdafx.h"

#define queueSize 15

class CircleQueue
{
public:
	IplImage* _pQue;//ͷָ��
	int _size;//���г���
	int _front;//�����׸�Ԫ�ص��±�
	int _rear;//����ĩβԪ�ص��±�
	CircleQueue(int size = queueSize);//���캯��
	~CircleQueue();   //��������
	void addQue(IplImage* val);  //���
	IplImage front();  //���ض�ͷԪ��
	IplImage back();  //���ض�βԪ��
	bool empty();  //�ж϶ӿ�
	bool full(); //�ж϶���
};