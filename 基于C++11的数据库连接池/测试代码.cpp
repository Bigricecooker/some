#include "ConnectionPool.h"
#include "MysqlConn.h"
#include <iostream>
using namespace std;

int query()
{
	MysqlConn conn;
	conn.connect("root", "123456", "textdb", "127.0.0.1");
	string sql = "insert into `person` values(6,21,'man','sy')";
	bool flag = conn.update(sql);
	cout << "flag value:" << flag << endl;

	sql = "select * from `person`";
	conn.query(sql);
	while (conn.next())
	{
		cout << conn.Value(0) << ","
			<< conn.Value(1) << ","
			<< conn.Value(2) << ","
			<< conn.Value(3) << endl;
	}
	return 0;
}

//1.单线程:使用/不使用连接池
//2.多线程:使用/不使用连接池

//不使用池
void op1(int begin, int end)
{
	for (int i = begin; i < end; i++)
	{
		MysqlConn conn;
		conn.connect("root", "123456", "textdb", "127.0.0.1");
		char sql[1024] = { 0 };
		sprintf_s(sql, "insert into `person` values(%d,21,'man','sy')", i);
		bool flag = conn.update(sql);
		cout << "flag value:" << flag << endl;
	}
}

//使用池
void op2(ConnectionPool* pool, int begin, int end)
{
	for (int i = begin; i < end; i++)
	{
		shared_ptr<MysqlConn> conn = pool->getConnection();
		char sql[1024] = { 0 };
		sprintf_s(sql, "insert into `person` values(%d,21,'man','sy')", i);
		bool flag = conn->update(sql);
		cout << "flag value:" << flag << endl;
	}
}


//测试单线程速度
void test1()
{
#if 0
	//非连接池,单线程用时: 62956360400纳秒,62956毫秒
	steady_clock::time_point begin = steady_clock::now();
	op1(0, 5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "非线程池,单线程用时: " << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒" << endl;

#else
	//连接池, 单线程用时: 23028217000纳秒, 23028毫秒
	ConnectionPool* pool = ConnectionPool::getConnectionPool();
	steady_clock::time_point begin = steady_clock::now();
	op2(pool, 0, 5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "线程池,单线程用时: " << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒" << endl;


#endif
}

//测试多线程速度
void test2()
{
#if 1
	
	//提前建立一次连接,避免下面多个线程同时使用一个用户民和密码连接数据库时连接失败的问题
	MysqlConn conn;
	conn.connect("root", "123456", "textdb", "127.0.0.1");

	//非连接池,多单线程用时: 13549012800纳秒,13549毫秒
	steady_clock::time_point begin = steady_clock::now();
	thread t1(op1, 0, 1000);
	thread t2(op1, 1000, 2000);
	thread t3(op1, 2000, 3000);
	thread t4(op1, 3000, 4000);
	thread t5(op1, 4000, 5000);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "非线程池,多单线程用时: " << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒" << endl;
#else
	//连接池,多单线程用时: 6630270900纳秒,6630毫秒
	ConnectionPool* pool = ConnectionPool::getConnectionPool();
	steady_clock::time_point begin = steady_clock::now();
	thread t1(op2, pool, 0, 1000);
	thread t2(op2, pool, 1000, 2000);
	thread t3(op2, pool, 2000, 3000);
	thread t4(op2, pool, 3000, 4000);
	thread t5(op2, pool, 4000, 5000);
	t1.join();
	t2.join();
	t3.join();
	t4.join();
	t5.join();
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "线程池,多单线程用时: " << length.count() << "纳秒,"
		<< length.count() / 1000000 << "毫秒" << endl;
#endif
}

int main()
{
	//query(); 
	//test1();
	test2();
	return 0;
}