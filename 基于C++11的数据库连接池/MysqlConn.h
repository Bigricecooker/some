#pragma once
#include <iostream>
#include <string.h>
#include <mysql.h>
#include <chrono>
using namespace std;
using namespace chrono;

class MysqlConn
{
public:
	//初始化数据库连接
	MysqlConn();

	//释放数据库连接
	~MysqlConn();

	//连接数据库
	bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);

	//更新数据库:insert,update,delete
	bool update(string sql);

	//查询数据库
	bool query(string sql);

	//遍历查询得到的结果集
	bool next();

	//得到结果集中的对应的字段值
	string Value(int index);

	//事务操作
	bool transacton();

	//提交事务
	bool commit();

	//事务回滚
	bool rollback();


	//刷新起始的空闲时间点
	void refreshAliveTime();

	//计算连接的存活时长
	long long getAliveTime();
private:
	//释放结果集内存
	void freeResult();

	//数据库对象
	MYSQL* m_conn = nullptr;

	//结果集
	MYSQL_RES* m_result = nullptr;

	//行数据
	//typedef char** MYSQL_ROW;
	MYSQL_ROW m_row = nullptr;

	//C++11中时钟提供的类型
	//绝对时钟
	steady_clock::time_point m_alivetime;
};

