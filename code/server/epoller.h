#ifndef EPOLLER_H
#define EPOLLER_H

#include <sys/epoll.h> //epoll_ctl()
#include <unistd.h> // close()
#include <assert.h> // close()
#include <vector>
#include <errno.h>

class Epoller {
public:
    explicit Epoller(int maxEvent = 1024);  // 构造函数，创建一个最大事件数为1024的Epoller对象
    ~Epoller();                             // 析构函数，释放Epoller对象占用的资源

    bool AddFd(int fd, uint32_t events); // 将一个文件描述符添加到Epoller中，并设置关注的事件
    bool ModFd(int fd, uint32_t events); // 修改Epoller中某个文件描述符关注的事件
    bool DelFd(int fd);                 // 从Epoller中移除一个文件描述符
    int Wait(int timeoutMs = -1);       // 等待事件发生，最多等待时间为timeoutMs
    int GetEventFd(size_t i) const;     // 获取第i个发生的事件的文件描述符
    uint32_t GetEvents(size_t i) const; // 获取第i个发生的事件
        
private:
    int epollFd_;                               // Epoller的文件描述符
    std::vector<struct epoll_event> events_;    // Epoller的事件数组
};

#endif //EPOLLER_H
