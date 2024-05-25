#include "MysqlConn.h"
//注意,因为MySQL提供的API都是c语言的,使用如果传入C++类型的字符串需要用string类的成员函数c_str()进行转换

MysqlConn::MysqlConn()
{
	//初始化连接环境
	m_conn = mysql_init(nullptr);

	//设置为支持中文的utf8字符编码,防止中文字符出现乱码
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	//关闭数据库连接
	if (m_conn != nullptr)
	{
		mysql_close(m_conn);
	}

	//释放结果集的内存
	freeResult();
}

bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	//cout <<ip<<"," << user << "," << passwd << "," << dbName << "," << port << endl;
	//连接MySQL服务器
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);

	//ptr和m_conn指向的地址其实是一样的
	if (ptr != nullptr)
		return true;
	return false;
}

bool MysqlConn::update(string sql)
{
	//执行sql语句
	if (mysql_query(m_conn, sql.c_str()))//执行成功返回0,失败返回非0
		return false;
	return true;
}

bool MysqlConn::query(string sql)
{
	//释放结果集中的内存
	freeResult();

	//执行sql查询语句
	if (mysql_query(m_conn, sql.c_str()))
		return false;

	//将结果集保存到客户端
	m_result = mysql_store_result(m_conn);
	return true;
}

bool MysqlConn::next()
{
	if (m_result != nullptr)
	{
		//得到结果集中的1条记录
		m_row = mysql_fetch_row(m_result);//返回一个二级指针,指向一个一级指针,即指针数组,指针数组保存着这条记录所有的字段的值
		if (m_row != nullptr)
			return true;
	}
	return false;
}

string MysqlConn::Value(int index)
{
	//获得当前记录中字段的数量
	int FieldCount = mysql_num_fields(m_result);

	//判断index是否有问题
	if (index >= FieldCount || index < 0)
	{
		//返回空字符串
		return string();
	}

	//保存取出的字段值
	char* val = m_row[index];
	
	//得到对应字段值的长度
	//防止下面转换为string类型出错
	unsigned long length = mysql_fetch_lengths(m_result)[index];//该函数返回的是所有字段对应的值的长度,但我们只需要index这个位置上的

	//转换为string类型
	return string(val, length);
}

bool MysqlConn::transacton()
{
	//将事务设置为手动提交
	return mysql_autocommit(m_conn,false);
}

bool MysqlConn::commit()
{
	//提交事务
	return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
	//事务回滚
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshAliveTime()
{
	//获得起始存活的时间点
	m_alivetime = steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
	//获得时间段:纳秒
	nanoseconds res = steady_clock::now() - m_alivetime;

	//将纳秒转换为低精度的毫秒
	milliseconds millsec = duration_cast<milliseconds>(res);

	//返回毫秒的数量
	return millsec.count();
}

void MysqlConn::freeResult()
{
	//释放结果集的内存
	if (m_result)
	{
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}
