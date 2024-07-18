#ifndef HEAP_TIMER_H
#define HEAP_TIMER_H

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>
#include "../log/log.h"

// 定义一个函数类型的变量TimeoutCallBack    超时回调函数
typedef std::function<void()> TimeoutCallBack;
// 定义一个时钟类型变量Clock
typedef std::chrono::high_resolution_clock Clock;
// 定义一个毫秒类型变量MS
typedef std::chrono::milliseconds MS;
// 定义一个时间戳类型变量TimeStamp
typedef Clock::time_point TimeStamp;

struct TimerNode {
    int id;         //id是由外部系统传入，如文件描述符等
    TimeStamp expires;  // 超时时间点
    TimeoutCallBack cb; // 回调function<void()>
    bool operator<(const TimerNode& t) {    // 重载比较运算符
        return expires < t.expires;
    }
    bool operator>(const TimerNode& t) {    // 重载比较运算符
        return expires > t.expires;
    }
};
class HeapTimer {
public:
    HeapTimer() { heap_.reserve(64); }  // 保留（扩充）容量
    ~HeapTimer() { clear(); }
    
    // 调整指定id的定时器节点的新过期时间
    void adjust(int id, int newExpires);
    // 添加一个定时器，id为定时器id，timeOut为过期时间，cb为回调函数
    void add(int id, int timeOut, const TimeoutCallBack& cb);
    // 执行指定ID的定时器节点的回调函数
    void doWork(int id);
    // 清除所有定时器
    void clear();
    // 处理到期的定时器
    void tick();
    // 弹出堆顶元素（最早到期的定时器）
    void pop();
    // 获取下一个定时器的过期时间
    int GetNextTick();

private:
    void del_(size_t i);    //删除指定位置的定时器节点
    void siftup_(size_t i); //上浮操作，保持堆性质
    bool siftdown_(size_t i, size_t n);//下沉操作，保持堆性质
    void SwapNode_(size_t i, size_t j);//交换两个定时器节点的位置

    std::vector<TimerNode> heap_;       //最小堆，用于存储定时器节点
    std::unordered_map<int, size_t> ref_;   // 映射ID到堆中的下标，便于快速查找和更新
};

#endif //HEAP_TIMER_H
