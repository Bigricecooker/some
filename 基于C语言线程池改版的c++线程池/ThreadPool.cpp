#include "ThreadPool.h"

ThreadPool::ThreadPool(int min, int max)
{
	// 实例化任务队列(线程池自身是在外面被实例化的)
	m_taskQ = new TaskQueue;//new失败会抛出异常

    do {
        // 初始化线程池
        m_minNum = min;
        m_maxNum = max;
        m_busyNum = 0;
        m_aliveNum = min;

        // 根据线程的最大上限给线程数组分配内存
        m_threadIDs = new pthread_t[max];
        if (m_threadIDs == nullptr)
        {
            cout << "malloc thread_t[] 失败...." << endl;;
            break;
        }

        // 初始化工作线程ID
        memset(m_threadIDs, -1, sizeof(pthread_t) * max);

        // 初始化互斥锁,条件变量
        if (pthread_mutex_init(&m_lock, NULL) != 0 ||
            pthread_cond_init(&m_notEmpty, NULL) != 0)
        {
            cout << "init mutex or condition fail..." << endl;
            break;
        }

        /////////////////// 创建线程 //////////////////
        // 根据最小线程个数, 创建工作线程
        for (int i = 0; i < min; ++i)
        {
            //注意这里,这里第三个参数要传递非成员函数的函数指针,所以把worker和manager修改为类的静态成员
            pthread_create(&m_threadIDs[i], NULL, worker, this);
            cout << "创建子线程, ID: " << to_string(m_threadIDs[i]) << endl;
        }
        // 创建管理者线程, 1个
        pthread_create(&m_managerID, NULL, manager, this);
        //(不知道这里第4个可不可以传NULL)
        //鉴定为不能,因为类中的静态成员函数只能访问静态成员变量
        //如果不传this,那工作线程和管理者线程访问不到类中的其他变量
    } while (0);
}

ThreadPool::~ThreadPool()
{
    //关闭线程池
    m_shutdown = true;

    //阻塞回收管理者线程
    pthread_join(m_managerID, NULL);

    //唤醒所有被阻塞的工作线程使其被销毁
    for (int i = 0; i < m_aliveNum; i++)
    {
        pthread_cond_signal(&m_notEmpty);
    }

    //销毁锁
    pthread_mutex_destroy(&m_lock);
    pthread_cond_destroy(&m_notEmpty);

    //释放堆内存
    if (m_threadIDs)
        delete[]m_threadIDs;//使用new[]创建的就要用delete[]销毁
    if(m_taskQ)
        delete m_taskQ;
}

void ThreadPool::addTask(Task task)
{
    //判断线程池是否被关闭
    if (m_shutdown)
    {
        return;
    }

    // 添加任务，不需要加锁，任务队列中有锁
    m_taskQ->addTask(task);

    //此时任务任务队列不为空,唤醒被条件变量阻塞的线程
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
    //C++的强制类型转换
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    //C的强制类型转换
    //ThreadPool* pool = (ThreadPool*)arg;
    // 循环的读取任务队列
        while (1)
        {
            //加锁线程池
            pthread_mutex_lock(&pool->m_lock);

            //判断任务队列是否为空
            while (pool->m_taskQ->taskNumber() == 0 && !pool->m_shutdown)//循环是因为可能要销毁多个线程
            {
                // 阻塞工作线程(与下面的管理线程配合让线程自杀或生产者生产了任务后唤醒)
                //
                pthread_cond_wait(&pool->m_notEmpty, &pool->m_lock);

                /*--------------------------------------------------*/
                // 判断是不是要销毁线程
                if (pool->m_exitNum > 0)
                {
                    pool->m_exitNum--;
                    if (pool->m_aliveNum > pool->m_minNum)
                    {
                        pool->m_aliveNum--;
                        pthread_mutex_unlock(&pool->m_lock);
                        pool->threadExit();//worker是静态成员函数,而这里调用非静态成员函数,所以要pool->
                    }
                }
                /*--------------------------------------------------*/
            }


            // 判断线程池是否被关闭了
            if (pool->m_shutdown)
            {
                pthread_mutex_unlock(&pool->m_lock);
                pool->threadExit();
            }

            //开始消费任务队列中的任务
            // 从任务队列中取出一个任务
            Task task = pool->m_taskQ->takeTask();

            // 工作的线程+1
            pool->m_busyNum++;

            // 线程池解锁
            pthread_mutex_unlock(&pool->m_lock);
           
            // 执行任务并在任务完成后销毁
            cout << "thread " << to_string(pthread_self()) << " start working..." << endl;
            task.function(task.arg);
            delete task.arg;
            task.arg = nullptr;

            // 任务处理结束
            cout << "thread " << to_string(pthread_self()) << " end working..." << endl;
            pthread_mutex_lock(&pool->m_lock);
            pool->m_busyNum--;
            pthread_mutex_unlock(&pool->m_lock);
        }
	return nullptr;
}

void* ThreadPool::manager(void* arg)
{
    //C++的强制类型转换
    ThreadPool* pool = static_cast<ThreadPool*>(arg);

    // 如果线程池没有关闭, 就一直检测
    while (!pool->m_shutdown)
    {
        //每3秒检测一次
        sleep(3);

        // 取出线程池中的任务数和线程数量
        //  取出工作的线程池数量
        pthread_mutex_lock(&pool->m_lock);
        int queueSize = pool->m_taskQ->taskNumber();
        int liveNum = pool->m_aliveNum;
        int busyNum = pool->m_busyNum;
        pthread_mutex_unlock(&pool->m_lock);

        // 创建线程
        const int NUMBER = 2;

        // 当前任务个数>存活的线程数 && 存活的线程数<最大线程个数
        if (queueSize > liveNum && liveNum < pool->m_maxNum)
        {
            // 线程池加锁
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

        // 销毁多余的线程
        // 这里其实是使用成员变量m_exitNum使多余的工作线程自杀
        // 忙线程*2 < 存活的线程数目 && 存活的线程数 > 最小线程数量
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
    //获取该线程的ID
    pthread_t tid = pthread_self();
    
    //在工作线程数组中将该与线程ID对应位置改为-1
    for (int i = 0; i < m_maxNum; i++)
    {
        if (m_threadIDs[i] == tid)
        {
            cout << "threadExit() function: thread "
                << to_string(pthread_self()) << " exiting..." << endl;
            m_threadIDs[i] = -1;
        }
    }

    //退出该线程
    pthread_exit(NULL);
}
