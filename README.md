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

### 2. Utils模块

- **共头文件模块**: 定义项目中使用的公共头文件，包含常用的系统库和项目内的其他模块头文件，方便统一管理和引用。
- **Log模块**: 日志系统，支持多级别日志输出（DEBUG、INFO、WARN、ERROR），日志文件管理。
- **工具接口**: 读取文件内容,向文件写入内容,URL编码与解码,字符串分割等常用工具函数.获取文件后缀名等。
- **Buffer模块**：套接字数据缓冲区管理，保证数据完整，socket可写时发送数据。以及文件读取数据保存位置

### 3. Protocol模块

- Http模块:
    - HttpRequest模块: HTTP请求解析与管理，支持GET、POST等方法，解析请求行、头部和消息体。
    - HttpResponse模块: 业务处理,HTTP响应构建与管理，设置状态码、响应头和消息体，生成完整HTTP响应数据。
    - HttpContext模块: HTTP请求上下文管理，保存请求和响应对象，处理请求生命周期，提供接口供业务处理使用。
    - HttpServer模块: 上述模块的整合,快速构建HTTP服务器，处理HTTP请求，生成HTTP响应，支持静态文件服务和动态请求处理。
- 其他协议模块（如FTP、SMTP等）可根据需要添加，提供相应的请求解析和响应构建功能。

---

## 三、如何构建与运行

下面给出在 Linux 环境中的示例步骤，用于快速构建并启动示例服务：

```bash
# 在项目根目录执行:
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
# 或者使用rebuild脚本快速构建
./rebuild.sh
```

进入到构建目录后，执行以下命令启动/暂停服务:

```bash
# 启动示例服务 build/目录下
./app/loop.sh start
# 停止示例服务
./app/loop.sh stop
```

用于测试和演示的脚本：`server_api.sh`, `client_api.sh`, 以及 `webbench`(用于并发压测 TcpServer)。
更多使用细节请参考仓库内相应脚本和 `api_test/` 下的API测试代码。
