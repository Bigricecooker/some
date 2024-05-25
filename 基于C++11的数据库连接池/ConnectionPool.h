#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <thread>
#include <json/json.h>
#include "MysqlConn.h"
using namespace std;

//因为只需要一个数据库池,所有这个连接池类是单例模式类
//单例模式的关键点
//1.唯一实例：单例模式确保一个类只有一个实例，并且提供全局访问点来访问这个实例。
//2.私有构造函数：通过将类的构造函数设为私有，防止外部类创建新的实例。
//3.静态方法：提供一个静态方法用于获取该唯一实例。
class ConnectionPool
{
public:
    // 获取唯一实例的静态方法
	static ConnectionPool* getConnectionPool();

    // 删除拷贝构造函数和赋值操作符，确保实例的唯一性
	ConnectionPool(const ConnectionPool& obj) = delete;
	ConnectionPool& operator=(const ConnectionPool& obj) = delete;

    //获取连接(使用C++11的共享的智能指针实现智能回收)可实现数据库连接的自动回收
    shared_ptr<MysqlConn> getConnection();

    //析构
    ~ConnectionPool();
private:
    // 私有构造函数，防止外部实例化
	ConnectionPool();

    //解析json文件(注意配置项目)
    bool parseJsonFile();

    //任务函数
    void producterConnection();//生产连接
    void recyclerConnection();//销毁连接

    //增加连接
    void addConnection();

    string m_ip;             // 数据库服务器ip地址
    string m_user;           // 数据库服务器用户名
    string m_dbName;         // 数据库服务器的数据库名
    string m_passwd;         // 数据库服务器密码
    unsigned short m_port;   // 数据库服务器绑定的端口

    int m_minSize;           // 连接池维护的最小连接数
    int m_maxSize;           // 连接池维护的最大连接数
    int m_maxIdleTime;       // 连接池中连接的最大空闲时长
    int m_timeout;           // 连接池获取连接的超时时长

	queue<MysqlConn*> m_connectionQ;      // 存储MySQL连接的队列

    //如果程序复杂,最好把消费者和生产者的锁分开
    mutex m_mutexQ;                       // 互斥锁
    condition_variable m_cond;           // 条件变量
};

