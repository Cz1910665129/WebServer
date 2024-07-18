#ifndef BUFFER_H
#define BUFFER_H
#include <cstring>   
#include <iostream>
#include <unistd.h>  
#include <sys/uio.h> 
#include <vector> 
#include <atomic>
#include <assert.h>

class Buffer {
public:
    // 构造函数，初始化缓冲区大小为1024
    Buffer(int initBuffSize = 1024);
    // 默认的析构函数
    ~Buffer() = default;

    // 返回可写入的字节数
    size_t WritableBytes() const;       
    // 返回可读取的字节数
    size_t ReadableBytes() const ;
    // 返回可预写入的字节数
    size_t PrependableBytes() const;

    // 返回缓冲区的头部
    const char* Peek() const;
    // 确保可写入指定字节数
    void EnsureWriteable(size_t len);
    // 表示已写入指定字节数
    void HasWritten(size_t len);

    // 缓冲区中取出指定字节数
    void Retrieve(size_t len);
    // 缓冲区中取出，直到指定位置的字节数
    void RetrieveUntil(const char* end);

    // 缓冲区中取出所有字节
    void RetrieveAll();
    // 将缓冲区中的字节转换为字符串
    std::string RetrieveAllToStr();

    // 缓冲区头部可写
    const char* BeginWriteConst() const;
    // 缓冲区头部可写
    char* BeginWrite();

    // 将字符串写入缓冲区
    void Append(const std::string& str);
    // 将字符串写入缓冲区
    void Append(const char* str, size_t len);
    // 将数据写入缓冲区
    void Append(const void* data, size_t len);
    // 将缓冲区写入缓冲区
    void Append(const Buffer& buff);

    // 从指定文件描述符中读取
    ssize_t ReadFd(int fd, int* Errno);
    // 将数据写入指定文件描述符中
    ssize_t WriteFd(int fd, int* Errno);

private:
    // 缓冲区的开头
    char* BeginPtr_();  // buffer开头
    // 缓冲区的开头
    const char* BeginPtr_() const;
    // 扩充缓冲区
    void MakeSpace_(size_t len);

    // 缓冲区
    std::vector<char> buffer_;  
    // 读的下标
    std::atomic<std::size_t> readPos_;  
    // 写的下标
    std::atomic<std::size_t> writePos_; 
};

#endif //BUFFER_H
