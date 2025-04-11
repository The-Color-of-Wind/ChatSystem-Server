# 实时聊天系统—服务器（**[客户端](https://github.com/The-Color-of-Wind/ChatSystem-Client)**）
## 项目介绍（服务器）：
基于 Linux 的高性能聊天服务器与 Qt 客户端的跨网段通信系统，采用 TCP 作为通信协议，支持**用户注册、登录、好友管理 和 实时消息收发**。服务端基于 Linux 开发，使用 **epoll + 线程池** 进行高并发连接管理，客户端采用 Qt 实现跨平台 GUI。（本篇只包含客户端）

## 客户端
- 利用 **多线程机制**将 TCP 连接和数据通信逻辑 从 主 UI 线程 中分离，以确保界面响应流畅，避免因阻塞操作导致的界面卡顿
- 采用 **单例模式** 管理 TCP 连接和数据库连接，确保系统中只有一个实例进行连接
- 利用 **Qt 信号与槽机制** 实现模块间的解耦，确保界面与逻辑的独立性
- 利用 **Qt 布局管理器和 UI**，设计符合用户习惯的界面，确保交互流畅，提升用户体验

## 关于本项目
- 本客户端是自学 Qt 过程中的实践产物，主要用于与服务器交互，验证服务器端的开发效果。因此，界面美化较差，且可能存在部分 bug。
- 分享本项目的目的是希望能提供一个较为完整的项目框架和实现思路，供自学 Qt 的同学参考。（请大家不要嫌弃）
- 因为这是我的第一个 Qt 项目，代码框架可能较为凌乱。如果后续有时间和精力，我将规范代码，并持续维护。


### **本项目支持跨网段传输，可以对其进行打包生成安装包，届时将服务器部署到虚拟机上，就可以跟小伙伴们在同一局域网下聊天哦！**（如果部署到云服务器上，就可以随意聊天了！）


## 目录
1. [Demo演示](#Demo演示)
2. [框架](#框架)
3. [界面概述](#界面概述)
4. [类概述](#类概述)
5. [更新日志](#更新日志)
6. [安装包](#安装包)


## Demo演示
[演示视频](https://www.bilibili.com/video/BV1GrosY3E7k/?vd_source=57d3045b67b7aa01f9f207a33b419c6a)

## 框架
![17cbcaa469b8cabf301f1f46c5eec79](https://github.com/user-attachments/assets/79878936-25bb-45c1-9fb8-c0420581f572)
![67482f768071677e8eb44c841035083](https://github.com/user-attachments/assets/f8f68a2b-4542-4edd-9f86-b1178da9703d)


## 界面概述
[启动界面](ClassDescription/action.md)

[登录界面](ClassDescription/loginwidget.md)

[注册界面](ClassDescription/registerwidget.md)

[主界面](ClassDescription/mainwidget.md)

[聊天界面](ClassDescription/chatwidget.md)

[好友界面](ClassDescription/friendwidget.md)


## 类概述
[用户类](ClassDescription/user.md)

[好友类](ClassDescription/userfriend.md)

[聊天框类](ClassDescription/userchat.md)

[信息类](ClassDescription/message.md)


## 更新日志
- 优化界面


## 安装包
**暂未上线**
