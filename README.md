# 实时聊天系统—服务器（**[客户端](https://github.com/The-Color-of-Wind/ChatSystem-Client)**）
---

## 项目介绍
基于 **Epoll + Reactor + 线程池** 的高性能通信服务器，支持 **Qt 跨平台客户端**接入，支持 **5000+ 实时通信连接**，适用于**在线聊天、消息推送**等高并发场景。

---

## 项目特点

### 高并发架构

- **Reactor + 线程池**：主线程仅负责监听和连接分发，工作线程异步处理 I/O，有效提升并发处理能力
- **线程池 + 任务队列**：使用自研线程池来管理多个工作线程，并通过任务队列来调度任务。
- **数据库连接池（单例 + RAII）**：单例模式创建并数据库连接池封装，配合 RAII 自动释放资源，避免连接泄漏

### 自定义协议

- **协议格式**：`[4字节长度] + JSON正文`
- **拆包粘包处理**：利用状态机实现信息的拆分，确保消息完整解析，拆分成功率 **100%**

### 异步日志系统

- 基于 **管道通信** + **日志线程** 实现异步日志写入
- 支持**多级日志等级**（INFO, ERROR, DEBUG），避免阻塞主线程 I/O

### 性能压测
- 使用 **JMeter 自定义 TCP Sampler** 进行压力测试
- 已验证可支持 **2000+ 并发连接** 实时通信，系统稳定运行

---

## 目录
1. [已实现功能](#已实现功能)
2. [Demo演示](#Demo演示)
3. [服务器框架](#服务器框架)
4. [类概述](#类概述)
5. [硬件信息](#硬件信息)
6. [项目运行](#项目运行)
7. [更新日志](#更新日志)（QPS优化）
8. [致谢](#致谢)
   
---

## 已实现功能

- ✅ 用户注册与登录
- ✅ 好友添加/删除/列表
- ✅ 实时聊天（点对点）
- ✅ 防粘包拆包的消息协议
- ✅ 异步日志记录
- ✅ 数据库连接池支持
- 持续更新中...

---

## Demo演示
[演示视频](https://www.bilibili.com/video/BV1GrosY3E7k/?vd_source=57d3045b67b7aa01f9f207a33b419c6a)

---

## 服务器框架
![55857738ed533bb911103e999148fb0](https://github.com/user-attachments/assets/4978336a-a5e4-41e3-b974-c24102199cfc)
![7b1cca322ae53001663766a33e3e685](https://github.com/user-attachments/assets/5c54e15a-cf95-42ee-b578-963c651635a2)

---

## 类概述

[读取配置文件](ModuleDescription/IniConfig.md)

[网络通信](ModuleDescription/ChatServer.md)

[业务处理](ModuleDescription/ChatConn.md)

[线程池](ModuleDescription/ThreadPool.md)

[数据库连接池](ModuleDescription/MysqlConnectPool.md)

[锁](ModuleDescription/lock.md)

[id && fd 映射](ModuleDescription/ChatMapping.md)

[日志](ModuleDescription/log.md)

---

## 硬件信息

- 处理器：4
- 内核：4
- 内存：3.0GB
- 文件描述符限制：100000（ulimit -n）

---

## 项目运行

- 环境：
  - 操作系统：Ubuntu 20.04
  - 数据库：MySQL 8.0.41 （数据库表私聊我）
  - 
- 构建
  - sudo apt-get install g++
  - sudo apt-get install make
  - sudo apt-get install libjsoncpp-dev
  - sudo apt-get install libmysqlclient-dev
  - make
  - 修改配置文件（端口默认12345）
  - ./startserv

- 数据库
  - sql目录下有两个数据库文件，分别是 结构 / 结构+数据，可根据需求选择

---

## 更新日志
### **version1**：
| 指标项 | 2000并发 | 5000并发 | 
|-------|-----------|--------------|
| **请求量** | 4000 | 10000 |
| **登录响应**| 3551ms| 10488ms |
| **登录成功率**| 97.2%| 70.3% |
| **通信响应**| 341ms| 1179ms |
| **通信成功率**| 97.2%| 70.3% |

> *注*：
> - *登录都需要一定的时间初始化，时延差强人意，但成功率太差*
> - *发送信息响应时延表现较好，但成功率不足，且暂无重发机制*

### **version2**：增加消息头 + 增加防沾包机制 + 映射互斥锁
| 指标项 | 2000并发 | 5000并发 | 
|-------|-----------|--------------|
| **请求量** | 4000 | 10000 |
| **登录响应**| 3660ms | 10375ms |
| **登录成功率**| 100%| 100% |
| **通信响应**| 1693ms | 9453ms |
| **通信成功率**| 100%| 100% |

> *注*：
> - *由于采用自定义消息格式，故需自定义 TCP Sampler 进行测试*
> - *发送信息成功率达到100%，但由于互斥锁机制，导致吞吐量急速下降*

---

## 致谢
Linux高性能服务器编程，游双著.

感谢前辈的项目引路：https://github.com/qinguoyi/TinyWebServer?tab=readme-ov-file
