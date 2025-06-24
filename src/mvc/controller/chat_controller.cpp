#include "chat_controller.h"
#include "ai/phi3/phi3_engine.h"
#include "ai/phi3/phi3_engine_raii.h"
#include "ai/phi3/phi3_global_model.h"
#include "log_utils.h"
#include "session_manager.h"
#include <sstream>
#include <string>
#include <iostream>


const std::string BASE_TEXT = "[ChatController] ";

void ChatController::chat(HttpRequest& http_request, HttpResponse& http_response) {
    // std::string model_path = SysUtils::getRootPath() + "/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx";
    // std::string tokenizer_json_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json";
    // LOG_INFO(BASE_TEXT + model_path);
    
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
    
    // Phi3Engine engine;
    // if (!engine.init(model_path, tokenizer_json_path)) {
    //     // LOG_ERROR(BASE_TEXT + "模型加载失败");
    //     // std::cout << BASE_TEXT << "模型加载失败" << std::endl;
    // }
    if ( !SessionManager::getInstance().isSessionIdValid(sessionId)) 
        sessionId = SessionManager::getInstance().createSession();
    
    
    // auto& model = Phi3GlobalModel::getInstance();
    // auto& session = model.getSession();
    
    Phi3EngineRAII engine;
    std::string response;
    engine->chat(input, response, sessionId);
    Json json_output;
    json_output["data"] = response;
    json_output["sessionId"] = sessionId;
    // std::ostringstream oss;
    // oss << "Model loaded successfully. Session ptr: " << &session;

    // LOG_INFO(BASE_TEXT + oss.str());

    // Json json;
    // json["data"] = input;
    http_response.sendJson(200, json_output);
}