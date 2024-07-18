#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <sys/types.h>
#include <sys/uio.h>     // readv/writev
#include <arpa/inet.h>   // sockaddr_in
#include <stdlib.h>      // atoi()
#include <errno.h>      

#include "../log/log.h"
#include "../buffer/buffer.h"
#include "httprequest.h"
#include "httpresponse.h"
/*
进行读写数据并调用httprequest 来解析数据以及httpresponse来生成响应
*/
class HttpConn {
public:
    HttpConn();
    ~HttpConn();
    
    // 初始化连接
    void init(int sockFd, const sockaddr_in& addr);
    // 从socket读取数据
    ssize_t read(int* saveErrno);
    // 向socket写入数据
    ssize_t write(int* saveErrno);
    // 关闭连接
    void Close();
    // 获取socket的fd
    int GetFd() const;
    // 获取端口号
    int GetPort() const;
    // 获取IP地址
    const char* GetIP() const;
    // 获取socket地址
    sockaddr_in GetAddr() const;
    // 处理请求
    bool process();

    // 写的总长度
    int ToWriteBytes() { 
        return iov_[0].iov_len + iov_[1].iov_len; 
    }

    // 是否长连接
    bool IsKeepAlive() const {
        return request_.IsKeepAlive();
    }

    // 是否边缘触发
    static bool isET;
    // 资源文件路径
    static const char* srcDir;
    // 用户数量，原子操作，支持锁
    static std::atomic<int> userCount;  
    
private:
   
    int fd_;        //文件描述符
    // 定义一个sockaddr_in结构体变量addr_ 存储IPv4地址的结构体，通常用于表示网络连接的远程地址
    struct  sockaddr_in addr_;   
    // 定义一个布尔变量isClose_
    bool isClose_;
    
    int iovCnt_;  //io向量的数量
    // 定义一个iovec数组变量iov_
    struct iovec iov_[2];       //iovec用于描述一个缓冲区（buffer）的地址和长度
    
    // 读缓冲区
    Buffer readBuff_; 
    // 写缓冲区
    Buffer writeBuff_; 

    // 请求
    HttpRequest request_;
    // 响应
    HttpResponse response_;
};

#endif
