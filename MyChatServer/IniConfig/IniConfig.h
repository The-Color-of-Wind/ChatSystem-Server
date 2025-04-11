#ifndef INICONFIG_H
#define INICONFIG_H


#include <string>
#include <unordered_map>
#include <iostream>
#include <filesystem>
#include <unistd.h>
#include <limits.h>

using namespace std;


class IniConfig {
public:
	// 获取单例实例
	static IniConfig& getInstance();

	bool load(const string& path);
	string get(const string& section, const string& key, const string& default_value = "") const;
	int getInt(const string& section, const string& key, int default_value = 0) const;

	void iniConfigData();

	bool getListenEt() { return this->listen_et; }
	bool getConnEt() { return this->conn_et; }
	int getThreadNum() { return this->thread_num; }
	int getPort() { return this->port; }

private:
	IniConfig() = default;
	IniConfig(const IniConfig&) = delete;
	IniConfig& operator=(const IniConfig&) = delete;

private:
	unordered_map<string, unordered_map<string, string>> data;

public:
	// 网络配置
	string ip;
	int port;
	int max_conn;

	// 数据库配置
	string db_host;
	string db_user;
	string db_pwd;
	string db_name;
	int db_port;

	// 模式配置
	bool listen_et;
	bool conn_et;

	// 线程池 + 数据库池 + 内存池
	int thread_num;
	int db_conn_num;
	bool mem_pool_enabled;

	// 日志配置
	string log_file;
	string log_level;
};

#endif 
