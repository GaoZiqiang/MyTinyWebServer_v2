#include "utils/config.h"
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    // 分析命令行
    config* _config = new config();
    printf("config之后\n");

    WebServer server;
    printf("server之后\n");
    server.init(_config);
    printf("server.init之后\n");
    server.sql_pool_init();
    printf("server.sql_pool_init之后\n");
    server.thread_pool_init();
    printf("server.thread_pool_init之后\n");
    server.event_listen();
    server.event_loop();
//    server.thread_pool_destroy();
}