#pragma once
#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "TaskQueue.h"
using namespace std;
class ThreadPool
{
public:
    // �����̳߳ز���ʼ��
    ThreadPool(int min, int max);

    // �����̳߳�
    ~ThreadPool();

    // ���̳߳��������
    void addTask(Task task);

    // �����������о�ֻ��Ϊ���ô��뿴��������
    // ��ȡ�̳߳��й������̵߳ĸ���
    int getBusyNum();

    // ��ȡ�̳߳��л��ŵ��̵߳ĸ���
    int getPoolAliveNum();

private:
    // �������߳�(�������߳�)������
    static void* worker(void* arg);

    // �������߳�������
    static void* manager(void* arg);

    // �����߳��˳�
    void threadExit();
private:
    pthread_mutex_t m_lock;     //�̳߳ص���
    pthread_cond_t m_notEmpty;  //��������(�������Ϊ��ʱ���������߳�,��������������������)
    pthread_t* m_threadIDs;     //�����߳�ID
    pthread_t m_managerID;      //�������߳�ID

	TaskQueue* m_taskQ;           //ָ��������е�ָ��

    int m_minNum;               //��С�߳�����
    int m_maxNum;               //����߳�����
    int m_busyNum;              //æ���̸߳���
    int m_aliveNum;             //�����̸߳���
    int m_exitNum;              //��Ҫ���ٵ��̸߳���
    bool m_shutdown = false;



};

