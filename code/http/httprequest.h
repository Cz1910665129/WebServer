#ifndef HTTP_REQUEST_H
#define HTTP_REQUEST_H

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>    // 正则表达式
#include <errno.h>     
#include <mysql/mysql.h>  //mysql

#include "../buffer/buffer.h"
#include "../log/log.h"
#include "../pool/sqlconnpool.h"

class HttpRequest {
public:
    enum PARSE_STATE {
        /// 解析请求行
        REQUEST_LINE,
        /// 解析头部
        HEADERS,
        /// 解析正文
        BODY,
        /// 解析结束
        FINISH,        
    };
    
    HttpRequest() { Init(); }
    ~HttpRequest() = default;

    // 初始化
    void Init();
    // 解析Buffer
    bool parse(Buffer& buff);   

    // 获取请求路径
    std::string path() const;
    // 获取请求路径的引用
    std::string& path();
    // 获取请求方法
    std::string method() const;
    // 获取请求版本
    std::string version() const;
    // 获取POST参数
    std::string GetPost(const std::string& key) const;
    // 获取POST参数
    std::string GetPost(const char* key) const;

    // 判断是否保持连接
    bool IsKeepAlive() const;

private:
    bool ParseRequestLine_(const std::string& line);    // 处理请求行
    void ParseHeader_(const std::string& line);         // 处理请求头
    void ParseBody_(const std::string& line);           // 处理请求体

    void ParsePath_();                                  // 处理请求路径
    void ParsePost_();                                  // 处理Post事件
    void ParseFromUrlencoded_();                        // 从url种解析编码

    static bool UserVerify(const std::string& name, const std::string& pwd, bool isLogin);  // 用户验证

    // 定义解析状态
    PARSE_STATE state_;
    // 存储请求方法
    std::string method_;
    // 存储请求路径
    std::string path_;
    // 存储请求版本
    std::string version_;
    // 存储请求体
    std::string body_;
    // 存储请求头
    std::unordered_map<std::string, std::string> header_;
    // 存储请求参数
    std::unordered_map<std::string, std::string> post_;

    // 定义一个默认的HTML字符串
    static const std::unordered_set<std::string> DEFAULT_HTML;
    // 定义一个默认的HTML标签
    static const std::unordered_map<std::string, int> DEFAULT_HTML_TAG;
    static int ConverHex(char ch);  // 16进制转换为10进制
};

#endif