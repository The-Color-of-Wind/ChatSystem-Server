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
	// ��ʼ�����ݿ���Ϣ
	this->url = url;
	this->Port = Port;
	this->User = User;
	this->PassWord = PassWord;
	this->DatabaseName = DBName;

	lock.lock();
	printf("init connection_pool\n");

	this->MaxConn = MaxConn;
	this->CurConn = 0;
	this->FreeConn = 0;

	// �ȳ�ʼ���ź���Ϊ 0�������ӳɹ�������� post
	reserve = sem(0);

	for (int i = 0; i < MaxConn; ++i) {
		MYSQL* con = mysql_init(nullptr);
		if (con == nullptr) {
			std::cerr << "mysql_init error\n";
			exit(1);
		}

		con = mysql_real_connect(con, url.c_str(), User.c_str(), PassWord.c_str(),
			DBName.c_str(), Port, nullptr, 0);
		if (con == nullptr) {
			std::cerr << "mysql_real_connect error: " << mysql_error(con) << "\n";
			exit(1);
		}

		connList.push_back(con);
		++FreeConn;

		reserve.post();  // ��ʼ���ɹ��� post �ź���
	}

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
	if (!connList.empty()) {
		for (auto conn : connList) {
			mysql_close(conn);
		}
		connList.clear();
		CurConn = 0;
		FreeConn = 0;
	}
	lock.unlock();
}



connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool)
{
	*SQL = connPool->GetConnection();

	assert(SQL != NULL);
	this->conRAII = *SQL;
	this->poolRAII = connPool;
}
connectionRAII::~connectionRAII()
{
	if (this->conRAII && this->poolRAII) {
		this->poolRAII->ReleaseConnection(this->conRAII);
		this->conRAII = nullptr;
	}
	//this->poolRAII->ReleaseConnection(this->conRAII);
}



