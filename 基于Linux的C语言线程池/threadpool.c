#include "threadpool.h"
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
// ����ṹ��
typedef struct Task
{
    void (*function)(void* arg);
    void* arg;
}Task;



// �̳߳ؽṹ��
struct ThreadPool
{
    // �������
    Task* taskQ;
    int queueCapacity;  // ����
    int queueSize;      // ��ǰ�������
    int queueFront;     // ��ͷ -> ȡ����
    int queueRear;      // ��β -> ������

    pthread_t managerID;    // �������߳�ID
    pthread_t* threadIDs;   // �������߳�ID
    int minNum;             // ��С�߳�����
    int maxNum;             // ����߳�����
    int busyNum;            // æ���̵߳ĸ���
    int liveNum;            // �����̵߳ĸ���
    int exitNum;            // Ҫ���ٵ��̸߳���
    pthread_mutex_t mutexPool;  // ���������̳߳�
    pthread_mutex_t mutexBusy;  // ��busyNum����
    pthread_cond_t notFull;     // ��������ǲ�������
    pthread_cond_t notEmpty;    // ��������ǲ��ǿ���

    int shutdown;           // �ǲ���Ҫ�����̳߳�, ����Ϊ1, ������Ϊ0
};

ThreadPool* threadPoolCreate(int min, int max, int queueSize)
{
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do
    {
        //�����̳߳��ڴ�
        if (pool == NULL)
        {
            printf("malloc threadpool fail...\n");
            break;
        }

        //���乤���߳�ID���ڴ沢��ʼ��
        pool->threadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if (pool->threadIDs == NULL)
        {
            printf("malloc threadIDs fail...\n");
            break;
        }
        memset(pool->threadIDs, -1, sizeof(pthread_t) * max);

        //��ʼ���ṹ����
        pool->minNum = min;
        pool->maxNum = max;
        pool->busyNum = 0;
        pool->liveNum = min;    //�����̸߳�����ʼ����С�������
        pool->exitNum = 0;

        //��ʼ������������������
        if (pthread_mutex_init(&pool->mutexPool, NULL) != 0 ||
            pthread_mutex_init(&pool->mutexBusy, NULL) != 0 ||
            pthread_cond_init(&pool->notEmpty, NULL) != 0 ||
            pthread_cond_init(&pool->notFull, NULL) != 0)
        {
            printf("mutex or condition init fail...\n");
            break;
        }

        //��ʼ���������
        pool->taskQ = (Task*)malloc(sizeof(Task) * queueSize);
        pool->queueCapacity = queueSize;
        pool->queueSize = 0;
        pool->queueFront = 0;
        pool->queueRear = 0;

        //
        pool->shutdown = 0;

        // �����������̺߳���С�����Ĺ����߳�
        pthread_create(&pool->managerID, NULL, manager, pool);
        for (int i = 0; i < min; ++i)
        {
            pthread_create(&pool->threadIDs[i], NULL, worker, pool);
        }

        //ȫ����ʼ���ɹ�
        return pool;

    } while (0);

    //�ж�����ʼ��ʧ��,�ͷ�������Դ
    if (pool && pool->threadIDs) free(pool->threadIDs);
    if (pool && pool->taskQ) free(pool->taskQ);
    if (pool) free(pool);

    return NULL;
}

int threadPoolDestroy(ThreadPool* pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    // �ر��̳߳�
    pool->shutdown = 1;
    // �������չ������߳�
    pthread_join(pool->managerID, NULL);
    // �����������������߳�
    for (int i = 0; i < pool->liveNum; ++i)
    {
        pthread_cond_signal(&pool->notEmpty);
    }
    // �ͷŶ��ڴ�
    if (pool->taskQ)
    {
        free(pool->taskQ);
    }
    if (pool->threadIDs)
    {
        free(pool->threadIDs);
    }

    pthread_mutex_destroy(&pool->mutexPool);
    pthread_mutex_destroy(&pool->mutexBusy);
    pthread_cond_destroy(&pool->notEmpty);
    pthread_cond_destroy(&pool->notFull);

    free(pool);
    pool = NULL;

    return 0;
}

void threadPoolAdd(ThreadPool* pool, void(*func)(void*), void* arg)
{
    pthread_mutex_lock(&pool->mutexPool);
    // �жϹ��������ǲ����Ѿ�����
    while (pool->queueSize == pool->queueCapacity && !pool->shutdown)
    {
        //�����������߳�
        pthread_cond_wait(&pool->notFull, &pool->mutexPool);
    }
    //�ж��̳߳��Ƿ񱻹ر�
    if (pool->shutdown)
    {
        pthread_mutex_unlock(&pool->mutexPool);
        return;
    }

    // �������
    pool->taskQ[pool->queueRear].function = func;
    pool->taskQ[pool->queueRear].arg = arg;
    pool->queueRear = (pool->queueRear + 1) % pool->queueCapacity;
    pool->queueSize++;

    //�����˶�����Ҳ��Ҫ���������Ĺ����߳�
    pthread_cond_signal(&pool->notEmpty);

    pthread_mutex_unlock(&pool->mutexPool);

}

int threadPoolBusyNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexBusy);
    int busyNum = pool->busyNum;
    pthread_mutex_unlock(&pool->mutexBusy);
    return busyNum;
}

int threadPoolAliveNum(ThreadPool* pool)
{
    pthread_mutex_lock(&pool->mutexPool);
    int aliveNum = pool->liveNum;
    pthread_mutex_unlock(&pool->mutexPool);
    return aliveNum;
}

void* worker(void* arg)
{
    //ǿ��ת��Ϊ�̳߳ؽṹ(ֻ��һ���̳߳�)
    ThreadPool* pool = (ThreadPool*)arg;

    //ѭ���Ķ�ȡ�������
    while (1)
    {
        //�����̳߳�
        pthread_mutex_lock(&pool->mutexPool);

        //�ж���������Ƿ�Ϊ��
        while (pool->queueSize == 0 && !pool->shutdown)//ѭ������Ϊ����Ҫ���ٶ���߳�
        {
            // ���������߳�(������Ĺ����߳�������߳���ɱ���������������������)
            pthread_cond_wait(&pool->notEmpty, &pool->mutexPool);

            /*--------------------------------------------------*/
            // �ж��ǲ���Ҫ�����߳�(�����߼��ϵ�����)
            if (pool->exitNum > 0)
            {
                pool->exitNum--;
                if (pool->liveNum > pool->minNum)
                {
                    pool->liveNum--;
                    pthread_mutex_unlock(&pool->mutexPool);
                    threadExit(pool);
                }
            }
            /*--------------------------------------------------*/
        }


        // �ж��̳߳��Ƿ񱻹ر���
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&pool->mutexPool);
            threadExit(pool);
        }

        //��ʼ������������е�����
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;//ȡͷ��������
        task.arg = pool->taskQ[pool->queueFront].arg;

        // �ƶ�ͷ���
        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->queueSize--;

        //������������к��ٻ����������߳�
        pthread_cond_signal(&pool->notFull);

        //�����̳߳�
        pthread_mutex_unlock(&pool->mutexPool);

        //����æ���̸߳���(��Ϊ�������������߳�,����Ҫ����)
        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum++;
        pthread_mutex_unlock(&pool->mutexBusy);

        //ִ�����������ͷ��ڴ�
        task.function(task.arg);
        free(task.arg);
        task.arg = NULL;
        printf("thread %ld end working...\n", pthread_self());

        //����æ���̸߳���
        pthread_mutex_lock(&pool->mutexBusy);
        pool->busyNum--;
        pthread_mutex_unlock(&pool->mutexBusy);
    }
    return NULL;
}

void* manager(void* arg)
{
    //ǿ��ת��Ϊ�̳߳ؽṹ(ֻ��һ���̳߳�)
    ThreadPool* pool = (ThreadPool*)arg;

    //�������߳�һֱ�ɻ�
    while (!pool->shutdown)
    {
        //3����һ��
        sleep(3);

        //ȡ�̳߳�������������͵�ǰ�̵߳�����(Ҫ����)
        pthread_mutex_lock(&pool->mutexPool);
        int queueSize = pool->queueSize;
        int liveNum = pool->liveNum;
        pthread_mutex_unlock(&pool->mutexPool);

        // ȡ��æ���̵߳�����(��ʵ���Է�����,����������������һ��,��ΪbusyNum������)
        pthread_mutex_lock(&pool->mutexBusy);
        int busyNum = pool->busyNum;
        pthread_mutex_unlock(&pool->mutexBusy);

        //��������������߳�(����ĸ���>�����̸߳��� && �����߳���<����߳���)
        if (queueSize > liveNum && liveNum < pool->maxNum)
        {
            pthread_mutex_lock(&pool->mutexPool);
            //ÿ�μ�2���߳�
            int counter = 0;
            for (int i = 0; i < pool->maxNum && counter < 2 && pool->liveNum < pool->maxNum; i++)
            {
                if (pool->threadIDs[i] == -1)//Ѱ��û�д洢�߳�ID��
                {
                    //������һ�������߳�
                    pthread_create(&pool->threadIDs[i], NULL, worker, pool);
                    pool->liveNum++;
                    counter++;
                }
            }
            pthread_mutex_unlock(&pool->mutexPool);
        }

        //���������������߳�(æ���߳�*2 < �����߳��� && �����߳�>��С�߳���)
        //thread_mutex_lock(&pool->mutexBusy);
        if (pool->busyNum * 2 < pool->liveNum && liveNum > pool->minNum)
        {
            //pthread_mutex_unlock(&pool->mutexBusy);//����Ϊ���ﻹ�ǵü�һ��busyNum��,��Ϊ�����߳�����Ҳ�޸���busyNum��ֵ
            //byd����������˺þ�
            
            pthread_mutex_lock(&pool->mutexPool);
            pool->exitNum = 2;
            pthread_mutex_unlock(&pool->mutexPool);

            //�ù����߳���ɱ(�úú�)
            for (int i = 0; i < 2; ++i)
            {
                pthread_cond_signal(&pool->notEmpty);//�������汻�������������Ĺ����߳�
            }
        }
    }
    return NULL;
}

void threadExit(ThreadPool* pool)
{
    //��õ�ǰ�̵߳��߳�ID
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->maxNum; i++)
    {
        if (pool->threadIDs[i] == tid)
        {
            pool->threadIDs[i] = -1;
            printf("threadExit() called, %ld exiting...\n", tid);
            break;
        }
    }
    //�˳���ǰ�߳�
    pthread_exit(NULL);
}
