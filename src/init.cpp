#include "init.h"
#include "ai/phi3/phi3_engine_pool.h"
#include "config_manager.h"
#include "log/logs.h"
#include "db/sql_connection_pool.h"
#include "conn/conn_factory_manager.h"

#include "log_utils.h"
#include "mvc/controller/chat_controller.h"
#include "mvc/controller/test_controller.h"
#include "mvc/controller/user_controller.h"
#include "mvc/controller/test_tx_controller.h"
#include "websocket_conn.h"

const std::string BASE_TEXT = "[Init] ";


// === 初始化热更新 ===
void initConfigManager() {
    ConfigManager::getInstace().init(true, ConfigManager::getInstace().getPath());
    LOG_INFO(BASE_TEXT + ConfigManager::getInstace().get("log_close"));

    ConfigManager::getInstace().registerCallback("log_close", [](){
        bool enabled = !ConfigManager::getInstace().getBool("log_close", false);
        LOG_INFO("enabled == " + std::to_string(enabled));
    });
}

// === 初始化日志 ===
void initLog(ServerConfig config) {
    Log::getInstance().init(Log::DEBUG, "server_log", !config.log_close);
}


// === 初始化路由 ===
void registerAllRoutesImpl() {
        TestController::registerRoutes();
        UserController::registerRoutes();
        TestTxController::registerRoutes();
        ChatController::registerRoutes();
}

void initRouter() {
    RouterConfig::setRouteRegisterFunc(registerAllRoutesImpl);
    RouterConfig::registerAllRoutes();
}

// === 初始化频率限制器 ===
void registerAllRateLimiterImpl() {
    // 防止单个 IP 请求爆刷
    RateLimiter::getInstance().registerLimitRule("access_ip", 20, 10);
    // // 登录失败限流（防爆破登录）
    RateLimiter::getInstance().registerLimitRule("login_ip", 5, 180);
}

void initRateLimiter(ServerConfig config) {
    RateLimiter::getInstance().init(config.rate_limiter_close);
    RateLimiterConfig::setRateLimiterFunc(registerAllRateLimiterImpl);
    if ( !RateLimiter::getInstance().isLimiterClose()) RateLimiterConfig::registerAllRateLimiter();

}

// === 初始化拦截器 ===
void registerAllInterceptorImpl() {
    Interceptor::getInstance().registerTypeRule("login_unrequired", {
    "/login.html",
    "/register",
    "/index.html",
    "/",
    "/logo.jpg",
    "/api/hello",
    "/login",
    "/welcome.html",
    "/video.mp4",
    "/test_transaction.html",
    "/api/tx_test",
    "/ai/chat",
    "/ai/history"
    
    }, true);

     // 注册 URI 黑名单（命中即拦截）
    Interceptor::getInstance().registerTypeRule("uri_blacklist", {
        "/forbidden",
        "/hack.html",
        "/internal/debug",
        "/admin/deleteAll",
        "/api/hack", 
        "/shutdown"
    }, false); // false 表示黑名单机制
}

void initInterceptor(ServerConfig config) {
    Interceptor::getInstance().init(config.interceptor_close);
    InterceptorConfig::setInterceptorFunc(registerAllInterceptorImpl);
    if ( !Interceptor::getInstance().isInterceptorClose()) InterceptorConfig::registerAllInterceptor();
}

// === 初始化数据库 ===
void initSqlConnPool(ServerConfig config,
                        std::string& db_username, 
                        std::string& db_password, 
                        std::string& db_name, 
                        int db_port
                        ) 
{
    LOG_INFO(BASE_TEXT + "初始化数据库连接池，共" + std::to_string(config.conn_num) + "个数据库连接对象。");
    SqlConnPool::getInstance().init("localhost", db_port, db_username, db_password, db_name, config.conn_num);
}

// === 初始化连接对象工厂 ===
void initConnFactory(ServerConfig config) {
    ConnFactoryManager::getInstance().registerFactory(config.http_port, []() {
        return std::make_shared<HttpConn>();
    });
    ConnFactoryManager::getInstance().registerFactory(config.test_port, []() {
        return std::make_shared<HttpConn>();
    });
    ConnFactoryManager::getInstance().registerFactory(config.websocket_port, []() {
        return std::make_shared<WebSocketConn>();
    });
}

// === 初始化Phi3引擎池 ===
void initPhi3EnginePool() {
    Phi3EnginePool::getInstance().init(8);
}

void initAllModules(ServerConfig config, 
                    std::string& db_username, 
                    std::string& db_password, 
                    std::string& db_name, 
                    int db_port) {
    // === 日志模块 ===
    initLog(config);
    // === 热更新模块 ===
    initConfigManager();
    // === 路由器模块 ===
    initRouter();
    // === 频率限制器模块 ===
    initRateLimiter(config);
    // === 拦截器模块 ===
    initInterceptor(config);
    // === 数据库模块 ===
    initSqlConnPool(config, db_username, db_password, db_name, db_port);
    // === 初始化连接对象工厂 === 
    initConnFactory(config);
    // === 初始化Phi3引擎池 ===
    initPhi3EnginePool();
}