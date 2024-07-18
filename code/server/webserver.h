#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "epoller.h"
#include "../timer/heaptimer.h"

#include "../log/log.h"
#include "../pool/sqlconnpool.h"
#include "../pool/threadpool.h"

#include "../http/httpconn.h"

class WebServer {
public:
    WebServer(
        int port, int trigMode, int timeoutMS, bool OptLinger, 
        int sqlPort, const char* sqlUser, const  char* sqlPwd, 
        const char* dbName, int connPoolNum, int threadNum,
        bool openLog, int logLevel, int logQueSize);

    ~WebServer();
    void Start();

private:
    bool InitSocket_(); 
    // 初始化事件模式
    void InitEventMode_(int trigMode);
    // 添加客户端
    void AddClient_(int fd, sockaddr_in addr);
  
    // 处理监听
    void DealListen_();
    // 处理写入
    void DealWrite_(HttpConn* client);
    // 处理读取
    void DealRead_(HttpConn* client);

    // 发送错误
    void SendError_(int fd, const char*info);
    // 延长连接时间
    void ExtentTime_(HttpConn* client);
    // 关闭连接
    void CloseConn_(HttpConn* client);

    // 读取事件
    void OnRead_(HttpConn* client);
    // 写入事件
    void OnWrite_(HttpConn* client);
    // 处理请求
    void OnProcess(HttpConn* client);

    // 最大文件描述符
    static const int MAX_FD = 65536;

    // 设置文件描述符非阻塞
    static int SetFdNonblock(int fd);

    // 端口号
    int port_;
    // 是否打开linger
    bool openLinger_;
    // 超时时间，毫秒
    int timeoutMS_;  /* 毫秒MS */
    // 是否关闭
    bool isClose_;
    // 监听文件描述符
    int listenFd_;
    // 源目录
    char* srcDir_;
    
    // 监听事件
    uint32_t listenEvent_;  // 监听事件
    // 连接事件
    uint32_t connEvent_;    // 连接事件
   
    // 堆定时器
    std::unique_ptr<HeapTimer> timer_;
    // 线程池
    std::unique_ptr<ThreadPool> threadpool_;
    // epoll
    std::unique_ptr<Epoller> epoller_;
    // 用户
    std::unordered_map<int, HttpConn> users_;
};

#endif //WEBSERVER_H
