#include "sql_connect_pool.h"

connection_pool::connection_pool()
{
	this->CurConn = 0;
	this->FreeConn = 0;
}
connection_pool::~connection_pool()
{
	this->DestroyPool();
}

//����ģʽ
connection_pool *connection_pool::GetInstance()
{
	static connection_pool connPool;
	return &connPool;
}

void connection_pool::init(string url, string User, string PassWord, string DBName, int Port, unsigned int MaxConn)
{
	//��ʼ�����ݿ���Ϣ
	this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DBName;

	lock.lock();
	printf("init action\n");
	int s = 0;
	//����MaxConn�����ݿ�����
	for (int i = 0; i < MaxConn; i++) {
		MYSQL *con = NULL;
		con = mysql_init(con);
		if (con == NULL) {
			cout << "Error:" << mysql_error(con);
			printf("mysql_init error\n");
			exit(1);
		}
		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(), DBName.c_str(), Port, NULL, 0);

		if (con == NULL) {
			cout << "Error:" << mysql_error(con);
			printf("mysql_real_connect error\n");
			exit(1);
		}

		connList.push_back(con);
		++FreeConn;
	}
	//���ź�����ʼ��Ϊ������Ӵ���
	reserve = sem(FreeConn);

	this->MaxConn = FreeConn;

	lock.unlock();
}


//��������ʱ�������ݿ����ӳ��з���һ���������ӣ�����ʹ�úͿ���������
MYSQL *connection_pool::GetConnection()
{
	MYSQL *con = NULL;


	reserve.wait();
	lock.lock();

	con = connList.front();
	connList.pop_front();

	--FreeConn;
	++CurConn;
	lock.unlock();

	return con;
}
bool connection_pool::ReleaseConnection(MYSQL *con)
{
	if (con == NULL) {
		return false;
	}

	lock.lock();
	connList.push_back(con);
	++FreeConn;
	--CurConn;
	lock.unlock();

	reserve.post();
	return true;
}
int connection_pool::GetFreeConn()
{
	return this->FreeConn;
}
void connection_pool::DestroyPool()
{
	lock.lock();
	if (connList.size() > 0) {
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); it++) {
			MYSQL *con = *it;
			mysql_close(con);
		}
		CurConn = 0;
		FreeConn = 0;
		connList.clear();
		lock.unlock();
	}
	lock.unlock();
}



connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)
{
	*SQL = connPool->GetConnection();

	this->conRAII = *SQL;
	this->poolRAII = connPool;
}
connectionRAII::~connectionRAII()
{
	this->poolRAII->ReleaseConnection(this->conRAII);
}



