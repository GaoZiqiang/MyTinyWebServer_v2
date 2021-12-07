#include "thread_pool.h"

pthread_t threadPool::accept_id;
pthread_t threadPool::worker_ids[WORKER_THREAD_NUM];
pthread_mutex_t threadPool::accept_mutex;
pthread_mutex_t threadPool::client_mutex;
pthread_cond_t threadPool::accept_cond;
pthread_cond_t threadPool::client_cond;
std::list<int> threadPool::client_list;

threadPool::threadPool() {
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
//    printf("in threadPool::accept_thread_func(void *arg)\n");
    bool stop = (bool*)arg;
    while (true) {
        ::pthread_mutex_lock(&threadPool::accept_mutex);
        ::pthread_cond_wait(&threadPool::accept_cond, &threadPool::accept_mutex);
        // 接收新客户端
        printf("this is threadPool::accept_thread_func\n");
        ::pthread_mutex_unlock(&threadPool::accept_mutex);
    }
    return nullptr;
}

void* threadPool::worker_thread_func(void *arg) {
//    printf("in threadPool::worker_thread_func(void *arg)\n");
    bool stop = (bool*)arg;
    while (true) {
        ::pthread_mutex_lock(&threadPool::client_mutex);
        while (threadPool::client_list.empty())
            ::pthread_cond_wait(&threadPool::client_cond, &threadPool::client_mutex);
        int clientfd = threadPool::client_list.front();// 取出队首clientfd
        threadPool::client_list.pop_front();
        ::pthread_mutex_unlock(&threadPool::client_mutex);
        // 处理客户端发来的数据
        printf("this is threadPool::worker_thread_func, now deal with worker %d\n", clientfd);
    }
    return nullptr;
}