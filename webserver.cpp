#include "webserver.h"

void WebServer::init(config* config) {
    init(config->PORT, config->user, config->passwd, config->dbname, config->LOGWrite,
         config->OPT_LINGER, config->TRIGMode,  config->sql_num,  config->thread_num,
         config->close_log, config->actor_model);
}

void WebServer::event_listen() {
    m_listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(m_listenfd >= 0);

    // 优雅的关闭TCP连接
    if (0 == m_OPT_LINGER) {
        // 缺省方式关闭连接，直接丢弃残留数据
        struct linger tmp_lin = {0, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp_lin, sizeof(tmp_lin));
    } else if (1 == m_OPT_LINGER) {
        // 关闭方先等待一个超时时间，若残留数据仍然未发送完毕，直接丢弃残留数据并关闭TCP连接
        struct linger tmp_lin = {1, 1};
        setsockopt(m_listenfd, SOL_SOCKET, SO_LINGER, &tmp_lin, sizeof(tmp_lin));
    }

    // 设置socket属性信息
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(m_port);

    int flag = 1;
    // 设置端口可重用
    setsockopt(m_listenfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    // bind
    ret = bind(m_listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret >= 0);
    // listen
    ret = listen(m_listenfd, 5);
    assert(ret >= 0);

    signal_utils.init(TIMESLOT);

    // 创建epoll内核空间
    epoll_event events[MAX_EVENT_NUMS];
    m_epollfd = epoll_create(5);
    assert(m_epollfd != -1);

    // 将m_listenfd添加到epoll空间
    signal_utils.add_fd(m_epollfd, m_listenfd);
//    http_conn::m_epollfd = m_epollfd;

    // 创建ALARM定时器信号传输管道
    ret = socketpair(PF_UNIX, SOCKET_STREAM, 0, m_pipef);
    assert(ret != -1);
    // 设置管道写端非阻塞
    signal_utils.set_non_blocking(m_pipefd[1]);
    // 将管道读端添加到epoll空间
    signal_utils.add_fd(m_epollfd, m_pipefd[0]);

    // 绑定信号处理函数
    signal_utils.add_sig(SIGPIPE, SIG_IGN);
    signal_utils.add_sig(SIGALRM, signal_utils.sig_handler, false);
    signal_utils.add_sig(SIGTERM, signal_utils.sig_handler, false);

    // 启动定时器
    alarm(TIMESLOT);

    // signalUitls类成员变量初始化
    signalUtils::u_epollfd = m_epollfd;
    signalUtils::u_pipefd = m_pipefd;
}

void WebServer::event_loop() {
    bool timeout = false;
    bool stop = false;

    while (!stop) {
        // epoll_wait
        int ready_num = epoll_wait(m_epollfd, events, MAX_EVENT_NUMS, -1);// 无限等待
        if (ready_num < 0 && errno != EINTR) {
            LOG_ERROR("%s", "epoll fail");
            break;
        }
        // 返回值处理
        for (int i = 0; i < ready_num; i++) {
            // events处理
            int sockfd = events[i].data.fd;// 事件sockfd
            // new client处理
            if (sockfd == m_listenfd) {
                bool ret = deal_with_newclient();
                if (false == ret)// new client连接失败
                    continue;
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 断开服务器连接，移除timer定时器
                timerNode* timer = users_timer[sockfd].timer;
                // 封装一个remove_client()函数
                signal_utils.cb_func(timer);// 将闹钟绑定的客户端的sockfd从epoll空间中移除
                if (nullptr != timer) {
                    timer_list.del_timer(timer);
                }
            // ALARM signal处理
            } else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) {
                bool ret = deal_with_signal(timeout, stop);
                if (false == ret)
                    LOG_ERROR("%s", "deal alarm signal fail");
            } else if (events[i].events & EPOLLIN) {// read数据处理
                deal_with_read();

            } else if (events[i].events & EPOLLOUT) {// write数据处理
                deal_with_write();
            }
        }// for
        // timeout处理
        if (timeout) {
            signal_utils.timer_handler();
            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }// while
}// event_loop
