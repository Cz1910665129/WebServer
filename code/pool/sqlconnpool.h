#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    // 获取单例对象
    static SqlConnPool *Instance();
    // 获取一个连接
    MYSQL *GetConn();
    // 释放一个连接
    void FreeConn(MYSQL * conn);
    // 获取当前空闲连接数
    int GetFreeConnCount();

    // 初始化连接池
    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    // 关闭连接池
    void ClosePool();

private:
    // 构造函数
    SqlConnPool() = default;
    // 析构函数
    ~SqlConnPool() { ClosePool(); }

    // 最大连接数
    int MAX_CONN_;

    // 连接队列
    std::queue<MYSQL *> connQue_;
    // 互斥锁
    std::mutex mtx_;
    // 信号量
    sem_t semId_;
};

/* 资源在对象构造初始化 资源在对象析构时释放*/
//RAII模式：以构造函数开始，以析构函数结束
class SqlConnRAII {
public:
    //构造函数：从连接池中获取一个MYSQL连接
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool);
        *sql = connpool->GetConn();
        sql_ = *sql;
        connpool_ = connpool;
    }
    
    //析构函数：释放MYSQL连接
    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); }
    }
    
private:
    MYSQL *sql_;
    SqlConnPool* connpool_;
};

#endif // SQLCONNPOOL_H
