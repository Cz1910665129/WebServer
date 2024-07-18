#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include "../buffer/buffer.h"
#include "../log/log.h"

class HttpResponse {
public:
    HttpResponse();
    ~HttpResponse();

    // 初始化，参数：源文件目录，路径，是否保持存活，错误码
    void Init(const std::string& srcDir, std::string& path, bool isKeepAlive = false, int code = -1);
    // 构造响应
    void MakeResponse(Buffer& buff);
    // 取消映射文件
    void UnmapFile();
    // 获取文件指针
    char* File();
    // 获取文件长度
    size_t FileLen() const;
    // 构造错误响应
    void ErrorContent(Buffer& buff, std::string message);
    // 获取错误码
    int Code() const { return code_; }

private:

    //  添加状态行到缓冲区buff中
    void AddStateLine_(Buffer &buff);
    // 向缓冲区buff中添加HTTP响应头
    void AddHeader_(Buffer &buff);
    // 向缓冲区buff中添加HTTP响应内容
    void AddContent_(Buffer &buff);

    // 发生错误时调用
    void ErrorHtml_();
    // 获取文件类型
    std::string GetFileType_();

    // HTTP状态码
    int code_;
    // 是否保持连接
    bool isKeepAlive_;

    // 请求路径
    std::string path_;
    // 源文件目录
    std::string srcDir_;
    
    // 源文件
    char* mmFile_; 
    // 源文件状态
    struct stat mmFileStat_;

    // 后缀类型集
    static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;  
    // 编码状态集
    static const std::unordered_map<int, std::string> CODE_STATUS;          
    // 编码路径集
    static const std::unordered_map<int, std::string> CODE_PATH;            
};


#endif //HTTP_RESPONSE_H

