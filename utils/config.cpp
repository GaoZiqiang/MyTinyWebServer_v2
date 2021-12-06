#include "config.h"

config::config(){
    port = 9999;// socket访问端口
    log_write = 0;// 日志写入方式，默认同步
    trig_mode = 0;// 触发组合模式,默认listenfd LT + connfd LT
    close_log = 0;// 关闭日志，默认不关闭

    db_user = "root";
    db_passwd = "";
    db_name = "tinywebserver";
    sql_num = 8;// 数据库连接池数量,默认8

    thread_num = 8;// 线程池内的线程数量,默认8

    listen_trig_mode = 0;// listenfd触发模式，默认LT
    conn_trig_mode = 0;// connfd触发模式，默认LT

    opt_linger = 0;// 优雅关闭链接，默认不使用
    actor_mode = 0;// 并发模型,默认是proactor--改
}

void config::parse_arg(int argc, char*argv[]){
    cout << "this is config::parse_arg()" << endl;
//    int opt;
//    const char *str = "p:l:m:o:s:t:c:a:";
//    while ((opt = getopt(argc, argv, str)) != -1)
//    {
//        switch (opt)
//        {
//            case 'p':
//            {
//                PORT = atoi(optarg);
//                break;
//            }
//            case 'l':
//            {
//                LOGWrite = atoi(optarg);
//                break;
//            }
//            case 'm':
//            {
//                TRIGMode = atoi(optarg);
//                break;
//            }
//            case 'o':
//            {
//                OPT_LINGER = atoi(optarg);
//                break;
//            }
//            case 's':
//            {
//                sql_num = atoi(optarg);
//                break;
//            }
//            case 't':
//            {
//                thread_num = atoi(optarg);
//                break;
//            }
//            case 'c':
//            {
//                close_log = atoi(optarg);
//                break;
//            }
//            case 'a':
//            {
//                actor_model = atoi(optarg);
//                break;
//            }
//            default:
//                break;
//        }
//    }
}