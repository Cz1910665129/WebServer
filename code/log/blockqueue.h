# ifndef BLOCKQUEUE_H
# define BLOCKQUEUE_H

#include <deque>
#include <condition_variable>
#include <mutex>
#include <sys/time.h>
using namespace std;

template<typename T>
class BlockQueue {
public:
    explicit BlockQueue(size_t maxsize = 1000); // 构造函数，初始化队列最大容量
    ~BlockQueue(); // 析构函数
    bool empty(); // 判断队列是否为空
    bool full(); // 判断队列是否已满
    void push_back(const T& item); // 将任务放入队列尾部
    void push_front(const T& item); // 将任务放入队列头部
    bool pop(T& item);  // 弹出的任务放入item
    bool pop(T& item, int timeout);  // 等待时间
    void clear(); // 清空队列
    T front(); // 获取队列头部的任务
    T back(); // 获取队列尾部的任务
    size_t capacity(); // 获取队列容量
    size_t size(); // 获取队列大小

    void flush(); // 刷新队列
    void Close(); // 关闭队列

private:
    deque<T> deq_;                      // 底层数据结构
    mutex mtx_;                         // 锁
    bool isClose_;                      // 关闭标志
    size_t capacity_;                   // 容量
    condition_variable condConsumer_;   // 消费者条件变量
    condition_variable condProducer_;   // 生产者条件变量
};

template<typename T>
BlockQueue<T>::BlockQueue(size_t maxsize) : capacity_(maxsize) {
    // 断言maxsize大于0
    assert(maxsize > 0);
    // 初始化isClose_为false
    isClose_ = false;
}

template<typename T>
BlockQueue<T>::~BlockQueue() {
    Close();
}

template<typename T>
void BlockQueue<T>::Close() {
    // lock_guard<mutex> locker(mtx_); // 操控队列之前，都需要上锁
    // deq_.clear();                   // 清空队列
    clear();
    isClose_ = true;
    // 唤醒所有等待的消费者线程
    condConsumer_.notify_all();
    // 唤醒所有等待的生产者线程
    condProducer_.notify_all();
}

template<typename T>
void BlockQueue<T>::clear() {
    lock_guard<mutex> locker(mtx_);     //lock_guard是std::mutex的智能指针，它可以自动加锁和解锁
    deq_.clear();                       //确保在清空队列的过程中，不会有其他线程访问队列
}

// 模板类BlockQueue的empty()函数，用于判断队列是否为空
template<typename T>
bool BlockQueue<T>::empty() {
    // 上锁，防止多线程同时访问
    lock_guard<mutex> locker(mtx_);
    // 返回队列是否为空
    return deq_.empty();
}

template<typename T>
bool BlockQueue<T>::full() {
    // 上锁
    lock_guard<mutex> locker(mtx_);
    // 判断队列大小是否达到容量
    return deq_.size() >= capacity_;
}

template<typename T>
void BlockQueue<T>::push_back(const T& item) {
    // 注意，条件变量需要搭配unique_lock
    unique_lock<mutex> locker(mtx_);    
    while(deq_.size() >= capacity_) {   // 队列满了，需要等待
        condProducer_.wait(locker);     // 暂停生产，等待消费者唤醒生产条件变量
    }
    deq_.push_back(item);
    condConsumer_.notify_one();         // 唤醒消费者
}

template<typename T>
void BlockQueue<T>::push_front(const T& item) {
    unique_lock<mutex> locker(mtx_);
    while(deq_.size() >= capacity_) {   // 队列满了，需要等待
        condProducer_.wait(locker);     // 暂停生产，等待消费者唤醒生产条件变量
    }
    deq_.push_front(item);
    condConsumer_.notify_one();         // 唤醒消费者
}

template<typename T>
bool BlockQueue<T>::pop(T& item) {
    unique_lock<mutex> locker(mtx_);
    while(deq_.empty()) {
        condConsumer_.wait(locker);     // 队列空了，需要等待
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();         // 唤醒生产者
    return true;
}

template<typename T>
bool BlockQueue<T>::pop(T &item, int timeout) {
    unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty()){
        if(condConsumer_.wait_for(locker, std::chrono::seconds(timeout))            
                == std::cv_status::timeout){        //等待超时时，弹出失败，直接返回
            return false;
        }
        if(isClose_){
            return false;           //队列已经被关闭，直接返回
        }
    }
    item = deq_.front();
    deq_.pop_front();
    condProducer_.notify_one();       
    return true;
}

template<typename T>
T BlockQueue<T>::front() {                  //返回队列首部
    lock_guard<std::mutex> locker(mtx_);
    return deq_.front();
}

template<typename T>
T BlockQueue<T>::back() {                   //返回队列尾部
    lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<typename T>
size_t BlockQueue<T>::capacity() {              // 返回队列容量
    lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<typename T>                        
size_t BlockQueue<T>::size() {                  //返回队列大小
    lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

// 唤醒消费者
template<typename T>                    
void BlockQueue<T>::flush() {
    //唤醒一个等待的消费者线程
    condConsumer_.notify_one();
}
# endif