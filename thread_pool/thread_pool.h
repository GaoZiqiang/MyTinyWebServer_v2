#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <semaphore.h>
#include <list>
#include <stdio.h>
#include "../webserver.h"
#include "../http/http_conn.h"
#include "../mysql/sql_connection_pool.h"

const int WORKER_THREAD_NUM = 3;

class threadPool {
public:
    threadPool();
    ~threadPool() {};
    static connection_pool* m_conn_pool;// mysql数据库

    static void thread_init(bool stop);
    static void thread_destroy();

    static void* accept_thread_func(void* arg);
    static void* worker_thread_func(void* arg);

public:
    // accpet线程id
    static pthread_t accept_id;
    // worker线程id
    static pthread_t worker_ids[WORKER_THREAD_NUM];

    static pthread_mutex_t accept_mutex;// 互斥信号量
    static pthread_mutex_t client_mutex;

    static pthread_cond_t accept_cond;// 条件信号量
    static pthread_cond_t client_cond;

//    static std::list<int> client_list;// client列表
    static std::list<http_conn*> client_list;

};
#endif