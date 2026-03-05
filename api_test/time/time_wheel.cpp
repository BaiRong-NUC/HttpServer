// 基于时间轮的定时器实现
#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <unistd.h>
using Action = std::function<void()>; // 定时器回调函数类型,无参无返回值
class TimerTask
{
private:
    uint64_t _id;         // 定时器ID
    uint64_t _expireTime; // 定时器超时时间
    Action _timer_action; // 定时器回调函数
    Action _release;      // 定时器释放函数,用于定时器到期后从时间轮中移除定时器对象
    bool _isActive;       // 定时器是否被取消

public:
    TimerTask(uint64_t id, uint64_t expireTime, Action action, Action release)
        : _id(id), _expireTime(expireTime), _timer_action(action), _release(release), _isActive(true) {}

    uint64_t GetId() const { return this->_id; }
    uint64_t GetExpireTime() const { return this->_expireTime; }
    void Cancel() { this->_isActive = false; }        // 取消定时器,设置定时器状态为已取消
    bool IsActive() const { return this->_isActive; } // 获取定时器状态
    ~TimerTask()
    {
        // 当智能指针引用计数为0时,定时器对象被销毁,执行定时器回调函数和释放函数
        // 刷新时间只需要再通过智能指针构建一个对象即可.
        if (this->_isActive == true)
            this->_timer_action();
        this->_release();
    } // 定时器到期时执行回调函数
};

class Timer
{
private:
    using PtrTimerTask = std::shared_ptr<TimerTask>;
    using WeakTimerTask = std::weak_ptr<TimerTask>;       // 不会增加引用计数的定时器对象指针,用于更新定时器到期时间
    std::vector<std::vector<PtrTimerTask>> _timeWheel;    // 时间轮,每个槽位存储一个定时器列表
    size_t _tick;                                         // 到期指针,指向那块释放那块
    size_t _wheelSize;                                    // 时间轮大小(最大延时时间)
    std::unordered_map<uint64_t, WeakTimerTask> _taskMap; // 定时器ID到定时器对象的映射

    // 删除定时器
    void _Remove(uint64_t id)
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            _taskMap.erase(it);
        }
    }

public:
    Timer(size_t wheelSize = 60) : _tick(0), _wheelSize(wheelSize), _timeWheel(wheelSize) {}

    size_t GetTickPtr() const { return this->_tick; }

    // 添加定时器
    void Add(uint64_t id, uint64_t expireTime, Action action)
    {
        PtrTimerTask timerTask = std::make_shared<TimerTask>(id, expireTime, action, [this, id]()
                                                             { this->_Remove(id); });
        _taskMap[id] = WeakTimerTask(timerTask);                                              // 将定时器对象存储在映射中
        this->_timeWheel[(this->_tick + expireTime) % this->_wheelSize].push_back(timerTask); // 将定时器对象添加到时间轮的对应槽位中
    }

    // 刷新定时器,更新定时器到期时间
    void Refresh(uint64_t id, uint64_t newExpireTime = 0)
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            // 通过weak_ptr获取定时器对象,实例化shared_ptr
            PtrTimerTask timerTask = it->second.lock();

            // 如果newExpireTime为0,则将定时器到期时间设置为当前时间加上定时器原来的超时时间
            if (newExpireTime == 0)
            {
                newExpireTime = timerTask->GetExpireTime(); // 这里假设定时器的超时时间为时间轮大小
            }
            // 将定时器对象添加到时间轮的对应槽位中
            this->_timeWheel[(this->_tick + newExpireTime) % this->_wheelSize].push_back(timerTask);
        }
    }

    // 时间轮转动,每秒执行一次
    void Tick()
    {
        this->_tick = (this->_tick + 1) % this->_wheelSize; // 时间轮转动,指向下一个槽位
        this->_timeWheel[this->_tick].clear();              // 清空当前槽位的定时器列表,定时器到期后会自动调用析构函数执行回调函数和释放函数
    }

    // 取消定时器
    void Cancel(uint64_t id)
    {
        auto it = _taskMap.find(id);
        if (it != _taskMap.end())
        {
            PtrTimerTask timerTask = it->second.lock();
            if (timerTask)
            {
                timerTask->Cancel(); // 取消定时器,设置定时器状态为已取消
            }
        }
    }
};

int main(int argc, char const *argv[])
{
    Timer timer(10); // 创建一个时间轮,最大计时时间为10秒
    timer.Add(1, 5, []()
              { std::cout << "Timer 1 expired!" << std::endl; }); // 添加一个定时器,ID为1,超时时间为3秒,回调函数为输出定时器到期信息
    timer.Add(2, 5, []()
              { std::cout << "Timer 2 expired!" << std::endl; });
    timer.Cancel(2); // 取消定时器2
    for (int i = 0; i < 5; i++)
    {
        sleep(1);
        timer.Refresh(1);
        std::cout << "Timer 1 refreshed!" << std::endl;
        timer.Tick();
        std::cout << "Tick: " << timer.GetTickPtr() << std::endl;
    }
    // 模拟时间轮转动,每秒执行一次
    while (true)
    {
        timer.Tick();
        std::cout << "Tick: " << timer.GetTickPtr() << std::endl;
        sleep(1);
    }
    return 0;
}
