#ifndef CONFIG_H
#define CONFIG_H

#include "../webserver.h"
#include <string>
#include <iostream>

using namespace std;

class config {
public:
    config();
    ~config();

    void parse_arg(int argc, char* argv[]);
//    void init(int sock_port, string db_user, string db_passwd, string db_name, int log_write,
//              int opt_linger, int trig_mode, int sql_num, int thread_num, int close_log,
//              int actor_mode);
public:
    // 基础
    int port;
    int log_write;// 日志写入方式
    int trig_mode;// 触发组合模式
    int close_log;// 是否关闭日志

    // 数据库相关
    string db_user;
    string db_passwd;
    string db_name;
    int sql_num;

    // 线程池相关
    int thread_num;

    // epoll event相关
    int listen_trig_mode;// listenfd触发模式
    int conn_trig_mode;// connfd触发模式
    int opt_linger;// 优雅断开连接
    int actor_mode;// 事件处理模式
};

#endif