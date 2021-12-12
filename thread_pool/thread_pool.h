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
extern pthread_t accept_id;// accpet线程id
extern pthread_t worker_ids[WORKER_THREAD_NUM];// worker线程id

extern pthread_mutex_t accept_mutex;// 互斥信号量
extern pthread_mutex_t client_mutex;

extern pthread_cond_t accept_cond;// 条件信号量
extern pthread_cond_t client_cond;

extern std::list<http_conn*> client_list;

//class WebServer;

class threadPool {
public:
    threadPool();
    ~threadPool();
    static connection_pool* m_conn_pool;// mysql数据库

//    void thread_init();
//    void thread_destroy();

//    WebServer* webserver;

//    static void* accept_thread_func(void* arg);
//    static void* worker_thread_func(void* arg);

public:


};
#endif