#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <thread>
#include <json/json.h>
#include "MysqlConn.h"
using namespace std;

//��Ϊֻ��Ҫһ�����ݿ��,����������ӳ����ǵ���ģʽ��
//����ģʽ�Ĺؼ���
//1.Ψһʵ��������ģʽȷ��һ����ֻ��һ��ʵ���������ṩȫ�ַ��ʵ����������ʵ����
//2.˽�й��캯����ͨ������Ĺ��캯����Ϊ˽�У���ֹ�ⲿ�ഴ���µ�ʵ����
//3.��̬�������ṩһ����̬�������ڻ�ȡ��Ψһʵ����
class ConnectionPool
{
public:
    // ��ȡΨһʵ���ľ�̬����
	static ConnectionPool* getConnectionPool();

    // ɾ���������캯���͸�ֵ��������ȷ��ʵ����Ψһ��
	ConnectionPool(const ConnectionPool& obj) = delete;
	ConnectionPool& operator=(const ConnectionPool& obj) = delete;

    //��ȡ����(ʹ��C++11�Ĺ��������ָ��ʵ�����ܻ���)��ʵ�����ݿ����ӵ��Զ�����
    shared_ptr<MysqlConn> getConnection();

    //����
    ~ConnectionPool();
private:
    // ˽�й��캯������ֹ�ⲿʵ����
	ConnectionPool();

    //����json�ļ�(ע��������Ŀ)
    bool parseJsonFile();

    //������
    void producterConnection();//��������
    void recyclerConnection();//��������

    //��������
    void addConnection();

    string m_ip;             // ���ݿ������ip��ַ
    string m_user;           // ���ݿ�������û���
    string m_dbName;         // ���ݿ�����������ݿ���
    string m_passwd;         // ���ݿ����������
    unsigned short m_port;   // ���ݿ�������󶨵Ķ˿�

    int m_minSize;           // ���ӳ�ά������С������
    int m_maxSize;           // ���ӳ�ά�������������
    int m_maxIdleTime;       // ���ӳ������ӵ�������ʱ��
    int m_timeout;           // ���ӳػ�ȡ���ӵĳ�ʱʱ��

	queue<MysqlConn*> m_connectionQ;      // �洢MySQL���ӵĶ���

    //���������,��ð������ߺ������ߵ����ֿ�
    mutex m_mutexQ;                       // ������
    condition_variable m_cond;           // ��������
};

