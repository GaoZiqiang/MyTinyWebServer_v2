#include "thread_pool.h"

class connectionRAII;

connection_pool* threadPool::m_conn_pool;
pthread_t threadPool::accept_id;
pthread_t threadPool::worker_ids[WORKER_THREAD_NUM];
pthread_mutex_t threadPool::accept_mutex;
pthread_mutex_t threadPool::client_mutex;
pthread_cond_t threadPool::accept_cond;
pthread_cond_t threadPool::client_cond;
//std::list<int> threadPool::client_list;
std::list<http_conn*> threadPool::client_list;

threadPool::threadPool() {
    printf("in threadPool::threadPool()\n");
    m_conn_pool = WebServer::m_connPool;
//    connection_pool* conn;
//    m_conn_pool = connection_pool::GetInstance();
//    m_conn_pool->init("127.0.0.1", WebServer::m_dbuser, WebServer::m_dbpasswd, WebServer::m_dbname,
//                      3306, WebServer::m_sql_num, WebServer::m_close_log);

    accept_id = 0;
    worker_ids[WORKER_THREAD_NUM] = {0};

    pthread_mutex_init(&accept_mutex, nullptr);
    pthread_cond_init(&accept_cond, nullptr);
//    printf("in threadPool::threadPool(), pthreadPool::accept_cond = %d\n", accept_cond);

    pthread_mutex_init(&client_mutex, nullptr);
    pthread_cond_init(&client_cond, nullptr);
//    printf("in threadPool::threadPool(), pthreadPool::client_cond = %d\n", client_cond);

    bool stop = false;
    ::pthread_create(&threadPool::accept_id, nullptr, accept_thread_func, (void*)&stop);
//    printf("in threadPool::threadPool(), pthreadPool::accept_id = %d\n", accept_id);
    for (int i = 0; i < WORKER_THREAD_NUM; i++) {
//        printf("创建线程 %d\n", i + 1);
        ::pthread_create(&threadPool::worker_ids[i], nullptr, worker_thread_func, (void*)&stop);
    }
}

//threadPool::~threadPool() {
////    pthread_mutex_destroy(&accept_mutex);
////    pthread_cond_destroy(&accept_cond);
////    pthread_mutex_destroy(&client_mutex);
////    pthread_cond_destroy(&client_cond);
//}

void threadPool::thread_init(bool stop) {
    accept_id = 0;
    worker_ids[WORKER_THREAD_NUM] = {0};

    pthread_mutex_init(&accept_mutex, nullptr);
    pthread_cond_init(&accept_cond, nullptr);
//    printf("in threadPool::threadPool(), pthreadPool::accept_cond = %d\n", accept_cond);

    pthread_mutex_init(&client_mutex, nullptr);
    pthread_cond_init(&client_cond, nullptr);
//    printf("in threadPool::threadPool(), pthreadPool::client_cond = %d\n", client_cond);

//    bool stop = false;
    ::pthread_create(&threadPool::accept_id, nullptr, accept_thread_func, (void*)&stop);
//    printf("in threadPool::threadPool(), pthreadPool::accept_id = %d\n", accept_id);
    for (int i = 0; i < WORKER_THREAD_NUM; i++) {
//        printf("创建线程 %d\n", i + 1);
        ::pthread_create(&threadPool::worker_ids[i], nullptr, worker_thread_func, (void*)&stop);
    }




//    printf("in threadPool::thread_init\n");
//    threadPool::accept_id = 0;
//    printf("in threadPool::thread_init, pthreadPool::accept_id = %d\n", accept_id);
//    threadPool::worker_ids[WORKER_THREAD_NUM] = {0};
//
//    ::pthread_mutex_init(&threadPool::accept_mutex, nullptr);
//    ::pthread_cond_init(&threadPool::accept_cond, nullptr);
//
//    ::pthread_mutex_init(&threadPool::client_mutex, nullptr);
//    ::pthread_cond_init(&threadPool::client_cond, nullptr);
//
//    ::pthread_create(&threadPool::accept_id, nullptr, accept_thread_func, (void*)&stop);
//    for (int i = 0; i < WORKER_THREAD_NUM; i++) {
//        printf("创建线程 %d\n", i + 1);
//        ::pthread_create(&threadPool::worker_ids[i], nullptr, worker_thread_func, (void*)&stop);
//    }
//    printf("in threadPool::thread_init, accept_cond: %d\n", threadPool::accept_cond);
}

void threadPool::thread_destroy() {
    ::pthread_mutex_destroy(&accept_mutex);
    ::pthread_cond_destroy(&accept_cond);
    ::pthread_mutex_destroy(&client_mutex);
    ::pthread_cond_destroy(&client_cond);
}

void* threadPool::accept_thread_func(void *arg) {
    bool b_OK = (bool*)arg;
    while (b_OK) {
        ::pthread_mutex_lock(&threadPool::accept_mutex);
        ::pthread_cond_wait(&threadPool::accept_cond, &threadPool::accept_mutex);
        ::pthread_mutex_unlock(&threadPool::accept_mutex);
        // 接收新客户端
        printf("this is threadPool::accept_thread_func\n");

        WebServer::deal_with_newclient();
    }
    return nullptr;
}

void* threadPool::worker_thread_func(void *arg) {
//    printf("in threadPool::worker_thread_func(void *arg)\n");
    bool b_OK = (bool*)arg;
    while (b_OK) {
        ::pthread_mutex_lock(&threadPool::client_mutex);
        while (threadPool::client_list.empty())
            ::pthread_cond_wait(&threadPool::client_cond, &threadPool::client_mutex);
//        int clientfd = threadPool::client_list.front();// 取出队首clientfd
        http_conn* request = threadPool::client_list.front();// 取出队首clientfd
        threadPool::client_list.pop_front();
        ::pthread_mutex_unlock(&threadPool::client_mutex);
        // 处理客户端发来的数据
//        printf("this is threadPool::worker_thread_func, now deal with client %d\n", clientfd);
        printf("this is threadPool::worker_thread_func, now deal with client\n");
//        WebServer::deal_with_read(clientfd);// 改
        if (0 == request->m_state) {
            printf("in thread_pool, 进行读\n");
            printf("in thread_pool, sockfd = %d\n", request->m_sockfd);
//            char buf[256];
//            read(request->m_sockfd, buf, 256);
//            printf("read from client %d: %s\n", request->m_sockfd, buf);
            if (request->read_once()) {
                printf("in thread_pool, after request->read_once()\n");
                request->improv = 1;
                connectionRAII mysql_con(&request->mysql, WebServer::m_connPool);
                printf("in thread_pool, after connectionRAII mysql_con\n");
                request->process();
                printf("in thread_pool, after request->process(), request->process成功\n");
            } else {
                printf("in thread_pool, read失败\n");
                request->improv = 1;
                request->timer_flag = 1;
            }
        } else {
            printf("in thread_pool, 进行写\n");
            if (request->write()) {
                request->improv = 1;
            } else {
                request->improv = 1;
                request->timer_flag = 1;
            }
        }

    }
    return nullptr;
}