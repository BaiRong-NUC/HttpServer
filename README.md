# HttpServer 项目说明

轻量级的 C++ 高性能事件驱动 HttpServer 示例，采用 Reactor 和 epoll 模型，并集成了一个基于 FastAPI 的模型服务示例。

## 一、项目模型

### 1. 多 Reactor 多线程模型

- 一个 Reactor 线程负责监听新连接。
- 其他 Reactor 线程负责 I/O 事件处理。
- 连接的读写、超时和回调处理都在所属 EventLoop 中完成，保证线程安全。

注意：线程数并不是越多越好，线程切换过多会拖累吞吐。本项目当前重点在网络库结构与服务集成，没有额外加入业务线程池。

当前实现已经补上业务线程池，但只用于承载用户的业务回调，不参与 Reactor 线程内必须串行执行的 I/O 状态变更。

## 二、核心模块

### 1. Server 模块

- Socket 模块：封装创建、绑定、监听、发送、接收和关闭等套接字操作。
- Channel 模块：管理文件描述符关心的 I/O 事件，并在事件触发时执行回调。
- Connection 模块：管理连接生命周期、数据收发和超时处理。
- Acceptor 模块：负责接收新连接并交给 TcpServer。
- Timer 模块：负责定时任务和连接超时回收。
- Poller 模块：封装 epoll 的注册、等待与分发。
- EventLoop 模块：驱动一个线程内的事件循环。
- TcpServer 模块：对外提供服务启动和回调配置入口。

### 2. Model Serving 模块

- 基于 FastAPI 和 Uvicorn 提供模型预测接口。
- 构建时会把 app/serving 同步到 build/app/serving。
- 构建时会把 app/artifacts 同步到 build/app/artifacts。
- 默认由 build/app/loop.sh 联动启动和停止。

### 3. Library 分发产物

- 构建时会额外把公共头文件同步到 build/lib/include。
- build/lib/libhttpserver.a 与 build/lib/include 可直接作为手动集成用的静态库 SDK。

## 三、目录概览

- include 和 src：网络库与协议实现。
- app：示例业务、网页资源、模型服务脚本与模型产物。
- build/lib：静态库分发目录，包含 libhttpserver.a 和 include。
- build/app：构建后的运行目录，包含 server、loop.sh、serving、artifacts、wwwroot 和 log。
- api_test：开发阶段的 API 示例与验证代码。
- test：客户端与错误场景测试程序。

## 四、快速开始

### 1. 安装模型服务依赖

推荐安装到 conda 的 web 环境中：

```bash
conda activate web
pip install -r app/serving/requirements.txt
```

如果没有 web 环境，模型服务脚本会继续尝试使用项目根目录下的 .venv；两者都没有时才回退到系统 python3。

### 2. 构建项目

```bash
mkdir -p build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

或直接执行：

```bash
./rebuild.sh
```

### 3. 启动整套服务

构建完成后，推荐直接从 build/app 启动：

```bash
./build/app/loop.sh start
./build/app/loop.sh status
./build/app/loop.sh stop
```

说明：

- start 会同时启动 C++ HttpServer 和 Python 模型服务。
- status 会同时显示两个服务的状态，并打印模型服务当前选择的 Python 来源。
- stop 会同时停止两个服务，不需要再手动执行额外的 kill 命令。
- 运行期日志与状态文件默认写入 build/app/log。

### 4. 仅启动模型服务

```bash
./build/app/serving/run_model.sh start
./build/app/serving/run_model.sh status
./build/app/serving/run_model.sh stop
```

### 5. 简单分发方式

静态库给其他项目手动集成：

- 直接提供 build/lib/libhttpserver.a。
- 直接提供 build/lib/include。

应用给其他 Linux 机器直接运行：

- 直接拷贝整个 build/app 目录。
- 目标机器自行安装 Python 模型依赖后，运行 ./loop.sh start 即可。
- 运行日志和 PID 状态会统一写入 build/app/log。

## 五、测试与压测

### 1. 压测 HttpServer

```bash
./webbench -c 100 -t 30 http://127.0.0.1:8085/
```

本地压测步骤：

```bash
# 1. 启动服务
./build/app/loop.sh start

# 2. 对首页执行 30 秒压测
./webbench -c 100 -t 30 http://127.0.0.1:8085/

# 3. 测试结束后停止服务
./build/app/loop.sh stop
```

本次压测环境：

- 操作系统：Linux 6.17.0-20-generic x86_64 GNU/Linux
- CPU：Intel(R) Core(TM) Ultra 9 285H
- 逻辑 CPU 数：16
- 内存：30 GiB
- 构建方式：Debug
- 压测工具：仓库内置 webbench 1.5
- 压测目标：本机回环地址 http://127.0.0.1:8085/

本次本地实测结果：

```text
Webbench - Simple Web Benchmark 1.5
Runing info: 100 clients, running 30 sec.

Speed=1328 pages/min, 395312 bytes/sec.
Requests: 664 susceed, 0 failed.
```

说明：

- 这组数据是在本机回环网络下得到的，主要反映当前开发机构建下的本地处理能力。

远端公网地址压测命令：

```bash
./webbench -c 100 -t 30 http://38.190.254.70:8085/
```

本次远端实测结果：

```text
Webbench - Simple Web Benchmark 1.5
Runing info: 100 clients, running 30 sec.

Speed=3826 pages/min, 1143870 bytes/sec.
Requests: 1913 susceed, 0 failed.
```

说明：

- 这组数据是从当前机器对公网地址 http://38.190.254.70:8085/ 发起的远端压测结果。
- 本次测试同样使用 100 并发、持续 30 秒，结果包含公网链路和远端服务器环境带来的综合影响。

### 2. 测试模型接口

```bash
curl -X POST "http://127.0.0.1:8000/predict" \
	-H "Content-Type: application/json" \
	-d '{"features":[6,148,72,35,0,33.6,0.627,50]}'
```

模型接口当前要求传入 8 个基础特征，顺序为：Pregnancies、Glucose、BloodPressure、SkinThickness、Insulin、BMI、DiabetesPedigreeFunction、Age。

## 六、常用产物与脚本

- [build/app/loop.sh](build/app/loop.sh)：整套服务的统一启动、停止、状态脚本。
- [build/app/server](build/app/server)：C++ HttpServer 可执行文件。
- [build/app/log](build/app/log)：运行日志与共享 PID 状态目录。
- [build/app/serving/run_model.sh](build/app/serving/run_model.sh)：模型服务独立启动脚本。
- [build/app/wwwroot](build/app/wwwroot)：同步后的前端静态资源。
- [build/app/artifacts](build/app/artifacts)：同步后的模型与预处理器文件。
- [build/lib/libhttpserver.a](build/lib/libhttpserver.a)：静态库产物。
- [build/lib/include](build/lib/include)：静态库对外头文件目录。

## 七、在线演示

示例页面：

http://38.190.254.70:8085/http_server.html

## 八、扩展方向

- 在 protocol 下添加新的协议模块。
- 在 app/src 中扩展业务逻辑。
- 在 app/serving 中扩展模型预处理和预测接口。

## 九、近期更新

### 1. 修正时间轮推进与代理超时配置

- timerfd 每次 read 返回的是 8 字节的累计超时次数，时间轮现在按累计次数推进 Tick，不再把“读到 8 字节”误当成“发生 8 次超时”。
- 修正后，30 秒非活跃连接超时会按真实秒数触发，不会再在约 4 秒内被提前回收。
- 图像恢复代理的连接超时已经同步放宽到 600 秒，与 curl 的 10 分钟超时配置保持一致，避免长耗时请求被服务器侧提前中断。

### 2. HTTP 路由正则改为注册期预编译

- 路由注册仍然在 [include/protocol/http/http_server.h](include/protocol/http/http_server.h) 和 [src/protocol/http/http_server.cpp](src/protocol/http/http_server.cpp) 中完成。
- Get、Post、Put、Delete 在注册路由时就构造 std::regex，请求到来时只做 std::regex_match，不再为每次请求重复编译正则。
- 这样可以把正则构造成本前移到启动阶段，降低高并发场景下的请求路径匹配开销。

### 3. 业务线程池上提到 TcpServer 层统一管理

- 业务线程池已经从协议层移动到 [include/server/tcp_server.h](include/server/tcp_server.h) 与 [src/server/tcp_server.cpp](src/server/tcp_server.cpp)。
- TcpServer 在构造时创建业务线程池，并在建立连接时把线程池指针传给每个 Connection。
- 以后扩展自定义协议时，不需要在协议层重复实现线程池；只要像现在一样设置 message_callback，耗时业务就会被 Connection 统一投递到业务线程池。

### 4. 保持 Reactor 单线程 I/O 模型，只把业务回调下沉到线程池

- 没有把 [src/server/event_loop.cpp](src/server/event_loop.cpp) 中的 \_RunAllTasks() 整体改成线程池执行。
- 原因是事件循环里的任务并不全是业务逻辑，很多任务属于 Send、Close、EnableWrite、RemoveChannel 这类必须在连接所属 Reactor 线程执行的 I/O 状态操作。
- 如果直接把整个任务队列丢到线程池，会破坏连接的单线程模型，引入额外锁和竞态风险。
- 当前做法是在 [src/server/connection.cpp](src/server/connection.cpp) 中只把 message_callback 投递到业务线程池；投递前先暂停该连接读事件，业务完成后再回到原 EventLoop 恢复读事件，避免同一连接的 Buffer 和 Context 被 Reactor 与业务线程同时访问。
- 任务分配按连接 id 固定落到同一个 worker，每个 worker 维护独立队列，因此没有一个全局任务队列锁，锁竞争相对较低。

### 5. 线程池改成每个 worker 一把锁的桶式并发模型

- 线程池中的任务队列现在是跨线程共享的：Reactor 线程会在 Submit() 时向 worker 的 tasks 队列 push_back，worker 线程会从同一队列 pop_front，析构停止时还会修改 stopping 状态。
- 这些操作如果不加锁，std::deque 会发生数据竞争，结果是未定义行为；轻则任务丢失，重则直接崩溃。
- 当前实现不是使用一个全局大锁，而是每个 worker 自带一把互斥锁，既保证 tasks 队列和 stopping 状态的并发安全，也避免所有 worker 共用一把锁带来的集中竞争。
