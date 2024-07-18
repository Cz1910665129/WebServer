#include "sqlconnpool.h"
//创建静态单例对象
SqlConnPool* SqlConnPool::Instance() {
    static SqlConnPool pool;
    return &pool;
}

// 初始化
// 初始化数据库连接池
void SqlConnPool::Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize = 10) {
    // 断言连接池大小必须大于0
    assert(connSize > 0);
    // 循环初始化连接
    for(int i = 0; i < connSize; i++) {
        MYSQL* conn = nullptr;
        // 初始化连接
        conn = mysql_init(conn);
        if(!conn) {
            LOG_ERROR("MySql init error!");
            assert(conn);
        }
        // 连接数据库
        conn = mysql_real_connect(conn, host, user, pwd, dbName, port, nullptr, 0);
        if (!conn) {
            LOG_ERROR("MySql Connect error!");
        }
        // 将连接放入队列
        connQue_.emplace(conn);
    }
    // 设置最大连接数
    MAX_CONN_ = connSize;
    // 初始化信号量
    sem_init(&semId_, 0, MAX_CONN_);
}

MYSQL* SqlConnPool::GetConn() {
    MYSQL* conn = nullptr;
    if(connQue_.empty()) {
        LOG_WARN("SqlConnPool busy!");
        return nullptr;
    }
    sem_wait(&semId_);  // -1
    lock_guard<mutex> locker(mtx_);
    conn = connQue_.front();
    connQue_.pop();
    return conn;
}

// 存入连接池，实际上没有关闭
void SqlConnPool::FreeConn(MYSQL* conn) {
    assert(conn);
    lock_guard<mutex> locker(mtx_);
    connQue_.push(conn);
    sem_post(&semId_);  // +1
}

// 关闭数据库连接池
void SqlConnPool::ClosePool() {
    // 使用互斥锁保护connQue_
    lock_guard<mutex> locker(mtx_);
    // 遍历connQue_，关闭每一个连接
    while(!connQue_.empty()) {
        // 获取连接队列的第一个连接
        auto conn = connQue_.front();
        // 弹出连接队列的第一个连接
        connQue_.pop();
        // 关闭连接
        mysql_close(conn);
    }
    // 结束MySQL库
    mysql_library_end();
}

int SqlConnPool::GetFreeConnCount() {
    // 互斥锁保护下面代码
    lock_guard<mutex> locker(mtx_);
    // 返回连接队列中空闲连接的数量
    return connQue_.size();
}
