#include "ConnectionPool.h"
using namespace Json;

ConnectionPool* ConnectionPool::getConnectionPool()
{
    //pool是静态的局部变量,它的作用域就是这个函数内,但它的生命周期和全局变量类似
    static ConnectionPool pool;

    //所以要返回地址
    return &pool;
}

bool ConnectionPool::parseJsonFile()
{
    //读取文件
    ifstream ifs("dbconf.json");

    //解析json文件
    //使用 Reader 对象 rd 解析文件内容,将解析结果存储在 Value 对象 root 中
    Reader rd;
    Value root;
    rd.parse(ifs, root);

    //检查 root 是否是一个对象.如果是,则从中提取配置参数
    if (root.isObject())
    {
        try {
            m_ip = root["ip"].asString();
            m_user = root["user"].asString();
            m_dbName = root["dbname"].asString();
            m_passwd = root["passwd"].asString();
            m_port = root["port"].asUInt();
            m_minSize = root["minsize"].asInt();
            m_maxSize = root["maxsize"].asInt();
            m_maxIdleTime = root["maxidletime"].asInt();
            m_timeout = root["timeout"].asInt();
        }
        catch (const Json::LogicError& e) {
            cerr << "Error parsing JSON fields: " << e.what() << endl;
            return false;
        }
        return true;
    }
    return false;
}

void ConnectionPool::producterConnection()
{
    //一直运行
    while (true)
    {
        /*
          locker(m_mutexQ); : 这一行代码创建了一个名为locker的unique_lock对象，并将m_mutexQ作为参数传递给它，
          从而锁定了m_mutexQ所保护的资源。在unique_lock对象locker的生命周期内，m_mutexQ将一直处于被锁定的状态，
          直到locker对象被销毁（超出其作用域），互斥量m_mutexQ才会被解锁。
          如果m_mutexQ已经被锁了,则该线程会阻塞在这里
         */
        // 这里的理解还是有点问题
        unique_lock<mutex>locker(m_mutexQ);//作用域为这个while循环

        //判断是否要增加连接数量,不增加则阻塞该生产线程
        while (m_connectionQ.size() >= m_minSize)//用while而不是if的原因是防止当有多个生产线程时出问题,本程序不会出现这个问题
        {
            m_cond.wait(locker);
        }

        //增加连接
        addConnection();

        //如果有消费者线程被阻塞则唤醒所有消费者线程(调用getConnection的)
        //这里有多个生产者线程也不要紧,上面是个循环
        m_cond.notify_all();
    }
}

void ConnectionPool::recyclerConnection()
{
    //一直运行
    while (true)
    {
        //每3秒检测一次
        //使用的是C++11中的函数sleep_for
        //不用sleep
        //seconds:秒   milliseconds:毫秒
        this_thread::sleep_for(chrono::seconds(3));

        //加锁
        lock_guard<mutex>locker(m_mutexQ);

        //判断是否要销毁连接
        while (m_connectionQ.size() > m_minSize)
        {
            //判断连接队列队头元素是否等待超时,队头如果空闲则其空闲时间最长
            //这里注意,留在队列中的连接一定是空闲的,要么就是刚创建的,要么是刚连接完还回来的
            MysqlConn* conn = m_connectionQ.front();

            //如果超时则销毁
            if (conn->getAliveTime() >= m_maxIdleTime)
            {
                m_connectionQ.pop();
                delete conn;
            }
            else
            {
                break;
            }
        }
    }
}

void ConnectionPool::addConnection()//这里要加锁也是在调用这个函数的地方加
{
    //创建MySQL连接对象
    MysqlConn* conn = new MysqlConn;

    //初始化连接对象
    if (!(conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port)))
    {
        cerr << "Failed to connect to the database" << endl;
        return ;
    }

    //连接成功则记录当前时间
    conn->refreshAliveTime();

    //将连接存入队列
    m_connectionQ.push(conn);
}

shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    unique_lock<mutex>locker(m_mutexQ);

    //判断连接池队列是否为空
    while (m_connectionQ.empty())
    {
        //如果是超过了阻塞时间,说明队列还是为空
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            if (m_connectionQ.empty())
            {
                //return nullptr;
                continue;
            }
        }
    }

    //从队列中获取一个连接并将其从队列中弹出
    //第二个参数为删除器,可指定有名函数和匿名函数
    //[]里指定匿名函数捕捉外部变量的方式
    shared_ptr<MysqlConn> connnptr(m_connectionQ.front(), [this](MysqlConn* conn) {
        //可以用lock_guard<mutex>locker(m_mutexQ)
        //当locker被析构时m_mutexQ自动解锁,不过这种方式和之前的unique_lock<mutex>locker(m_mutexQ)无法控制加锁的范围
        lock_guard<mutex>locker(m_mutexQ);
        //m_mutexQ.lock();
        conn->refreshAliveTime();
        m_connectionQ.push(conn); 
       // m_mutexQ.unlock();
        });

    m_connectionQ.pop();

    //如果生产者线程被阻塞则唤醒生产者线程(调用producterConnection的)
    m_cond.notify_all();//这里如果也唤醒了其他消费者线程是没有影响的,因为上面的判断是否阻塞是在循环里面,如果队列为空它们还是会阻塞在上面

    return connnptr;
}

ConnectionPool::~ConnectionPool()
{
    //判断任务队列是否为空,不为空则将队列清空并释放连接内存
    while (!m_connectionQ.empty())
    {
        MysqlConn* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}

ConnectionPool::ConnectionPool()
{
    //加载配置文件
    if (!parseJsonFile())
    {
        return;
    }

    //初始化连接
    for (int i = 0; i < m_minSize; i++)
    {
        //增加连接
        addConnection();
    }

    //创建生产连接的线程
    //C++11提供的线程库
    //这里不使用POSIX线程库
    thread producter(&ConnectionPool::producterConnection, this);

    //创建销毁连接的线程
    thread recycler(&ConnectionPool::recyclerConnection, this);

    //分离主线程
    //使主线程不需要等待这两个线程完成，就可以继续执行其他任务
    //相当与POSIX线程库的pthread_detach
    producter.detach();
    recycler.detach();
}