#pragma once
#include"stdafx.h"

#define queueSize 15

class CircleQueue
{
public:
	IplImage* _pQue;//头指针
	int _size;//队列长度
	int _front;//队列首个元素的下标
	int _rear;//队列末尾元素的下标
	CircleQueue(int size = queueSize);//构造函数
	~CircleQueue();   //析构函数
	void addQue(IplImage* val);  //入队
	IplImage front();  //返回队头元素
	IplImage back();  //返回队尾元素
	bool empty();  //判断队空
	bool full(); //判断队满
};