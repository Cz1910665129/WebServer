#include "httpconn.h"
using namespace std;

const char* HttpConn::srcDir;
std::atomic<int> HttpConn::userCount;
bool HttpConn::isET;

HttpConn::HttpConn() { 
    fd_ = -1;
    addr_ = { 0 };
    isClose_ = true;
};

HttpConn::~HttpConn() { 
    Close(); 
};

void HttpConn::init(int fd, const sockaddr_in& addr) {
    assert(fd > 0);
    // 客户端连接数量+1
    userCount++;
    // 保存客户端地址信息
    addr_ = addr;
    // 保存客户端socket描述符
    fd_ = fd;
    // 清空写缓冲区
    writeBuff_.RetrieveAll();
    // 清空读缓冲区
    readBuff_.RetrieveAll();
    // 设置连接状态为false
    isClose_ = false;
    // 打印客户端连接信息
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
}

void HttpConn::Close() {
    // 取消文件映射
    response_.UnmapFile();
    // 如果isClose_为false，则将isClose_置为true，并将用户数减一
    if(isClose_ == false){
        isClose_ = true; 
        userCount--;
        // 关闭文件描述符
        close(fd_);
        // 记录日志
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", fd_, GetIP(), GetPort(), (int)userCount);
    }
}

int HttpConn::GetFd() const {
    return fd_;
};

struct sockaddr_in HttpConn::GetAddr() const {
    return addr_;
}

// 获取IP地址
const char* HttpConn::GetIP() const {
    // 将网络地址转换为字符串
    return inet_ntoa(addr_.sin_addr);
}

int HttpConn::GetPort() const {
    // 返回socket的端口号
    return addr_.sin_port;
}

ssize_t HttpConn::read(int* saveErrno) {
    // 读取客户端发送的数据         即读取buff中的请求信息
    ssize_t len = -1;
    do {
        // 从fd中读取数据到readBuff_中
        len = readBuff_.ReadFd(fd_, saveErrno);
        if (len <= 0) {
            break;
        }
    } while (isET); // ET:边沿触发要一次性全部读出
    return len;
}

bool HttpConn::process() {                  //从readbuff_中解析请求，生成响应报文，放入writeBuff_中
    request_.Init();
    if(readBuff_.ReadableBytes() <= 0) {
        return false;
    }
    else if(request_.parse(readBuff_)) {    // 解析成功
        LOG_DEBUG("%s", request_.path().c_str());
        response_.Init(srcDir, request_.path(), request_.IsKeepAlive(), 200);
    } else {
        response_.Init(srcDir, request_.path(), false, 400);
    }

    response_.MakeResponse(writeBuff_); // 生成响应报文放入writeBuff_中
    // 响应头
    iov_[0].iov_base = const_cast<char*>(writeBuff_.Peek()); // 将writeBuff_中的数据指针赋值给iov_[0].iov_base
    iov_[0].iov_len = writeBuff_.ReadableBytes(); // 将writeBuff_中可读字节数赋值给iov_[0].iov_len
    iovCnt_ = 1; // iovCnt_置为1

    // 文件
    if(response_.FileLen() > 0  && response_.File()) {
        iov_[1].iov_base = response_.File();
        iov_[1].iov_len = response_.FileLen();   
        iovCnt_ = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response_.FileLen() , iovCnt_, ToWriteBytes());
    return true;
}


// 主要采用writev连续写函数
ssize_t HttpConn::write(int* saveErrno) {           
    /* write()将writeBuff_中的内容写入fd */
    ssize_t len = -1;
    do {
        len = writev(fd_, iov_, iovCnt_);   // 将iov(writeBuff_)的内容写到fd中
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov_[0].iov_len + iov_[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov_[0].iov_len) {
            iov_[1].iov_base = (uint8_t*) iov_[1].iov_base + (len - iov_[0].iov_len);
            iov_[1].iov_len -= (len - iov_[0].iov_len);
            if(iov_[0].iov_len) {
                writeBuff_.RetrieveAll();
                iov_[0].iov_len = 0;
            }
        }
        else {
            iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len; 
            iov_[0].iov_len -= len; 
            writeBuff_.Retrieve(len);
        }
    } while(isET || ToWriteBytes() > 10240);
    return len;
}



