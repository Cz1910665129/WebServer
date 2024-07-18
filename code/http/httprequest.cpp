#include "httprequest.h"
using namespace std;

// 定义一个常量unordered_set，存储默认的HTML页面
const unordered_set<string> HttpRequest::DEFAULT_HTML{
            "/index", "/register", "/login",
             "/welcome", "/video", "/picture", };

// 定义一个常量unordered_map，存储默认HTML页面的标签
const unordered_map<string, int> HttpRequest::DEFAULT_HTML_TAG {
            {"/register.html", 0}, {"/login.html", 1},  };

void HttpRequest::Init() {
    // 初始化请求方法
    method_ = path_ = version_ = body_ = "";
    // 设置请求状态
    state_ = REQUEST_LINE;
    // 清空请求头
    header_.clear();
    // 清空请求体
    post_.clear();
}

bool HttpRequest::IsKeepAlive() const {
    //如果header_中"Connection"的个数为1
   if(header_.count("Connection") == 1) {
        //如果header_中"Connection"的值为"keep-alive"且version_为"1.1"
        return header_.find("Connection")->second == "keep-alive" && version_ == "1.1";
    }
    //否则返回false
    return false;
}

// 解析处理
bool HttpRequest::parse(Buffer& buff) {
    const char CRLF[] = "\r\n";      // 行结束符标志(回车换行)
    if(buff.ReadableBytes() <= 0) { // 没有可读的字节
        return false;
    }
    // 读取数据
    while(buff.ReadableBytes() && state_ != FINISH) {
        // 从buff中的读指针开始到读指针结束，这块区域是未读取的数据并去处"\r\n"，返回有效数据得行末指针
        const char* lineEnd = search(buff.Peek(), buff.BeginWriteConst(), CRLF, CRLF + 2);      //获取一行的末尾位置指针
        // 转化为string类型
        std::string line(buff.Peek(), lineEnd);
        switch(state_)
        {
        /*
            有限状态机，从请求行开始，每处理完后会自动转入到下一个状态    
        */
        case REQUEST_LINE:      
            if(!ParseRequestLine_(line)) {
                return false;
            }
            ParsePath_();   // 解析路径
            break;    
        case HEADERS:
            ParseHeader_(line);
            if(buff.ReadableBytes() <= 2) { 
                state_ = FINISH;
            }
            break;
        case BODY:
            ParseBody_(line);
            break;
        default:
            break;
        }
        if(lineEnd == buff.BeginWrite()) { break; } // 读完了
        buff.RetrieveUntil(lineEnd + 2);        // 跳过回车换行
    }
    LOG_DEBUG("[%s], [%s], [%s]", method_.c_str(), path_.c_str(), version_.c_str());
    return true;
}

// 解析路径
void HttpRequest::ParsePath_() {
    // 如果path_为"/"，则将path_设置为"/index.html"
    if(path_ == "/") {
        path_ = "/index.html"; 
    }
    // 否则，遍历DEFAULT_HTML数组，如果数组中的元素与path_相等，则将path_设置为"path_.html"
    else {
        for(auto &item: DEFAULT_HTML) {
            if(item == path_) {
                path_ += ".html";
                break;
            }
        }
    }
}

bool HttpRequest::ParseRequestLine_(const string& line) {
    regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$"); 
    smatch subMatch;
    // 在匹配规则中，以括号()的方式来划分组别 一共三个括号 [0]表示整体
    if(regex_match(line, subMatch, patten)) {      // 匹配指定字符串整体是否符合
        method_ = subMatch[1];
        path_ = subMatch[2];
        version_ = subMatch[3];
        state_ = HEADERS;   // 状态转换为下一个状态
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void HttpRequest::ParseHeader_(const string& line) {
    // 利用正则表达式解析头部以冒号分割的键值对信息
    regex patten("^([^:]*): ?(.*)$");
    smatch subMatch;//存放匹配结果
    // 如果当前行line匹配正则表达式patten
    if(regex_match(line, subMatch, patten)) {
        // 将解析出来的头部信息存入map中
        header_[subMatch[1]] = subMatch[2];
    }
    else {
        // 如果解析失败，则状态转换为下一个状态
        state_ = BODY;  // 状态转换为下一个状态
    }
}

void HttpRequest::ParseBody_(const string& line) {
    body_ = line;
    ParsePost_();
    state_ = FINISH;    // 状态转换为下一个状态
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

// 16进制转化为10进制
int HttpRequest::ConverHex(char ch) {
    if(ch >= 'A' && ch <= 'F') return ch -'A' + 10;
    if(ch >= 'a' && ch <= 'f') return ch -'a' + 10;
    return ch;
}

// 处理post请求
void HttpRequest::ParsePost_() {
    if(method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
        ParseFromUrlencoded_();     // POST请求体示例
        if(DEFAULT_HTML_TAG.count(path_)) { // 如果是登录/注册的path            查找_path的值是否在DEFAULT_HTML_TAG中
            int tag = DEFAULT_HTML_TAG.find(path_)->second; 
            LOG_DEBUG("Tag:%d", tag);
            if(tag == 0 || tag == 1) {
                bool isLogin = (tag == 1);  // 为1则是登录
                if(UserVerify(post_["username"], post_["password"], isLogin)) {
                    path_ = "/welcome.html";
                } 
                else {
                    path_ = "/error.html";
                }
            }
        }
    }   
}

// 从url中解析编码
void HttpRequest::ParseFromUrlencoded_() {
    if(body_.size() == 0) { return; }   // 如果body_为空，直接返回
    string key, value;                  // 定义key和value
    int num = 0;                        // 用于转换16进制
    int n = body_.size();               // body_的长度
    int i = 0, j = 0;                   // 初始化j为0
    for(; i < n; i++) {                 // 遍历body_
        char ch = body_[i];             // 当前字符
        switch (ch) {       
        case '=':                       // key          
            key = body_.substr(j, i - j); // 获取key
            j = i + 1;
            break;      
        case '+':                       // 键值对中的空格换为+或者%20
            body_[i] = ' ';             // 将+替换为空格
            break;
        case '%':                     
            num = ConverHex(body_[i + 1]) * 16 + ConverHex(body_[i + 2]);           // 获取16进制数          
            body_[i + 2] = num % 10 + '0';      // 将16进制数转换为字符
            body_[i + 1] = num / 10 + '0';
            i += 2;
            break;       
        case '&':                           // 键值对连接符           
            value = body_.substr(j, i - j);// 获取value
            j = i + 1;           
            post_[key] = value;            // 将key和value插入到post_中
            LOG_DEBUG("%s = %s", key.c_str(), value.c_str());
            break;
        default:
            break;
        }
    }
    assert(j <= i);                         // 断言j小于i
    if(post_.count(key) == 0 && j < i) {    // 如果post_中没有key，将key和value插入到post_中
        value = body_.substr(j, i - j);
        post_[key] = value;
    }
}

bool HttpRequest::UserVerify(const string &name, const string &pwd, bool isLogin) {
    // 用户名或密码为空，返回false
    if(name == "" || pwd == "") { return false; }
    LOG_INFO("Verify name:%s pwd:%s", name.c_str(), pwd.c_str());
    MYSQL* sql;
    // 从连接池中获取一个连接
    SqlConnRAII(&sql,  SqlConnPool::Instance());
    assert(sql);
    
    bool flag = false;
    unsigned int j = 0;
    char order[256] = { 0 };
    MYSQL_FIELD *fields = nullptr;
    MYSQL_RES *res = nullptr;
    
    // 如果是注册行为，默认值为true
    if(!isLogin) { flag = true; }
    /* 查询用户及密码 */
    // 查询用户名对应的密码
    snprintf(order, 256, "SELECT username, password FROM user WHERE username='%s' LIMIT 1", name.c_str());
    LOG_DEBUG("%s", order);

    if(mysql_query(sql, order)) { 
        mysql_free_result(res);
        return false; 
    }
    // 获取查询结果
    res = mysql_store_result(sql);
    j = mysql_num_fields(res);
    fields = mysql_fetch_fields(res);

    while(MYSQL_ROW row = mysql_fetch_row(res)) {
        LOG_DEBUG("MYSQL ROW: %s %s", row[0], row[1]);
        string password(row[1]);
        /* 注册行为 且 用户名未被使用*/
        if(isLogin) {
            if(pwd == password) { flag = true; }
            else {
                flag = false;
                LOG_INFO("pwd error!");
            }
        } 
        else { 
            flag = false; 
            LOG_INFO("user used!");
        }
    }
    mysql_free_result(res);

    /* 注册行为 且 用户名未被使用*/
    if(!isLogin && flag == true) {
        LOG_DEBUG("regirster!");
        bzero(order, 256);
        // 插入用户名和密码
        snprintf(order, 256,"INSERT INTO user(username, password) VALUES('%s','%s')", name.c_str(), pwd.c_str());
        LOG_DEBUG( "%s", order);
        if(mysql_query(sql, order)) { 
            LOG_DEBUG( "Insert error!");
            flag = false; 
        }
        flag = true;
    }
    // SqlConnPool::Instance()->FreeConn(sql);
    LOG_DEBUG( "UserVerify success!!");
    return flag;
}

std::string HttpRequest::path() const{
    return path_;
}

std::string& HttpRequest::path(){
    return path_;
}
std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::version() const {
    return version_;
}

// 获取HttpRequest类的post参数
std::string HttpRequest::GetPost(const std::string& key) const {
    // 断言key不为空
    assert(key != "");
    // 如果post参数中存在key，则返回对应的值
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    // 否则返回空字符串
    return "";
}

std::string HttpRequest::GetPost(const char* key) const {
    assert(key != nullptr);
    if(post_.count(key) == 1) {
        return post_.find(key)->second;
    }
    return "";
}
