#include"stdafx.h"
#include "CircleQueue.h"

using namespace std;

CircleQueue::CircleQueue(int size)
{
	_pQue = new IplImage[size];
	_size = size;
	_front = 0;
	_rear = 0;
}

CircleQueue::~CircleQueue()
{
	delete[]_pQue;
	_pQue = NULL;
}

void CircleQueue::addQue(IplImage* val)
{
	if (full())
	{
		_rear = 0;
	}
	_pQue[_rear] = *val;
	_rear = ++_rear%_size;
}

IplImage CircleQueue::front()
{
	return _pQue[_front];
}

IplImage CircleQueue::back()
{
	return _pQue[_rear];
}
bool CircleQueue::empty()
{
	return (_rear - _front) == 0;
}

bool CircleQueue::full()
{

	return (_rear + 1) % _size == _front;
}