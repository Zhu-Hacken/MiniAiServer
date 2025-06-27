#include <iostream>
#include "init.h"
#include "config/configs.h"
#include "log_utils.h"
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>
#include "net/net_server.h"

const std::string BASE_TEXT = "[MiniAiServer] ";

int main(int argc, char* argv[]) {

    std::string username = "webuser";
    std::string password = "webpwd";
    std::string databasename = "littlewebserver";

    // 从命令行解析配置
    ServerConfig config;
    config.parseArgs(argc, argv);

    initAllModules(config, username, password, databasename);



    LOG_INFO(BASE_TEXT + "MiniAiServer 启动！");

    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test_env");

    LOG_INFO(BASE_TEXT + "ONNX Runtime 环境初始化成功！");

    // NetServer server(config);
    NetServer::getInstance().init(config, username, password, databasename);
    NetServer::getInstance().run();
    // server.init(username, password, databasename);
    // server.run();

    return 0;
}