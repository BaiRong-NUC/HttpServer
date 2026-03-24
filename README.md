# HttpServer 项目说明

## 一、项目使用模型

### 1. 多Reactor多线程模型（主从Reactor）

- 一个Reactor线程专门负责监听事件，其他Reactor线程进行IO处理。
- IO Reactor线程将数据分发给线程池进行业务处理。

> 注意：执行流不宜过多，过多会导致线程切换频繁，降低性能。业务处理由IO Reactor线程完成，本HttpServer未添加线程池。

## 二、项目模块划分

### 1. Server模块（Reactor模型TCP服务器）

- **Socket模块**：封装套接字相关操作（创建、绑定、监听、连接、发送、接收、释放等）。
- **Channel模块**：文件描述符IO事件管理，触发事件时调用回调处理。
- **Connection模块**：通信连接管理（新建、关闭、超时、数据收发、连接过程函数等）。
- **Acceptor模块**：监听套接字事件，接受新连接，封装Connection对象，设置回调。
- **TimerQueue模块**：定时任务管理，连接超时关闭等。
- **Poller模块**：epoll事件封装，事件注册与分发。
- **EventLoop模块**：事件监控管理，一个模块一个线程，所有连接操作都在EventLoop中完成，保证线程安全。
- **TcpServer模块**：服务器整体管理，对外用户接口，快速搭建服务器，用户可设置回调函数。

#### 1.1 Server模块性能测试(webbench)

> 测试命令（忽略带宽，简单测试）：

```bash
./webbench -c 100 -t 30 http://127.0.0.1:8085/
```

> 测试结果示例：

````
Webbench - Simple Web Benchmark 1.5
Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.

Request:
GET / HTTP/1.0
User-Agent: WebBench 1.5
Host: 127.0.0.1

Running info: 100 clients, running 30 sec.

Speed = 426 pages/min, 256 bytes/sec.
# HttpServer

轻量级的 C++ 高性能事件驱动 HttpServer 示例，采用 Reactor/epoll 模型，便于学习网络库设计与快速构建基础 HTTP 服务。

## 主要特点

- 基于 Reactor 的事件调度（epoll）
- 清晰的模块划分：Socket / Channel / Connection / EventLoop / Poller / TcpServer
- 支持静态文件服务与简单动态路由示例
- 可复用的工具与协议模块（位于 `include/` 与 `protocol/`）

## 设计简介

- 主从 Reactor 思路：一个主 Reactor 负责监听与接收，新连接分配到 IO Reactor（或同一线程）进行读写处理。
- `EventLoop` 保证每个连接的操作在所属线程/循环中执行，简化并发控制和线程安全。

## 模块概览

- `src/server`：事件循环与服务器实现核心。
- `app`：示例应用入口与脚本（`loop.sh` 用于启动/停止示例服务）。
- `api_test`：用于快速验证各模块行为的测试用例与示例。
- `include/protocol`：HTTP 等协议相关头文件和解析实现。

## 快速开始（Linux）

1. 在项目根目录构建：

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
````

或使用仓库提供的脚本快速构建：

```bash
./rebuild.sh
```

2. 启动/停止示例服务：

```bash
# 启动（在项目根或任意位置执行）
./app/loop.sh start

# 停止
./app/loop.sh stop

注意: kill 命令可能会导致服务器自动重启;
需要使用 `pkill -f server_supervisor.sh` 停止保活进程。
```

3. 直接运行可执行文件（构建后）：

```bash
./build/app/server
```

## 测试与压测

- 使用提供的脚本进行 API 测试：`server_api.sh`、`client_api.sh`。
- 使用 `webbench` 做简单并发压测：

```bash
./webbench -c 100 -t 30 http://127.0.0.1:8085/
```

## 常用脚本与位置

- 启动脚本：`app/loop.sh`
- 演示可执行：`build/app/server`（构建产物）
- API 测试：`api_test/` 目录下的示例
- test 目录：编写客户端,主要为了验证服务器的各种功能是否正常
- build/app/server_supervisor.sh: 服务器保活脚本,当服务器异常退出时会自动重启服务器

## 在线访问app演示

访问示例构建的app服务(公网示例):

http://38.190.254.70:8085/http_server.html

## 扩展

在 `protocol/` 下添加新的协议模块，或在 `app/src` 中扩展业务逻辑
