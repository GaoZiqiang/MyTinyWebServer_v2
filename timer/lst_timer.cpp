#include "lst_timer.h"

timerList::timerList() {
    head = nullptr;
    tail = nullptr;
}

timerList::~timerList() {
    // 释放timer链表
    timerNode* tmp = head;
    while (tmp) {
        head = tmp->next;
        delete tmp;
        tmp = head;
    }
}

void timerList::add_timer(timerNode *timer) {
    if (!timer) {
        return;
    }
    if (!head) {
        head = tail = timer;
        return;
    }
    // 直接插到head之前
    if (timer->expire < head->expire) {
        timer->next = head;
        head->prev = timer;
        head = timer;
        return;
    }
    // 插入排序，找到head之后第一个比timer->expire大的节点
    add_timer(timer, head);
}

// 插入排序，找到head之后第一个比timer->expire大的节点
void timerList::add_timer(timerNode *timer, timerNode *last_timer) {
    timerNode* tmp_prev = last_timer;
    timerNode* tmp = last_timer->next;
    while (tmp) {
        // 找到第一个比timer->expire大的节点
        if (timer->expire < tmp->expire) {
            tmp_prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = tmp_prev;
            break;
        }
        tmp_prev = tmp;
        tmp = tmp->next;
    }

    // 遍历完整个链表仍然没有找到满足tmp->expire > timer->expire的节点--将timer插入到链表尾
    if (!tmp) {
        tmp_prev->next = timer;
        timer->prev = tmp_prev;
        timer->next = nullptr;
        tail = timer;
    }
}

// 某个定时器任务发生变化时，调整该定时器的expire，然后将该timer_node向链表尾部移动
void timerList::adjust_timer(timerNode *timer) {
    if (!timer)
        return;

    timerNode* tmp = timer->next;
    // 若timer位于链表尾部或经增大后的timer->expire仍然小于timer->next->expire--则不调整，仍留在原位置
    if (!tmp || timer->expire < tmp->expire)
        return;

    // 若timer为头结点，则将该timer从链表中取出，从并重新插入链表
    if (timer == head) {
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        // 重新使用add_timer()将timer插入到链表合适的位置
        add_timer(timer, head);
    }
    // 若timer不是头结点，则从原链表中取出，并插入到tmp之后的节点
    else {
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        add_timer(timer, timer->next);
    }
}

// 将定时器timer从链表中删除
void timerList::del_timer(timerNode *timer) {
    if (!timer)
        return;

    // 链表中只有一个定时器timer
    if (timer == head && timer == tail) {
        delete timer;
        timer = nullptr;
        head = nullptr;
        tail = nullptr;
        return;
    }

    // timer为链表头结点
    if (timer == head) {
        head = head->next;
        head->prev = nullptr;
        delete timer;
        timer = nullptr;
        return;
    }

    // timer为尾节点
    if (timer == tail) {
        tail = timer->prev;
        tail->next = nullptr;
        delete timer;
        timer = nullptr;
        return;
    }

    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
    timer = nullptr;

    return;
}

// 脉搏函数--每次收到SIGALRM信号便触发一次
// 1 链表中没有timer
// 2 链表中有timer--查看是否有超时timer--删除之--并触发SIGALRM信号处理函数
void timerList::tick() {
    // 链表为空，当前没有客户端连接
    if (nullptr == head) {
        printf("链表为空，当前没有客户端连接\n");
//        LOG_INFO("%s", "no client in connection now");
        return;
    }
//    LOG_INFO("%s", "timer tick");
    printf("timer tick\n");
    time_t cur_time = time(nullptr);// 获取系统当前时间
    timerNode* tmp = head;
    // 从头结点开始依次遍历每个timer，直到找到一个超时的timer--cur_timer > timer->expire--实际只看表头结点即可
    while (tmp) {
        // 未超时--其后的节点都不会超时
        if (cur_time < tmp->expire)
            break;
        // 超时--执行SIGALRM信号处理函数--关闭sockfd，并从epollfd例程空间中删除该sockfd
        tmp->cb_func(tmp->user_data);
        // 执行完任务后，经其从timer_list中删除
        head = tmp->next;
        if (head)
            head->prev = nullptr;
        delete tmp;
        printf("剔除一个timer\n");
        tmp = head;// 继续遍历下一个
    }
}

// 先声明一下？
int signalUtils::u_epollfd = 0;
int *signalUtils::u_pipefd = 0;

void signalUtils::init(int timeslot) {
    m_TIMESLOT = timeslot;
}

// 对文件描述符设置非阻塞
int signalUtils::set_non_blocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    int ret = fcntl(fd, F_SETFL, new_option);
    assert(ret != -1);
    return 0;
}

void signalUtils::add_fd(int epollfd, int sockfd) {
    epoll_event event;
    event.data.fd = sockfd;
    // epoll event的触发模式暂时不区分
    event.events = EPOLLIN | EPOLLRDHUP;// trigmode = 0
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &event);
    set_non_blocking(sockfd);
}

// 信号处理函数--将alarm信号通过管道发送给epoll内核空间
void signalUtils::sig_handler(int sig) {
    int old_errno = errno;
    int msg = sig;
    send(signalUtils::u_pipefd[1], (char*)&msg, 1, 0);// 将alarm信号通过管道发送给epoll内核空间
    errno = old_errno;
}

// 设置信号函数
void signalUtils::add_sig(int sig, void (*handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart)
        sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    int ret = sigaction(sig, &sa, nullptr);
    assert(ret != -1);
}

// 定时处理任务
void signalUtils::timer_handler() {
//    printf("in signalUtils::timer_handler()\n");
    m_timer_list.tick();
//    printf("after tick()\n");
    alarm(m_TIMESLOT);// 重启定时器
//    printf("after alarm(m_TIMESLOT)\n");
}

void signalUtils::cb_func(client_data *user_data) {
    // 将sockfd从epoll内核空间中移除
    epoll_ctl(u_epollfd, EPOLL_CTL_DEL, user_data->sockfd, 0);
    assert(user_data);
    close(user_data->sockfd);
    // http连接池中user count --
//    http_conn::m_user_count--;
//    LOG_INFO("%s", "close a client");
}