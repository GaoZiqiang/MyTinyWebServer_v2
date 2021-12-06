#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <string>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <time.h>

#include "timer/lst_timer.h"
#include "utils/config.h"
#include "http/http_conn.h"
#include "thread_pool/thread_pool.h"

const int MAX_FD = 65536;// 最大文件描述符
const int MAX_EVENT_NUMS = 10000;// epoll最大监听量--最大事件数
const int TIMESLOT = 5;// 超时时间

using namespace std;

class config;

class WebServer {
public:
    WebServer();
    ~WebServer();

    void init(config* _config);
    void init(int sock_port, string db_user, string db_passwd, string db_name, int log_write,
              int opt_linger, int trig_mode, int sql_num, int thread_num, int close_log,
              int actor_mode);

    void thread_pool();
    void sql_pool();
    void log_write();
    void trig_mode();
    void event_listen();
    void event_loop();
    void timer(int connfd, struct sockaddr_in client_address);
    void adjust_timer(timerNode* timer);
    bool deal_timer(timerNode* timer, int sockfd);
    bool deal_with_newclient();
    bool deal_with_signal(bool& timeout, bool& stop);
    void deal_with_read(int sockfd);
    void deal_with_write(int sockfd);

public:
    // 基础
    int m_port;
    char *m_root;
    int m_log_write;
    int m_close_log;
    int m_actor_mode;

    // 数据库相关
    connection_pool *m_connPool;
    string m_dbuser;         //登陆数据库用户名
    string m_dbpasswd;     //登陆数据库密码
    string m_dbname; //使用数据库名
    int m_sql_num;

    // 线程池相关
//    threadPool *m_thread_pool;
    int m_thread_num;

    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMS];
    int m_epollfd;
    int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIG_mode;
    int m_LISTEN_TRIG_mode;
    int m_CONN_TRIG_mode;
    http_conn *users_http;

    // 定时器相关
    int m_pipefd[2];// ALARM定时器信号传输管道
    client_data *users_timer;
    timerList timer_list;
    signalUtils signal_utils;
};
#endif