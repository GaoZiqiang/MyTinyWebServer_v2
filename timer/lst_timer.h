#ifndef LST_TIMER_H
#define LST_TIMER_H

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>



// 前置声明
class timerNode;

// 用户数据:客户端地址、文件描述符sockfd、读缓存和定时器
struct client_data {
    sockaddr_in address;
    int sockfd;
//    char buf[];// 保留--存放用户接收发送的数据
    timerNode* timer;
};

// 定时器类--链表元素/链表单元--链表节点的存储结构
class timerNode {
public:
    timerNode() : prev(nullptr), next(nullptr) {}

public:
    // 存储结构内容
    // 1 闹钟本身信息--超时时间、处理函数
    time_t expire;
    void (*cb_func) (client_data*, int);// 待修改
    // 2 与闹钟绑定的客户数据
    client_data* user_data;
    // 3 节点前后指针
    timerNode* prev;
    timerNode* next;
};

class timerList {
public:
    timerList();
    ~timerList();

    void add_timer(timerNode* timer);
    void adjust_timer(timerNode* timer);
    void del_timer(timerNode* timer);
    // 脉搏函数--每次收到SIGALRM信号便触发一次
    void tick(int epollfd);// 待修改

private:
    // 链表的头指针和尾指针
    timerNode* head;
    timerNode* tail;

    // 重载--将timer添加到last_timer之后--供add_timer()和adjust_timer()使用
    void add_timer(timerNode* timer, timerNode* last_timer);
};

class signalUtils {
public:
    signalUtils();
    ~signalUtils();

    void init(int timeslot);
//    static void get_pipefd();
    int set_non_blocking(int fd);
    //将内核事件表注册读事件，ET模式，选择开启EPOLLONESHOT
//    void addfd(int epollfd, int sockfd, bool one_shot, int TRIGMode);
    void add_fd(int epollfd, int sockfd);
    // 信号处理函数
    static void sig_handler(int sig);
    // 添加信号，绑定信号处理函数
    void add_sig(int sig, void(handler)(int), bool restart = true);// 改进版
    // 定时处理任务
    void timer_handler();// 改
//    static void cb_func(client_data* user_data, int epollfd);// 改 epollfd不用传参数了
    static void cb_func(client_data* user_data);

    void show_error(int connfd, const char* info);

public:
    static int* u_pipefd;
    static int u_epollfd;// 全局唯一，不用通过参数传递了
    int m_TIMESLOT;
    timerList m_timer_list;
};
#endif