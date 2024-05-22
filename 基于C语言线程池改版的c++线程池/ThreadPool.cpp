#include "ThreadPool.h"

ThreadPool::ThreadPool(int min, int max)
{
	// ʵ�����������(�̳߳������������汻ʵ������)
	m_taskQ = new TaskQueue;//newʧ�ܻ��׳��쳣

    do {
        // ��ʼ���̳߳�
        m_minNum = min;
        m_maxNum = max;
        m_busyNum = 0;
        m_aliveNum = min;

        // �����̵߳�������޸��߳���������ڴ�
        m_threadIDs = new pthread_t[max];
        if (m_threadIDs == nullptr)
        {
            cout << "malloc thread_t[] ʧ��...." << endl;;
            break;
        }

        // ��ʼ�������߳�ID
        memset(m_threadIDs, -1, sizeof(pthread_t) * max);

        // ��ʼ��������,��������
        if (pthread_mutex_init(&m_lock, NULL) != 0 ||
            pthread_cond_init(&m_notEmpty, NULL) != 0)
        {
            cout << "init mutex or condition fail..." << endl;
            break;
        }

        /////////////////// �����߳� //////////////////
        // ������С�̸߳���, ���������߳�
        for (int i = 0; i < min; ++i)
        {
            //ע������,�������������Ҫ���ݷǳ�Ա�����ĺ���ָ��,���԰�worker��manager�޸�Ϊ��ľ�̬��Ա
            pthread_create(&m_threadIDs[i], NULL, worker, this);
            cout << "�������߳�, ID: " << to_string(m_threadIDs[i]) << endl;
        }
        // �����������߳�, 1��
        pthread_create(&m_managerID, NULL, manager, this);
        //(��֪�������4���ɲ����Դ�NULL)
        //����Ϊ����,��Ϊ���еľ�̬��Ա����ֻ�ܷ��ʾ�̬��Ա����
        //�������this,�ǹ����̺߳͹������̷߳��ʲ������е���������
    } while (0);
}

ThreadPool::~ThreadPool()
{
    //�ر��̳߳�
    m_shutdown = true;

    //�������չ������߳�
    pthread_join(m_managerID, NULL);

    //�������б������Ĺ����߳�ʹ�䱻����
    for (int i = 0; i < m_aliveNum; i++)
    {
        pthread_cond_signal(&m_notEmpty);
    }

    //������
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notEmpty);

    //�ͷŶ��ڴ�
    if (m_threadIDs)
        delete[]m_threadIDs;//ʹ��new[]�����ľ�Ҫ��delete[]����
    if(m_taskQ)
        delete m_taskQ;
}

void ThreadPool::addTask(Task task)
{
    //�ж��̳߳��Ƿ񱻹ر�
    if (m_shutdown)
    {
        return;
    }

    // ������񣬲���Ҫ�������������������
    m_taskQ->addTask(task);

    //��ʱ����������в�Ϊ��,���ѱ����������������߳�
    pthread_cond_signal(&m_notEmpty);
}

int ThreadPool::getBusyNum()
{
    int busyNum = 0;
    pthread_mutex_lock(&m_lock);
    busyNum = m_busyNum;
    pthread_mutex_unlock(&m_lock);
    return busyNum;
	return 0;
}

int ThreadPool::getPoolAliveNum()
{
    int threadNum = 0;
    pthread_mutex_lock(&m_lock);
    threadNum = m_aliveNum;
    pthread_mutex_unlock(&m_lock);
    return threadNum;
	return 0;
}

void* ThreadPool::worker(void* arg)
{
    //C++��ǿ������ת��
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    //C��ǿ������ת��
    //ThreadPool* pool = (ThreadPool*)arg;
    // ѭ���Ķ�ȡ�������
        while (1)
        {
            //�����̳߳�
            pthread_mutex_lock(&pool->m_lock);

            //�ж���������Ƿ�Ϊ��
            while (pool->m_taskQ->taskNumber() == 0 && !pool->m_shutdown)//ѭ������Ϊ����Ҫ���ٶ���߳�
            {
                // ���������߳�(������Ĺ����߳�������߳���ɱ���������������������)
                //
                pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

                /*--------------------------------------------------*/
                // �ж��ǲ���Ҫ�����߳�
                if (pool->m_exitNum > 0)
                {
                    pool->m_exitNum--;
                    if (pool->m_aliveNum > pool->m_minNum)
                    {
                        pool->m_aliveNum--;
                        pthread_mutex_unlock(&pool->m_lock);
                        pool->threadExit();//worker�Ǿ�̬��Ա����,��������÷Ǿ�̬��Ա����,����Ҫpool->
                    }
                }
                /*--------------------------------------------------*/
            }


            // �ж��̳߳��Ƿ񱻹ر���
            if (pool->m_shutdown)
            {
                pthread_mutex_unlock(&pool->m_lock);
                pool->threadExit();
            }

            //��ʼ������������е�����
            // �����������ȡ��һ������
            Task task = pool->m_taskQ->takeTask();

            // �������߳�+1
            pool->m_busyNum++;

            // �̳߳ؽ���
            pthread_mutex_unlock(&pool->m_lock);
           
            // ִ��������������ɺ�����
            cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
            task.function(task.arg);
            delete task.arg;
            task.arg = nullptr;

            // ���������
            cout << "thread " << to_string(pthread_self()) << " end working..." << endl;
            pthread_mutex_lock(&pool->m_lock);
            pool->m_busyNum--;
            pthread_mutex_unlock(&pool->m_lock);
        }
	return nullptr;
}

void* ThreadPool::manager(void* arg)
{
    //C++��ǿ������ת��
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    // ����̳߳�û�йر�, ��һֱ���
    while (!pool->m_shutdown)
    {
        //ÿ3����һ��
        sleep(3);

        // ȡ���̳߳��е����������߳�����
        //  ȡ���������̳߳�����
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->taskNumber();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_lock);

        // �����߳�
        const int NUMBER = 2;

        // ��ǰ�������>�����߳��� && �����߳���<����̸߳���
        if (queueSize > liveNum && liveNum < pool->m_maxNum)
        {
            // �̳߳ؼ���
            pthread_mutex_lock(&pool->m_lock);

            int num = 0;
            for (int i = 0; i < pool->m_maxNum && num < NUMBER
                && pool->m_aliveNum < pool->m_maxNum; ++i)
            {
                if (pool->m_threadIDs[i] == -1)
                {
                    pthread_create(&pool->m_threadIDs[i], NULL, worker, pool);
                    num++;
                    pool->m_aliveNum++;
                }
            }
            pthread_mutex_unlock(&pool->m_lock);
        }

        // ���ٶ�����߳�
        // ������ʵ��ʹ�ó�Ա����m_exitNumʹ����Ĺ����߳���ɱ
        // æ�߳�*2 < �����߳���Ŀ && �����߳��� > ��С�߳�����
        if (busyNum * 2 < liveNum && liveNum > pool->m_minNum)
        {
            pthread_mutex_lock(&pool->m_lock);
            pool->m_exitNum = NUMBER;
            pthread_mutex_unlock(&pool->m_lock);
            for (int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->m_notEmpty);
            }
        }
    }
	return nullptr;
}

void ThreadPool::threadExit()
{
    //��ȡ���̵߳�ID
    pthread_t tid = pthread_self();
    
    //�ڹ����߳������н������߳�ID��Ӧλ�ø�Ϊ-1
    for (int i = 0; i < m_maxNum; i++)
    {
        if (m_threadIDs[i] == tid)
        {
            cout << "threadExit() function: thread "
                << to_string(pthread_self()) << " exiting..." << endl;
            m_threadIDs[i] = -1;
        }
    }

    //�˳����߳�
    pthread_exit(NULL);
}
