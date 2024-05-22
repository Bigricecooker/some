#pragma once
#include <queue>
#include <pthread.h>

// 定义任务结构体
//using callback = void(*)(void*);
struct Task
{
    //无参构造
    Task()
    {
        function = nullptr;
        arg = nullptr;
    }

    //有参构造
    Task(void(*f)(void*), void* arg)
    {
        function = f;
        this->arg = arg;
    }

    void(* function)(void*);
    void* arg;
};


//任务队列
class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    // 添加任务
    void addTask(Task& task);//无参
    void addTask(void(*f)(void*), void* arg);//有参

    // 取出任务
    Task takeTask();

    // 获取当前任务个数
    inline size_t taskNumber()//这里不用加锁,用到的地方都加了
    {
        return m_taskQ.size();
    }
    

private:
    std::queue<Task> m_taskQ;//存放任务,无上限
    pthread_mutex_t m_mutex;
};

