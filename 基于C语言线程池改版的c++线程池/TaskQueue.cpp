#include "TaskQueue.h"

// 构造
TaskQueue::TaskQueue()
{
	//初始化互斥锁
	pthread_mutex_init(&this->m_mutex,NULL);
}

// 析构
TaskQueue::~TaskQueue()
{
	//销毁互斥锁
	pthread_mutex_destroy(&this->m_mutex);
}

// 添加任务
void TaskQueue::addTask(Task& task)//无参
{
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQ.push(task);
	pthread_mutex_unlock(&this->m_mutex);
}

// 添加任务
void TaskQueue::addTask(void(*f)(void*), void* arg)//有参
{
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQ.push(Task(f,arg));
	pthread_mutex_unlock(&this->m_mutex);
}

// 取出任务
Task TaskQueue::takeTask()
{
	Task t;
	pthread_mutex_lock(&m_mutex);
	if (m_taskQ.size() > 0)
	{
		t = m_taskQ.front();
		m_taskQ.pop();
	}
	pthread_mutex_unlock(&m_mutex);
	return t;//这里不要紧,后面判断
}
