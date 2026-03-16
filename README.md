# HttpServer 项目说明

## 一、项目使用模型

### 1. 多Reactor多线程模型（主从Reactor）

- 一个Reactor线程专门负责监听事件，其他Reactor线程进行IO处理。
- IO Reactor线程将数据分发给线程池进行业务处理。

> 注意：执行流不宜过多，过多会导致线程切换频繁，降低性能。业务处理由IO Reactor线程完成，本HttpServer未添加线程池。

## 二、项目模块划分

### 1. Server模块（Reactor模型TCP服务器）

- **Buffer模块**：套接字数据缓冲区管理，保证数据完整，socket可写时发送数据。
- **Socket模块**：封装套接字相关操作（创建、绑定、监听、连接、发送、接收、释放等）。
- **Channel模块**：文件描述符IO事件管理，触发事件时调用回调处理。
- **Connection模块**：通信连接管理（新建、关闭、超时、数据收发、连接过程函数等）。
- **Acceptor模块**：监听套接字事件，接受新连接，封装Connection对象，设置回调。
- **TimerQueue模块**：定时任务管理，连接超时关闭等。
- **Poller模块**：epoll事件封装，事件注册与分发。
- **EventLoop模块**：事件监控管理，一个模块一个线程，所有连接操作都在EventLoop中完成，保证线程安全。
- **TcpServer模块**：服务器整体管理，对外用户接口，快速搭建服务器，用户可设置回调函数。

### 2. 协议模块

- 为Reactor模型服务器提供应用层协议支持（HTTP协议）。

---

## 三、项目性能测试（webbench）

> 测试命令（忽略带宽，简单测试）：

```bash
./webbench -c 100 -t 30 http://127.0.0.1:8085/
```

> 测试结果示例：

```
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: 127.0.0.1

Running info: 100 clients, running 30 sec.

Speed = 426 pages/min, 256 bytes/sec.
Requests: 213 succeed, 0 failed.
```
