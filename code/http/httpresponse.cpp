#include "httpresponse.h"

using namespace std;
    // 后缀类型集
const unordered_map<string, string> HttpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};
    // 编码状态集
const unordered_map<int, string> HttpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};
    // 编码路径集
const unordered_map<int, string> HttpResponse::CODE_PATH = {
    { 400, "/400.html" },
    { 403, "/403.html" },
    { 404, "/404.html" },
};

// 构造函数
HttpResponse::HttpResponse() {
    // 初始化响应状态码
    code_ = -1;
    // 初始化文件路径
    path_ = srcDir_ = "";
    // 初始化连接状态
    isKeepAlive_ = false;
    // 初始化文件指针
    mmFile_ = nullptr; 
    // 初始化文件状态
    mmFileStat_ = { 0 };
};

HttpResponse::~HttpResponse() {
    UnmapFile();
}

// 初始化HttpResponse类的成员变量
void HttpResponse::Init(const string& srcDir, string& path, bool isKeepAlive, int code){
    // 断言srcDir不为空
    assert(srcDir != "");
    // 如果mmFile_不为空，则取消映射文件
    if(mmFile_) { UnmapFile(); }
    // 设置code_
    code_ = code;
    // 设置isKeepAlive_
    isKeepAlive_ = isKeepAlive;
    // 设置path_
    path_ = path;
    // 设置srcDir_
    srcDir_ = srcDir;
    // 设置mmFile_为空
    mmFile_ = nullptr; 
    // 设置mmFileStat_
    mmFileStat_ = { 0 };
}

void HttpResponse::MakeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    if(stat((srcDir_ + path_).data(), &mmFileStat_) < 0 || S_ISDIR(mmFileStat_.st_mode)) {
        code_ = 404;
    }
    else if(!(mmFileStat_.st_mode & S_IROTH)) {
        code_ = 403;
    }
    else if(code_ == -1) { 
        code_ = 200; 
    }
    ErrorHtml_();
    AddStateLine_(buff);
    AddHeader_(buff);
    AddContent_(buff);
}

char* HttpResponse::File() {
    return mmFile_;
}

size_t HttpResponse::FileLen() const {
    return mmFileStat_.st_size;
}

// 如果code_在CODE_PATH中存在
void HttpResponse::ErrorHtml_() {
    if(CODE_PATH.count(code_) == 1) {
        // 获取code_对应的path_
        path_ = CODE_PATH.find(code_)->second;
        // 获取srcDir_和path_拼接的路径的文件状态
        stat((srcDir_ + path_).data(), &mmFileStat_);
    }
}

void HttpResponse::AddStateLine_(Buffer& buff) {
    // 定义一个字符串变量status
    string status;
    // 判断code_是否在CODE_STATUS中
    if(CODE_STATUS.count(code_) == 1) {
        // 如果在，则将code_对应的状态码赋值给status
        status = CODE_STATUS.find(code_)->second;
    }
    else {
        // 如果不在，则将状态码置为400，并将400对应的状态码赋值给status
        code_ = 400;
        status = CODE_STATUS.find(400)->second;
    }
    // 将HTTP版本、状态码、状态码描述拼接成字符串，并添加到buff中
    buff.Append("HTTP/1.1 " + to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::AddHeader_(Buffer& buff) {
    //添加HTTP响应头
    buff.Append("Connection: ");
    //判断是否保持连接
    if(isKeepAlive_) {
        //保持连接
        buff.Append("keep-alive\r\n");
        //设置最大连接数和超时时间
        buff.Append("keep-alive: max=6, timeout=120\r\n");
    } else{
        //关闭连接
        buff.Append("close\r\n");
    }
    //添加响应内容类型
    buff.Append("Content-type: " + GetFileType_() + "\r\n");
}

void HttpResponse::AddContent_(Buffer& buff) {
    int srcFd = open((srcDir_ + path_).data(), O_RDONLY);
    if(srcFd < 0) { 
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    //将文件映射到内存提高文件的访问速度  MAP_PRIVATE 建立一个写入时拷贝的私有映射
    LOG_DEBUG("file path %s", (srcDir_ + path_).data());
    int* mmRet = (int*)mmap(0, mmFileStat_.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        ErrorContent(buff, "File NotFound!");
        return; 
    }
    mmFile_ = (char*)mmRet;
    //关闭文件描述符
    close(srcFd);
    //将Content-length添加到响应头中
    buff.Append("Content-length: " + to_string(mmFileStat_.st_size) + "\r\n\r\n");
}

void HttpResponse::UnmapFile() {
    if(mmFile_) {
        munmap(mmFile_, mmFileStat_.st_size);
        mmFile_ = nullptr;
    }
}

// 判断文件类型 
string HttpResponse::GetFileType_() {
    string::size_type idx = path_.find_last_of('.');
    if(idx == string::npos) {   // 最大值 find函数在找不到指定值得情况下会返回string::npos
        return "text/plain";
    }
    string suffix = path_.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void HttpResponse::ErrorContent(Buffer& buff, string message) 
{
    string body;
    string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code_) == 1) {
        status = CODE_STATUS.find(code_)->second;
    } else {
        status = "Bad Request";
    }
    body += to_string(code_) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>TinyWebServer</em></body></html>";

    buff.Append("Content-length: " + to_string(body.size()) + "\r\n\r\n");
    buff.Append(body);
}

