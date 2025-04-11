#include "IniConfig.h"
#include <fstream>
#include <sstream>
#include <algorithm>


// 静态方法定义
IniConfig& IniConfig::getInstance() {
    static IniConfig instance;
    return instance;
}

static string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end = s.find_last_not_of(" \t\r\n");
    return (start == string::npos) ? "" : s.substr(start, end - start + 1);
}

bool IniConfig::load(const string& path) {
    ifstream infile(path);
    if (!infile.is_open()) {
        cerr << "open file error！" << endl;
        return false;

    }

    string line, current_section;
    while (getline(infile, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (line.front() == '[' && line.back() == ']') {
            current_section = trim(line.substr(1, line.size() - 2));
        }
        else {
            size_t pos = line.find('=');
            if (pos == string::npos) continue;
            string key = trim(line.substr(0, pos));
            string value = trim(line.substr(pos + 1));
            data[current_section][key] = value;
        }
    }

    return true;
}

string IniConfig::get(const string& section, const string& key, const string& default_value) const {
    auto it = data.find(section);
    if (it != data.end()) {
        auto kit = it->second.find(key);
        if (kit != it->second.end()) {
            return kit->second;
        }
    }
    return default_value;
}

int IniConfig::getInt(const string& section, const string& key, int default_value) const {
    string val = get(section, key);
    try {
        return stoi(val);
    }
    catch (...) {
        //cerr << "section no exist !" << endl;
        return default_value;
    }
}

void IniConfig::iniConfigData()
{
    char cwd[PATH_MAX];  // PATH_MAX 是系统定义的最大路径长度
    if (!getcwd(cwd, sizeof(cwd))) {
        perror("getcwd() 错误");
    }
    string configPath = string(cwd) + "/config.ini";
    if (!this->load(configPath)) {
        cerr << "load Config file error！" << endl;
        return;
    }
    
    // 网络配置
    this->ip = this->get("server", "ip", "0.0.0.0");
    this->port = this->getInt("server", "port", 8080);
    this->max_conn = this->getInt("server", "max_conn", 10000);

    // 数据库配置
    this->db_host = this->get("database", "host", "127.0.0.1");
    this->db_user = this->get("database", "user", "root");
    this->db_pwd = this->get("database", "password", "");
    this->db_name = this->get("database", "db_name", "chat_system");
    this->db_port = this->getInt("database", "port", 3306);

    // 模式配置
    this->listen_et = this->getInt("mode", "listen_mode", 0) == 1;
    this->conn_et = this->getInt("mode", "conn_mode", 0) == 1;

    // 线程池 + 数据库池 + 内存池
    this->thread_num = this->getInt("pool", "thread_num", 8);
    this->db_conn_num = this->getInt("pool", "db_conn_num", 10);
    this->mem_pool_enabled = this->getInt("pool", "mem_pool", 0) == 1;

    // 日志配置
    this->log_file = this->get("log", "log_file", "./log/server.log");
    this->log_level = this->get("log", "log_level", "info");



}