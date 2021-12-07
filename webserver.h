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
    static bool deal_with_newclient();
    bool deal_with_signal(bool& timeout, bool& stop);
    static void deal_with_read(int sockfd);
    void deal_with_write(int sockfd);
    void test() {
        printf("this is a test func in webserver.h\n");
    }

public:
    // 基础
    int m_port;
    static char *m_root;
    int m_log_write;
    static int m_close_log;
    int m_actor_mode;

    // 数据库相关
    static connection_pool *m_connPool;
    static string m_dbuser;         //登陆数据库用户名
    static string m_dbpasswd;     //登陆数据库密码
    static string m_dbname; //使用数据库名
    static int m_sql_num;

    // 线程池相关
//    threadPool *m_thread_pool;
    int m_thread_num;

    // epoll_event相关
    epoll_event events[MAX_EVENT_NUMS];
    static int m_epollfd;
    static int m_listenfd;
    int m_OPT_LINGER;
    int m_TRIG_mode;
    int m_LISTEN_TRIG_mode;
    static int m_CONN_TRIG_mode;
    static http_conn *users_http;

    // 定时器相关
    int m_pipefd[2];// ALARM定时器信号传输管道
    static client_data *users_timer;
    static timerList timer_list;
    static signalUtils signal_utils;
};
#endif