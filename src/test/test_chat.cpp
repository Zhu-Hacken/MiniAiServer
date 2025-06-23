#include "ai/phi3/phi3_engine.h"
#include <iostream>
#include <string>
#include "log_utils.h"
#include "logs.h"
#include "util/utils.h"

const std::string BASE_TEXT = "[TestChat] ";

std::string build_prompt(const std::string& user_input) {
    return "<|user|>\n" + user_input + " <|end|>\n<|assistant|>";
}

int main() {
    Log::getInstance().init(Log::DEBUG, "server_log", true);

    Phi3Engine engine;
    std::string model_path = SysUtils::getRootPath() + "/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx";
    std::string tokenizer_json_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json";
    LOG_INFO(BASE_TEXT + model_path);
    if (!engine.init(model_path, tokenizer_json_path)) {
        // LOG_ERROR(BASE_TEXT + "模型加载失败");
        // std::cout << BASE_TEXT << "模型加载失败" << std::endl;
        return -1;
    }

    // std::string prompt = build_prompt("What is 1+1? ");
    // std::string prompt = build_prompt("你是谁？");
    std::string prompt = build_prompt("Hello");
    std::string response;
    if (engine.chat(prompt, response)) {
        // LOG_INFO(BASE_TEXT + "AI响应：" + response);
    } else {
        LOG_ERROR(BASE_TEXT + "推理失败");
    }
    return 0;
}