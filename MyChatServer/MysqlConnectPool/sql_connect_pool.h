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

	MYSQL *GetConnection();		//��ȡ���ݿ�����
	bool ReleaseConnection(MYSQL *conn);	//�ͷ�����
	int GetFreeConn();		//��ȡ����
	void DestroyPool();		//�������ݿ����ӳ�

	//����ģʽ
	static connection_pool *GetInstance();

	void init(string url, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn);

private:
	unsigned int MaxConn;	//���������
	unsigned int CurConn;	//��ǰ��ʹ�õ�������
	unsigned int FreeConn;	//��ǰ���е�������

private:
	locker lock;
	list<MYSQL *> connList;	//���ӳ�
	sem reserve;

private:
	string url;		//������ַ
	string Port;	//���ݿ�˿ں�
	string User;	//��¼���ݿ��û���
	string PassWord;	//����
	string DatabaseName;	//���ݿ���

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

