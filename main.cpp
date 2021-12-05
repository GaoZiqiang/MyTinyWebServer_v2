#include "utils/config.h"
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    string user = "root";
    string passwd = "";
    string dbname = "tinywebserver";

    // 分析命令行
    config config;
    config.parse_arg(argc, argv);

//    config.init();
    WebServer server;

//    server.init(config.port)


}