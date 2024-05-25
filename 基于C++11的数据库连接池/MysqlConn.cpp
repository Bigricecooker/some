#include "MysqlConn.h"
//ע��,��ΪMySQL�ṩ��API����c���Ե�,ʹ���������C++���͵��ַ�����Ҫ��string��ĳ�Ա����c_str()����ת��

MysqlConn::MysqlConn()
{
	//��ʼ�����ӻ���
	m_conn = mysql_init(nullptr);

	//����Ϊ֧�����ĵ�utf8�ַ�����,��ֹ�����ַ���������
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	//�ر����ݿ�����
	if (m_conn != nullptr)
	{
		mysql_close(m_conn);
	}

	//�ͷŽ�������ڴ�
	freeResult();
}

bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//cout <<ip<<"," << user << "," << passwd << "," << dbName << "," << port << endl;
	//����MySQL������
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);

	//ptr��m_connָ��ĵ�ַ��ʵ��һ����
	if (ptr != nullptr)
		return true;
	return false;
}

bool MysqlConn::update(string sql)
{
	//ִ��sql���
	if (mysql_query(m_conn, sql.c_str()))//ִ�гɹ�����0,ʧ�ܷ��ط�0
		return false;
	return true;
}

bool MysqlConn::query(string sql)
{
	//�ͷŽ�����е��ڴ�
	freeResult();

	//ִ��sql��ѯ���
	if (mysql_query(m_conn, sql.c_str()))
		return false;

	//����������浽�ͻ���
	m_result = mysql_store_result(m_conn);
	return true;
}

bool MysqlConn::next()
{
	if (m_result != nullptr)
	{
		//�õ�������е�1����¼
		m_row = mysql_fetch_row(m_result);//����һ������ָ��,ָ��һ��һ��ָ��,��ָ������,ָ�����鱣����������¼���е��ֶε�ֵ
		if (m_row != nullptr)
			return true;
	}
	return false;
}

string MysqlConn::Value(int index)
{
	//��õ�ǰ��¼���ֶε�����
	int FieldCount = mysql_num_fields(m_result);

	//�ж�index�Ƿ�������
	if (index >= FieldCount || index < 0)
	{
		//���ؿ��ַ���
		return string();
	}

	//����ȡ�����ֶ�ֵ
	char* val = m_row[index];
	
	//�õ���Ӧ�ֶ�ֵ�ĳ���
	//��ֹ����ת��Ϊstring���ͳ���
	unsigned long length = mysql_fetch_lengths(m_result)[index];//�ú������ص��������ֶζ�Ӧ��ֵ�ĳ���,������ֻ��Ҫindex���λ���ϵ�

	//ת��Ϊstring����
	return string(val, length);
}

bool MysqlConn::transacton()
{
	//����������Ϊ�ֶ��ύ
	return mysql_autocommit(m_conn,false);
}

bool MysqlConn::commit()
{
	//�ύ����
	return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
	//����ع�
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshAliveTime()
{
	//�����ʼ����ʱ���
	m_alivetime = steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
	//���ʱ���:����
	nanoseconds res = steady_clock::now() - m_alivetime;

	//������ת��Ϊ�;��ȵĺ���
	milliseconds millsec = duration_cast<milliseconds>(res);

	//���غ��������
	return millsec.count();
}

void MysqlConn::freeResult()
{
	//�ͷŽ�������ڴ�
	if (m_result)
	{
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}
