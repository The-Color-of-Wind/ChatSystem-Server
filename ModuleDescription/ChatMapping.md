# 用户映射表（ChatMapping）

## 描述
为支持多用户在线通信，维护一个 **用户ID与socket fd 的映射表** ，在用户登陆时建立映射关系，在断开连接时移出。

## 设计
- 使用 **单例模式** 管理全局用户映射表
- 利用读写锁机制`shared_lock<shared_mutex> lock()` `unique_lock<shared_mutex> lock()`提供线程安全的增删查
