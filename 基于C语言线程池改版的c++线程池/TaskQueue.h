#pragma once
#include <queue>
#include <pthread.h>

// ��������ṹ��
//using callback = void(*)(void*);
struct Task
{
    //�޲ι���
    Task()
    {
        function = nullptr;
        arg = nullptr;
    }

    //�вι���
    Task(void(*f)(void*), void* arg)
    {
        function = f;
        this->arg = arg;
    }

    void(* function)(void*);
    void* arg;
};


//�������
class TaskQueue
{
public:
    TaskQueue();
    ~TaskQueue();

    // �������
    void addTask(Task& task);//�޲�
    void addTask(void(*f)(void*), void* arg);//�в�

    // ȡ������
    Task takeTask();

    // ��ȡ��ǰ�������
    inline size_t taskNumber()//���ﲻ�ü���,�õ��ĵط�������
    {
        return m_taskQ.size();
    }
    

private:
    std::queue<Task> m_taskQ;//�������,������
    pthread_mutex_t m_mutex;
};

