#include "TaskQueue.h"

// ����
TaskQueue::TaskQueue()
{
	//��ʼ��������
	pthread_mutex_init(&this->m_mutex,NULL);
}

// ����
TaskQueue::~TaskQueue()
{
	//���ٻ�����
	pthread_mutex_destroy(&this->m_mutex);
}

// �������
void TaskQueue::addTask(Task& task)//�޲�
{
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQ.push(task);
	pthread_mutex_unlock(&this->m_mutex);
}

// �������
void TaskQueue::addTask(void(*f)(void*), void* arg)//�в�
{
	pthread_mutex_lock(&this->m_mutex);
	this->m_taskQ.push(Task(f,arg));
	pthread_mutex_unlock(&this->m_mutex);
}

// ȡ������
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
	return t;//���ﲻҪ��,�����ж�
}
