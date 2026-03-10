#include "../include/timer.h"
#include "../include/event_loop.h"

// TimerTask实现
TimerTask::TimerTask(uint64_t id, uint64_t expireTime, Action action, Action release)
    : _id(id), _expireTime(expireTime), _timer_action(action), _release(release), _isActive(true) {}

uint64_t TimerTask::GetId() const { return this->_id; }
uint64_t TimerTask::GetExpireTime() const { return this->_expireTime; }
void TimerTask::Cancel() { this->_isActive = false; }
bool TimerTask::IsActive() const { return this->_isActive; }
TimerTask::~TimerTask()
{
    if (this->_isActive == true)
        this->_timer_action();
    this->_release();
}

// Timer实现
Timer::Timer(EventLoop *eventLoop, size_t wheelSize)
    : _eventLoop(eventLoop), _tick(0), _wheelSize(wheelSize), _timeWheel(wheelSize),
      _timerChannel(eventLoop, Socket(timerfd_create(CLOCK_MONOTONIC, 0)))
{
    int timerfd = _timerChannel.GetSocket().GetSocketFd();
    if (timerfd < 0)
    {
        LOG(ERROR, "Failed to create timerfd");
        exit(EXIT_FAILURE);
    }

    struct itimerspec new_value;
    new_value.it_value.tv_sec = 1; // 初始过期时间为1s
    new_value.it_value.tv_nsec = 0;

    new_value.it_interval.tv_sec = 1; // 间隔时间为1s
    new_value.it_interval.tv_nsec = 0;

    // TFD_TIMER_ABSTIME: 表示定时器的超时时间是绝对时间;
    // 如果未设置此标志,超时时间是相对时间(从当前时间开始计算)
    if (timerfd_settime(timerfd, 0, &new_value, NULL) == -1)
    {
        LOG(ERROR, "Failed to set timerfd");
        exit(EXIT_FAILURE);
    }

    // 定时器就绪事件回调函数
    this->_timerChannel.readAction = [this, timerfd]()
    {
        uint64_t expirations; // 超时次数,每次超时向文件描述符中写入8字节数据
        ssize_t s = read(timerfd, &expirations, sizeof(expirations));
        if (s != sizeof(expirations))
        {
            LOG(ERROR, "Failed to read timerfd");
            exit(EXIT_FAILURE);
        }
        this->Tick();
    };

    // 开启定时器事件监控
    this->_timerChannel.EnableRead();
}

size_t Timer::GetTickPtr() const { return this->_tick; }

// 添加定时器任务,定时器任务的执行需要和EventLoop模块线程中运行
// Timer 对象生命周期大于任务执行时机.Add在EventLoop线程中保证了定时器成员变量的线程安全
void Timer::Add(uint64_t id, uint64_t expireTime, Action action)
{
    this->_eventLoop->RunTask([this, id, expireTime, action]()
    {
        PtrTimerTask timerTask = std::make_shared<TimerTask>(id, expireTime, action, [this, id](){ this->_Remove(id); });
        _taskMap[id] = WeakTimerTask(timerTask);
        this->_timeWheel[(this->_tick + expireTime) % this->_wheelSize].push_back(timerTask); 
    });
}

void Timer::Refresh(uint64_t id, uint64_t newExpireTime)
{
    this->_eventLoop->RunTask([this, id, newExpireTime]()
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            PtrTimerTask timerTask = it->second.lock();
            if (!timerTask)
                return;
            uint64_t expire = newExpireTime;
            if (expire == 0)
            {
                expire = timerTask->GetExpireTime();
            }
            this->_timeWheel[(this->_tick + expire) % this->_wheelSize].push_back(timerTask);
        }
    });
}

void Timer::_Remove(uint64_t id)
{
    this->_eventLoop->RunTask([this, id]()
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            _taskMap.erase(it);
        }
    });
}

void Timer::Tick()
{
    this->_eventLoop->RunTask([this]()
    {
        this->_tick = (this->_tick + 1) % this->_wheelSize;
        this->_timeWheel[this->_tick].clear();
    });
}

void Timer::Cancel(uint64_t id)
{
    this->_eventLoop->RunTask([this, id]()
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            PtrTimerTask timerTask = it->second.lock();
            if (timerTask)
            {
                timerTask->Cancel();
            }
        }
    });
}


// 只能在EventLoop线程中调用,查找定时器,用于测试
bool Timer::Find(uint64_t id)
{
    if(this->_eventLoop->InLoop() == false)
    {
        LOG(WARNING, "Find TimerTask must be called in EventLoop thread");
        return false;
    }

    auto it = _taskMap.find(id);
    if (it != _taskMap.end())
    {
        PtrTimerTask timerTask = it->second.lock();
        if (timerTask)
        {
            return true;
        }
    }
    return false;
}
