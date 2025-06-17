#include <iostream>
#include "init.h"
#include "config/configs.h"
#include "log_utils.h"

int main(int argc, char* argv[]) {

    std::string username = "webuser";
    std::string password = "webpwd";
    std::string databasename = "littlewebserver";

    // 从命令行解析配置
    ServerConfig config;
    config.parseArgs(argc, argv);

    initAllModules(config, username, password, databasename);


    LOG_INFO("MiniAiServer 启动！");
    return 0;
}