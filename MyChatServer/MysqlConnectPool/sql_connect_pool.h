#ifndef SQL_CONNECTION_POOL_H
#define SQL_CONNECTION_POOL_H

#include <stdio.h>
#include <list>
#include <mysql/mysql.h>
#include <error.h>
#include <string.h>
#include <iostream>
#include <string>
#include <assert.h>
#include "../lock/locker.h"

using namespace std;


class connection_pool
{
public:
	connection_pool();
	~connection_pool();

	MYSQL *GetConnection();		//获取数据库连接
	bool ReleaseConnection(MYSQL *conn);	//释放连接
	int GetFreeConn();		//获取连接
	void DestroyPool();		//销毁数据库连接池

	//单例模式
	static connection_pool *GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn);

private:
	unsigned int MaxConn;	//最大连接数
	unsigned int CurConn;	//当前已使用的连接数
	unsigned int FreeConn;	//当前空闲的连接数

private:
	locker lock;
	list<MYSQL *> connList;	//连接池
	sem reserve;

private:
	string url;		//主机地址
	string Port;	//数据库端口号
	string User;	//登录数据库用户名
	string PassWord;	//密码
	string DatabaseName;	//数据库名

};


class connectionRAII {
public:
	connectionRAII(MYSQL **con, connection_pool *connPool);
	~connectionRAII();
private:
	MYSQL *conRAII;
	connection_pool *poolRAII;
};

#endif

