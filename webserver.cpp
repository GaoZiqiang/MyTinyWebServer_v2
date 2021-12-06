#include "webserver.h"


//int signalUtils::u_epollfd = 0;
//int *signalUtils::u_pipefd = 0;

WebServer::WebServer() {
    // http_conn类对象
    users_http = new http_conn[MAX_FD];

    // root文件夹路径
    char server_path[200];
    getcwd(server_path, 200);// 将当前工作目录的绝对路径复制到server_path
    char root_path[6] = "/root";
    m_root = (char*)malloc(strlen(server_path) + strlen(root_path) + 1);
    strcpy(m_root, server_path);
    strcat(m_root, root_path);

    // 定时器
    users_timer = new client_data[MAX_FD];
}

WebServer::~WebServer() {
    close(m_epollfd);
    close(m_listenfd);
    close(m_pipefd[0]);
    close(m_pipefd[1]);
    delete[] users_http;
    delete[] users_timer;
//    delete m_thread_pool;
}

void WebServer::init(config* _config) {
    init(_config->port, _config->db_user, _config->db_passwd, _config->db_name, _config->log_write,
         _config->opt_linger, _config->trig_mode, _config->sql_num, _config->thread_num,
         _config->close_log, _config->actor_mode);
}

void WebServer::init(int sock_port, string db_user, string db_passwd, string db_name, int log_write, int opt_linger,
                     int trig_mode, int sql_num, int thread_num, int close_log, int actor_mode) {
    m_port = sock_port;
    m_dbuser = db_user;
    m_dbpasswd = db_passwd;
    m_dbname = db_name;
    m_log_write = log_write;
    m_OPT_LINGER = opt_linger;
    m_TRIG_mode = trig_mode;
    m_sql_num = sql_num;
    m_thread_num = thread_num;
    m_close_log = close_log;
    m_actor_mode = actor_mode;
}

bool WebServer::deal_with_newclient() {
    printf("this is WebServer::deal_with_newclient()\n");
    struct sockaddr_in clnt_addr;
    socklen_t clnt_addrlen = sizeof(clnt_addr);
    // accept new client
    int clientfd = accept(m_listenfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen);
    if (clientfd < 0) {
        LOG_ERROR("new client accept error, errno is %d", errno);
        return false;
    }
//    if (http_conn::m_user_count >= MAX_FD) {
//        LOG_ERROR("%s", "internal server busy");
//        return false;
//    }

//    LOG_INFO("%s", "new client connected");
    printf("新客户端连接成功\n");

    // 1 将clientfd添加到epoll内核空间
    signal_utils.add_fd(m_epollfd, clientfd);

    // 2 填充并添加users_timer数组
    users_timer[clientfd].sockfd = clientfd;
    users_timer[clientfd].address = clnt_addr;

    // 2 构建新timer并添加到timer_list
    timerNode* timer = new timerNode;
    time_t cur_time = time(nullptr);
    timer->expire = cur_time + 3 * TIMESLOT;
    timer->user_data = &users_timer[clientfd];
    timer->cb_func = signalUtils::cb_func;// 放到timerNode的构造函数???

    users_timer[clientfd].timer = timer;// timer
    signal_utils.m_timer_list.add_timer(timer);
//    printf("timerlist.add_timer成功\n");

    return true;
}

bool WebServer::deal_with_signal(bool &timeout, bool &stop) {
//    printf("接收到SIGALRM信号\n");
    int sig;
    char signals[1024];// 收到的信号放到数组中--一个信号一个字节char
    int ret = recv(m_pipefd[0], signals, sizeof(signals), 0);
    if (ret == -1)
    {
        return false;
    }
    else if (ret == 0)
    {
        return false;
    }
    else
    {
        for (int i = 0; i < ret; i++)
        {
            switch(signals[i])
            {
                case SIGALRM:
                    timeout = true;
                    break;

                case SIGTERM:
                    stop = true;// 强制关闭服务器主进程
                    break;
            }
        }// for
    }
//    printf("this is WebServer::deal_with_signal()\n");
//    alarm(TIMESLOT);
    return true;
}

void WebServer::deal_with_read(int sockfd) {
    int clientfd = sockfd;
//    printf("接收到客户端 %d 的数据\n", clientfd);
    // 1 将该client的读事件添加到线程池
    char buf[256];
    int ret = recv(clientfd, buf, 256, 0);
    if (ret < 0) {
        printf("发生读错误\n");
        return;
    } else if (ret == 0) {
        printf("对方已经断开连接\n");
        return;
    } else {
        printf("接收到客户端 %d 的数据: %s", clientfd, buf);
    }
    // 2 调整timer
    timerNode* tmp_timer = users_timer[clientfd].timer;
    if (tmp_timer) {
        time_t cur_time = time(nullptr);
        tmp_timer->expire = cur_time + 3 * TIMESLOT;
        signal_utils.m_timer_list.adjust_timer(tmp_timer);
    }

//    printf("this is WebServer::deal_with_read()\n");
    return ;
}

void WebServer::deal_with_write(int sockfd) {
    printf("this is WebServer::deal_with_write()\n");
    return ;
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
    ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd);
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

    // signalUitls静态类成员变量初始化
    signalUtils::u_epollfd = m_epollfd;
    signalUtils::u_pipefd = m_pipefd;
}

void WebServer::event_loop() {
    bool timeout = false;
    bool stop = false;


//    m_thread_pool = new threadPool();
//    m_thread_pool->thread_init(stop);
    threadPool thread_pool;
//    thread_pool.thread_init(stop);
//    threadPool::thread_init(stop);
    printf("threadPool::accept_id = %d\n", threadPool::accept_id);


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
                printf("new client连接\n");
//                pthread_cond_signal(&threadPool::accept_cond);
//                bool ret = deal_with_newclient();
//                if (false == ret)// new client连接失败
//                    continue;
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                // 断开服务器连接，移除timer定时器
                timerNode* timer = users_timer[sockfd].timer;
                // 封装一个remove_client()函数
                signal_utils.cb_func(&users_timer[sockfd]);// 将闹钟绑定的客户端的sockfd从epoll空间中移除
                if (nullptr != timer) {
                    timer_list.del_timer(timer);
                }
                // ALARM signal处理
            } else if ((sockfd == m_pipefd[0]) && (events[i].events & EPOLLIN)) {
                ::pthread_cond_signal(&threadPool::accept_cond);
                printf("threadPool::accept_cond: %d\n", threadPool::accept_cond);
                alarm(TIMESLOT);
//                bool ret = deal_with_signal(timeout, stop);// 两个传出参数
//                if (false == ret) {
//                    printf("%s", "deal alarm signal fail");
//                    LOG_ERROR("%s", "deal alarm signal fail");
//                }
//                printf("deal_with_signal 处理完毕\n");
            } else if (events[i].events & EPOLLIN) {// read数据处理
//                deal_with_read(sockfd);
                pthread_mutex_lock(&threadPool::client_mutex);
                threadPool::client_list.push_back(sockfd);
                pthread_mutex_unlock(&threadPool::client_mutex);
                pthread_cond_signal(&threadPool::client_cond);

            } else if (events[i].events & EPOLLOUT) {// write数据处理
                deal_with_write(sockfd);
            }
        }// for
        // timeout处理
        if (timeout) {
            signal_utils.timer_handler();
//            LOG_INFO("%s", "timer tick");
            timeout = false;
        }
    }// while
}// event_loop
