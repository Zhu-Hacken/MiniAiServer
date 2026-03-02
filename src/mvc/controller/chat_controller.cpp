#include "chat_controller.h"
#include "ai/phi3/phi3_engine.h"
#include "ai/phi3/phi3_engine_raii.h"
#include "ai/phi3/phi3_global_model.h"
#include "ai_session/chat_context_store.h"
#include "log_utils.h"
#include "session_manager.h"
#include <sstream>
#include <string>
#include <iostream>
#include "websocket/websocket_conn_manager.h"


const std::string BASE_TEXT = "[ChatController] ";

void ChatController::chat(HttpRequest& http_request, HttpResponse& http_response) {
    
    const auto& json = http_request.getJson();

    if (!json.contains("message")) {
        http_response.sendJson(400, {{"error", "Missing 'message' field"}});
        return;
    }

    // 获取输入
    std::string input = http_request.getJson()["message"];
    std::string sessionId;

    if (json.contains("sessionId")) {
        sessionId = json["sessionId"];
    }
    
    if ( !SessionManager::getInstance().isSessionIdValid(sessionId)) {
        sessionId = SessionManager::getInstance().createSession();
    }
    
    
    Phi3EngineRAII engine;
    std::string response;
    // engine->chat(input, response, sessionId);
    Json json_output;
    json_output["data"] = response;
    json_output["sessionId"] = sessionId;

    http_response.sendJson(200, json_output);
}

// void ChatController::chat(HttpRequest& http_request, HttpResponse& http_response) {
//     // std::string model_path = SysUtils::getRootPath() + "/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx";
//     // std::string tokenizer_json_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json";
//     // LOG_INFO(BASE_TEXT + model_path);
    
//     const auto& json = http_request.getJson();

//     if (!json.contains("message")) {
//         http_response.sendJson(400, {{"error", "Missing 'message' field"}});
//         return;
//     }

//     // 获取输入
//     std::string input = http_request.getJson()["message"];
//     std::string sessionId;

//     if (json.contains("sessionId")) {
//         sessionId = json["sessionId"];
//     }
    
//     // Phi3Engine engine;
//     // if (!engine.init(model_path, tokenizer_json_path)) {
//     //     // LOG_ERROR(BASE_TEXT + "模型加载失败");
//     //     // std::cout << BASE_TEXT << "模型加载失败" << std::endl;
//     // }
//     if ( !SessionManager::getInstance().isSessionIdValid(sessionId)) 
//         sessionId = SessionManager::getInstance().createSession();
    
    
//     // auto& model = Phi3GlobalModel::getInstance();
//     // auto& session = model.getSession();
    
//     Phi3EngineRAII engine;
//     std::string response;
//     engine->chat(input, response, sessionId);
//     Json json_output;
//     json_output["data"] = response;
//     json_output["sessionId"] = sessionId;
//     // std::ostringstream oss;
//     // oss << "Model loaded successfully. Session ptr: " << &session;

//     // LOG_INFO(BASE_TEXT + oss.str());

//     // Json json;
//     // json["data"] = input;
//     http_response.sendJson(200, json_output);
// }



void ChatController::getHistory(HttpRequest& http_request, HttpResponse& http_response) {
    LOG_DEBUG(BASE_TEXT + "进入 getHistory()");
    const auto& json = http_request.getJson();

    // 获取输入
    std::string sessionId;

    if (json.contains("sessionId")) {
        sessionId = json["sessionId"];
    }
    Json json_out;
    Json json_arr = Json::array();
    if ( !SessionManager::getInstance().isSessionIdValid(sessionId) && !SessionManager::getInstance().isSessionIdExpired(sessionId)) {
        LOG_DEBUG(BASE_TEXT + "无效 sessionId = " + sessionId);
        sessionId = SessionManager::getInstance().createSession(180000);
        // http_response.sendJson(400, {{"error", "Invalid sessionId"}});
    } else {
        const auto& history = ChatContextStore::getInstance().getContext(sessionId).historyText;
        if (!history.empty()) {
            for (const auto& [user, ai] : history) {
                json_arr.push_back({{"user", user}, {"ai", ai}});
            }
        }
    }
    json_out["sessionId"] = sessionId;
    json_out["history"] = json_arr; 

    http_response.sendJson(200, json_out);
}

void ChatController::streamInfer(WebSocketConn& conn, SessionId& sessionId, const std::string& msg) {
    if (msg.empty()) {
        LOG_INFO(BASE_TEXT + "收到来自 sessionId = " + sessionId + " 的空消息");
        return;
    }
    LOG_INFO(BASE_TEXT + "收到来自 sessionId = " + sessionId + " 的流式请求，内容：" + msg);
    Phi3EngineRAII engine;
    engine->chatStream(msg, sessionId);

    WebsocketConnManager::getInstance().sendToSession(sessionId, "[DONE]");
}