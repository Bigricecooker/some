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
    // 创建线程池并初始化
    ThreadPool(int min, int max);

    // 销毁线程池
    ~ThreadPool();

    // 给线程池添加任务
    void addTask(Task task);

    // 下面这两个感觉只是为了让代码看起来更好
    // 获取线程池中工作的线程的个数
    int getBusyNum();

    // 获取线程池中活着的线程的个数
    int getPoolAliveNum();

private:
    // 工作的线程(消费者线程)任务函数
    static void* worker(void* arg);

    // 管理者线程任务函数
    static void* manager(void* arg);

    // 单个线程退出
    void threadExit();
private:
    pthread_mutex_t m_lock;     //线程池的锁
    pthread_cond_t m_notEmpty;  //条件变量(任务队列为空时阻塞工作线程,生产者生产了任务后解锁)
    pthread_t* m_threadIDs;     //工作线程ID
    pthread_t m_managerID;      //管理者线程ID

	TaskQueue* m_taskQ;           //指向任务队列的指针

    int m_minNum;               //最小线程数量
    int m_maxNum;               //最大线程数量
    int m_busyNum;              //忙的线程个数
    int m_aliveNum;             //存活的线程个数
    int m_exitNum;              //将要销毁的线程个数
    bool m_shutdown = false;



};

