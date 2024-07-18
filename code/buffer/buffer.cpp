#include "buffer.h"

// 读写下标初始化，vector<char>初始化
Buffer::Buffer(int initBuffSize) : buffer_(initBuffSize), readPos_(0), writePos_(0)  {}  

// 可写的数量：buffer大小 - 写下标
size_t Buffer::WritableBytes() const {
    return buffer_.size() - writePos_;
}

// 可读的数量：写下标 - 读下标
size_t Buffer::ReadableBytes() const {
    return writePos_ - readPos_;
}

// 可预留空间：已经读过的就没用了，等于读下标
size_t Buffer::PrependableBytes() const {
    return readPos_;
}

// 返回当前读取位置的缓冲区字符串
const char* Buffer::Peek() const {
    return &buffer_[readPos_];
}

// 确保可写的长度
void Buffer::EnsureWriteable(size_t len) {
    // 如果要大于可写的字节数，则扩展缓冲区
    if(len > WritableBytes()) {
        MakeSpace_(len);
    }
    // 确保写入的数据字节数不超过可写的字节数
    assert(len <= WritableBytes());
}

// 移动写下标，在Append中使用
void Buffer::HasWritten(size_t len) {
    writePos_ += len;
}

// 读取len长度，移动读下标
void Buffer::Retrieve(size_t len) {
    readPos_ += len;
}

// 读取到end位置
void Buffer::RetrieveUntil(const char* end) {
    assert(Peek() <= end );
    Retrieve(end - Peek());         
}

// 取出所有数据，buffer归零，读写下标归零,在别的函数中会用到
void Buffer::RetrieveAll() {
    bzero(&buffer_[0], buffer_.size()); // 覆盖原本数据
    readPos_ = writePos_ = 0;
}

// 取出剩余可读的str
std::string Buffer::RetrieveAllToStr() {
    std::string str(Peek(), ReadableBytes());
    RetrieveAll();
    return str;
}

// 写指针的位置
// 获取可写入缓冲区的首地址
const char* Buffer::BeginWriteConst() const {
    return &buffer_[writePos_];
}

// 获取可写入缓冲区的首地址
char* Buffer::BeginWrite() {
    return &buffer_[writePos_];
}

// 添加str到缓冲区
void Buffer::Append(const char* str, size_t len) {
    assert(str);
    EnsureWriteable(len);   // 确保可写的长度
    std::copy(str, str + len, BeginWrite());    // 将str放到写下标开始的地方
    HasWritten(len);    // 移动写下标
}

// 向缓冲区中追加字符串           
void Buffer::Append(const std::string& str) {
    // 向缓冲区中追加字符串的c_str和长度
    Append(str.c_str(), str.size());
}

// 向缓冲区中追加数据
void Append(const void* data, size_t len) {
    // 向缓冲区中追加数据的c_str和长度
    Append(static_cast<const char*>(data), len);
}

// 将buffer中的读下标的地方放到该buffer中的写下标位置
void Append(const Buffer& buff) {
    Append(buff.Peek(), buff.ReadableBytes());
}

           
ssize_t Buffer::ReadFd(int fd, int* Errno) {
    char buff[65535];   // 栈区
    struct iovec iov[2];
    size_t writeable = WritableBytes(); // 先记录能写多少
    // 分散读， 保证数据全部读完
    iov[0].iov_base = BeginWrite();
    iov[0].iov_len = writeable;
    iov[1].iov_base = buff;
    iov[1].iov_len = sizeof(buff);

    ssize_t len = readv(fd, iov, 2);
    if(len < 0) {
        *Errno = errno;
    } else if(static_cast<size_t>(len) <= writeable) {   // 若len小于writable，说明写区可以容纳len
        writePos_ += len;   // 直接移动写下标
    } else {    
        writePos_ = buffer_.size(); // 写区写满了,下标移到最后
        Append(buff, static_cast<size_t>(len - writeable)); // 剩余的长度
    }
    return len;
}


ssize_t Buffer::WriteFd(int fd, int* Errno) {
    ssize_t len = write(fd, Peek(), ReadableBytes());
    if(len < 0) {
        *Errno = errno;
        return len;
    } 
    Retrieve(len);
    return len;
}

// 返回缓冲区的开始指针
char* Buffer::BeginPtr_() {
    return &buffer_[0];
}

// 获取缓冲区的开始指针
const char* Buffer::BeginPtr_() const{
    return &buffer_[0];
}

// 扩展空间
void Buffer::MakeSpace_(size_t len) {
    // 如果可写空间加上可前置空间小于需要的长度，则扩容
    if(WritableBytes() + PrependableBytes() < len) {
        // 将缓冲区的大小调整为writePos_+len+1
        buffer_.resize(writePos_ + len + 1);
    } else {
        // 将可读区域的数据复制到缓冲区的开始位置
        size_t readable = ReadableBytes();
        std::copy(BeginPtr_() + readPos_, BeginPtr_() + writePos_, BeginPtr_());
        readPos_ = 0;
        writePos_ = readable;
        // 断言可读空间等于实际可读空间
        assert(readable == ReadableBytes());
    }
}
