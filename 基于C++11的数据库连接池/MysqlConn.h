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
	//��ʼ�����ݿ�����
	MysqlConn();

	//�ͷ����ݿ�����
	~MysqlConn();

	//�������ݿ�
	bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);

	//�������ݿ�:insert,update,delete
	bool update(string sql);

	//��ѯ���ݿ�
	bool query(string sql);

	//������ѯ�õ��Ľ����
	bool next();

	//�õ�������еĶ�Ӧ���ֶ�ֵ
	string Value(int index);

	//�������
	bool transacton();

	//�ύ����
	bool commit();

	//����ع�
	bool rollback();


	//ˢ����ʼ�Ŀ���ʱ���
	void refreshAliveTime();

	//�������ӵĴ��ʱ��
	long long getAliveTime();
private:
	//�ͷŽ�����ڴ�
	void freeResult();

	//���ݿ����
	MYSQL* m_conn = nullptr;

	//�����
	MYSQL_RES* m_result = nullptr;

	//������
	//typedef char** MYSQL_ROW;
	MYSQL_ROW m_row = nullptr;

	//C++11��ʱ���ṩ������
	//����ʱ��
	steady_clock::time_point m_alivetime;
};

