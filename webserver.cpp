#include "webserver.h"


//int signalUtils::u_epollfd = 0;
//int *signalUtils::u_pipefd = 0;
int WebServer::clientfd;
sockaddr_in WebServer::clnt_addr;

int WebServer::m_listenfd;
//int WebServer::m_epollfd;
//char* WebServer::m_root;
//int WebServer::m_CONN_TRIG_mode;
//int WebServer::m_close_log;
//string WebServer::m_dbuser;
//string WebServer::m_dbname;
//string WebServer::m_dbpasswd;
//int WebServer::m_sql_num;
connection_pool* WebServer::m_connPool;
//
//client_data* WebServer::users_timer;
//timerList WebServer::timer_list;
//signalUtils WebServer::signal_utils;
//http_conn* WebServer::users_http;

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
//    m_thread_pool->thread_destroy();
    delete m_thread_pool;
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

void WebServer::sql_pool_init()
{
    //初始化数据库连接池
    m_connPool = connection_pool::GetInstance();

    m_connPool->init("127.0.0.1", m_dbuser, m_dbpasswd, m_dbname, 3306, m_sql_num, m_close_log);

    //初始化数据库读取表
    users_http->initmysql_result(m_connPool);
}

void WebServer::thread_pool_init() {
    m_thread_pool = new threadPool();
//    m_thread_pool->thread_init();
}

//void WebServer::thread_pool_destroy() {
//    m_thread_pool->thread_destroy();
//}

void WebServer::deal_with_newclient() {
    ::pthread_cond_signal(&accept_cond);

    // 1 将clientfd添加到epoll内核空间
    signal_utils.add_fd(m_epollfd, clientfd);

    // 2 填充并添加users_timer数组
    users_timer[clientfd].sockfd = clientfd;
    users_timer[clientfd].address = clnt_addr;

    // 3 构建新timer并添加到timer_list
    timerNode* timer = new timerNode;
    time_t cur_time = time(nullptr);
    timer->expire = cur_time + 3 * TIMESLOT;
    timer->user_data = &users_timer[clientfd];
    timer->cb_func = signalUtils::cb_func;

    users_timer[clientfd].timer = timer;// timer
    signal_utils.m_timer_list.add_timer(timer);
    // 4 users_http配置
    users_http[clientfd].init(clientfd, clnt_addr, m_root, m_CONN_TRIG_mode, m_close_log, m_dbuser,
                              m_dbpasswd, m_dbname);
}

void WebServer::add_newclient() {
//    struct sockaddr_in clnt_addr;
    socklen_t clnt_addrlen = sizeof(clnt_addr);
    // accept new client
    clientfd = accept(m_listenfd, (struct sockaddr*)&clnt_addr, &clnt_addrlen);
    if (clientfd < 0) {
//        LOG_ERROR("new client accept error, errno is %d", errno);
        return;
    }
    if (http_conn::m_user_count >= MAX_FD) {
//        LOG_ERROR("%s", "internal server busy");
        printf("internal server busy\n");
        return;
    }

//    LOG_INFO("%s", "new client connected");
    printf("新客户端连接成功\n");

    return;
}

bool WebServer::deal_with_signal(bool &timeout, bool &stop) {
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
    return true;
}

void WebServer::deal_with_read(int sockfd) {
    // 1 唤醒worker线程
    ::pthread_mutex_lock(&client_mutex);
    users_http[sockfd].m_state = 0;// read
    client_list.push_back(users_http + sockfd);// 传递sockfd
    ::pthread_mutex_unlock(&client_mutex);
    ::pthread_cond_signal(&client_cond);
    while (true) {
        if (1 == users_http[sockfd].improv) {
            if(1 == users_http[sockfd].timer_flag) {
                users_http[sockfd].timer_flag = 0;
            }
            users_http[sockfd].improv = 0;
            break;
        }
    }

    // 2 调整timer
    timerNode* tmp_timer = users_timer[sockfd].timer;// 此处sockfd为clientfd
    if (tmp_timer) {
        time_t cur_time = time(nullptr);
        tmp_timer->expire = cur_time + 3 * TIMESLOT;
        signal_utils.m_timer_list.adjust_timer(tmp_timer);
    }
    return;
}

void WebServer::deal_with_write(int sockfd) {
    ::pthread_mutex_lock(&client_mutex);
    users_http[sockfd].m_state = 1;
    client_list.push_back(users_http + sockfd);
    ::pthread_mutex_unlock(&client_mutex);
    ::pthread_cond_signal(&client_cond);
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
    http_conn::m_epollfd = m_epollfd;// 同步http_conn的epollfd

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
                printf("new client %d 连接\n", events[i].data.fd);
                deal_with_newclient();
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
                bool ret = deal_with_signal(timeout, stop);// 两个传出参数
                if (false == ret) {
                    printf("%s", "deal alarm signal fail");
//                    LOG_ERROR("%s", "deal alarm signal fail");
                }
            } else if (events[i].events & EPOLLIN) {// read数据处理
                printf("发生读事件, client %d 发来数据\n", sockfd);
                deal_with_read(sockfd);
            } else if (events[i].events & EPOLLOUT) {// write数据处理
                printf("发生写事件, 发送数据给client %d\n", sockfd);
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
