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

//1.���߳�:ʹ��/��ʹ�����ӳ�
//2.���߳�:ʹ��/��ʹ�����ӳ�

//��ʹ�ó�
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

//ʹ�ó�
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


//���Ե��߳��ٶ�
void test1()
{
#if 0
	//�����ӳ�,���߳���ʱ: 62956360400����,62956����
	steady_clock::time_point begin = steady_clock::now();
	op1(0, 5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "���̳߳�,���߳���ʱ: " << length.count() << "����,"
		<< length.count() / 1000000 << "����" << endl;

#else
	//���ӳ�, ���߳���ʱ: 23028217000����, 23028����
	ConnectionPool* pool = ConnectionPool::getConnectionPool();
	steady_clock::time_point begin = steady_clock::now();
	op2(pool, 0, 5000);
	steady_clock::time_point end = steady_clock::now();
	auto length = end - begin;
	cout << "�̳߳�,���߳���ʱ: " << length.count() << "����,"
		<< length.count() / 1000000 << "����" << endl;


#endif
}

//���Զ��߳��ٶ�
void test2()
{
#if 1
	
	//��ǰ����һ������,�����������߳�ͬʱʹ��һ���û���������������ݿ�ʱ����ʧ�ܵ�����
	MysqlConn conn;
	conn.connect("root", "123456", "textdb", "127.0.0.1");

	//�����ӳ�,�൥�߳���ʱ: 13549012800����,13549����
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
	cout << "���̳߳�,�൥�߳���ʱ: " << length.count() << "����,"
		<< length.count() / 1000000 << "����" << endl;
#else
	//���ӳ�,�൥�߳���ʱ: 6630270900����,6630����
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
	cout << "�̳߳�,�൥�߳���ʱ: " << length.count() << "����,"
		<< length.count() / 1000000 << "����" << endl;
#endif
}

int main()
{
	//query(); 
	//test1();
	test2();
	return 0;
}