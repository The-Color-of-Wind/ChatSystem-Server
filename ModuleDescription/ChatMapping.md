# 用户映射表（ChatMapping）

## 描述
为支持多用户在线通信功能，ChatMapping 模块维护一个全局的**用户ID与连接信息** （Socket/Connection）之间的映射关系表。便于服务器根据用户ID快速定位其对应连接，实现在任意时刻向特定用户推送消息。

## 设计
- 采用 **单例模式** 保证全局唯一的用户映射表，所有模块共享；

- 内部通过 **unordered_map<std::string, pair<int, chat_conn*>>** 存储用户ID与连接对象映射；

- 通过 **shared_mutex** 搭配 **shared_lock 和 unique_lock** 实现高性能读写分离的线程安全控制：
