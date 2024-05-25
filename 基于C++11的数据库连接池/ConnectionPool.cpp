#include "ConnectionPool.h"
using namespace Json;

ConnectionPool* ConnectionPool::getConnectionPool()
{
    //pool�Ǿ�̬�ľֲ�����,����������������������,�������������ں�ȫ�ֱ�������
    static ConnectionPool pool;

    //����Ҫ���ص�ַ
    return &pool;
}

bool ConnectionPool::parseJsonFile()
{
    //��ȡ�ļ�
    ifstream ifs("dbconf.json");

    //����json�ļ�
    //ʹ�� Reader ���� rd �����ļ�����,����������洢�� Value ���� root ��
    Reader rd;
    Value root;
    rd.parse(ifs, root);

    //��� root �Ƿ���һ������.�����,�������ȡ���ò���
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
    //һֱ����
    while (true)
    {
        /*
          locker(m_mutexQ); : ��һ�д��봴����һ����Ϊlocker��unique_lock���󣬲���m_mutexQ��Ϊ�������ݸ�����
          �Ӷ�������m_mutexQ����������Դ����unique_lock����locker�����������ڣ�m_mutexQ��һֱ���ڱ�������״̬��
          ֱ��locker�������٣������������򣩣�������m_mutexQ�Żᱻ������
          ���m_mutexQ�Ѿ�������,����̻߳�����������
         */
        // �������⻹���е�����
        unique_lock<mutex>locker(m_mutexQ);//������Ϊ���whileѭ��

        //�ж��Ƿ�Ҫ������������,�������������������߳�
        while (m_connectionQ.size() >= m_minSize)//��while������if��ԭ���Ƿ�ֹ���ж�������߳�ʱ������,�����򲻻�����������
        {
            m_cond.wait(locker);
        }

        //��������
        addConnection();

        //������������̱߳��������������������߳�(����getConnection��)
        //�����ж���������߳�Ҳ��Ҫ��,�����Ǹ�ѭ��
        m_cond.notify_all();
    }
}

void ConnectionPool::recyclerConnection()
{
    //һֱ����
    while (true)
    {
        //ÿ3����һ��
        //ʹ�õ���C++11�еĺ���sleep_for
        //����sleep
        //seconds:��   milliseconds:����
        this_thread::sleep_for(chrono::seconds(3));

        //����
        lock_guard<mutex>locker(m_mutexQ);

        //�ж��Ƿ�Ҫ��������
        while (m_connectionQ.size() > m_minSize)
        {
            //�ж����Ӷ��ж�ͷԪ���Ƿ�ȴ���ʱ,��ͷ��������������ʱ���
            //����ע��,���ڶ����е�����һ���ǿ��е�,Ҫô���Ǹմ�����,Ҫô�Ǹ������껹������
            MysqlConn* conn = m_connectionQ.front();

            //�����ʱ������
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

void ConnectionPool::addConnection()//����Ҫ����Ҳ���ڵ�����������ĵط���
{
    //����MySQL���Ӷ���
    MysqlConn* conn = new MysqlConn;

    //��ʼ�����Ӷ���
    if (!(conn->connect(m_user, m_passwd, m_dbName, m_ip, m_port)))
    {
        cerr << "Failed to connect to the database" << endl;
        return ;
    }

    //���ӳɹ����¼��ǰʱ��
    conn->refreshAliveTime();

    //�����Ӵ������
    m_connectionQ.push(conn);
}

shared_ptr<MysqlConn> ConnectionPool::getConnection()
{
    unique_lock<mutex>locker(m_mutexQ);

    //�ж����ӳض����Ƿ�Ϊ��
    while (m_connectionQ.empty())
    {
        //����ǳ���������ʱ��,˵�����л���Ϊ��
        if (cv_status::timeout == m_cond.wait_for(locker, chrono::milliseconds(m_timeout)))
        {
            if (m_connectionQ.empty())
            {
                //return nullptr;
                continue;
            }
        }
    }

    //�Ӷ����л�ȡһ�����Ӳ�����Ӷ����е���
    //�ڶ�������Ϊɾ����,��ָ��������������������
    //[]��ָ������������׽�ⲿ�����ķ�ʽ
    shared_ptr<MysqlConn> connnptr(m_connectionQ.front(), [this](MysqlConn* conn) {
        //������lock_guard<mutex>locker(m_mutexQ)
        //��locker������ʱm_mutexQ�Զ�����,�������ַ�ʽ��֮ǰ��unique_lock<mutex>locker(m_mutexQ)�޷����Ƽ����ķ�Χ
        lock_guard<mutex>locker(m_mutexQ);
        //m_mutexQ.lock();
        conn->refreshAliveTime();
        m_connectionQ.push(conn); 
       // m_mutexQ.unlock();
        });

    m_connectionQ.pop();

    //����������̱߳����������������߳�(����producterConnection��)
    m_cond.notify_all();//�������Ҳ�����������������߳���û��Ӱ���,��Ϊ������ж��Ƿ���������ѭ������,�������Ϊ�����ǻ��ǻ�����������

    return connnptr;
}

ConnectionPool::~ConnectionPool()
{
    //�ж���������Ƿ�Ϊ��,��Ϊ���򽫶�����ղ��ͷ������ڴ�
    while (!m_connectionQ.empty())
    {
        MysqlConn* conn = m_connectionQ.front();
        m_connectionQ.pop();
        delete conn;
    }
}

ConnectionPool::ConnectionPool()
{
    //���������ļ�
    if (!parseJsonFile())
    {
        return;
    }

    //��ʼ������
    for (int i = 0; i < m_minSize; i++)
    {
        //��������
        addConnection();
    }

    //�����������ӵ��߳�
    //C++11�ṩ���߳̿�
    //���ﲻʹ��POSIX�߳̿�
    thread producter(&ConnectionPool::producterConnection, this);

    //�����������ӵ��߳�
    thread recycler(&ConnectionPool::recyclerConnection, this);

    //�������߳�
    //ʹ���̲߳���Ҫ�ȴ��������߳���ɣ��Ϳ��Լ���ִ����������
    //�൱��POSIX�߳̿��pthread_detach
    producter.detach();
    recycler.detach();
}