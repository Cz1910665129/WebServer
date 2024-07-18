#include "epoller.h"

// 创建一个Epoller类，用于处理epoll事件
Epoller::Epoller(int maxEvent):epollFd_(epoll_create(512)), events_(maxEvent){
    // 创建epoll文件描述符，参数512表示epoll最大支持512个事件
    assert(epollFd_ >= 0 && events_.size() > 0);
}

// 销毁Epoller类
Epoller::~Epoller() {
    // 关闭epoll文件描述符
    close(epollFd_);
}

// 向epoll实例中添加一个文件描述符
bool Epoller::AddFd(int fd, uint32_t events) {
    // 如果文件描述符小于0，则返回false
    if(fd < 0) return false;
    // 构造epoll_event结构体
    epoll_event ev = {0};
    // 将文件描述符设置到ev结构体中
    ev.data.fd = fd;
    // 设置ev结构体中的事件
    ev.events = events;
    // 向epoll实例中添加文件描述符
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::ModFd(int fd, uint32_t events) {
    // 如果fd小于0，则返回false
    if(fd < 0) return false;
    // 初始化epoll_event结构体
    epoll_event ev = {0};
    // 将fd赋值给ev.data.fd
    ev.data.fd = fd;
    // 将events赋值给ev.events
    ev.events = events;
    // 调用epoll_ctl函数，将fd修改为指定的events
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::DelFd(int fd) {
    // 如果fd小于0，则返回false
    if(fd < 0) return false;
    // 调用epoll_ctl函数，从epollFd_中删除fd，0表示不需要额外的参数
    return 0 == epoll_ctl(epollFd_, EPOLL_CTL_DEL, fd, 0);
}

// 返回事件数量
// Wait函数用于等待epoll事件
int Epoller::Wait(int timeoutMs) {
    // epoll_wait函数用于等待epoll事件
    return epoll_wait(epollFd_, &events_[0], static_cast<int>(events_.size()), timeoutMs);
}

// 获取事件的fd
int Epoller::GetEventFd(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].data.fd;
}

// 获取事件属性
uint32_t Epoller::GetEvents(size_t i) const {
    assert(i < events_.size() && i >= 0);
    return events_[i].events;
}
