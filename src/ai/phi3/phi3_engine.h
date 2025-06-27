#pragma once
#include "ai/onnx_engine.h"
#include <cstdint>
#include <string>
#include <vector>
#include "session_manager.h"
#include "tokenizer/tokenizer.h"

/*
 * Phi3 对话引擎：封装对 phi3.onnx 的加载与推理流程
*/
class Phi3Engine : public OnnxEngine {
public:
    Phi3Engine();
    ~Phi3Engine() = default;

    // 加载模型：指定模型路径
    bool init(const std::string& model_path, const std::string& tokenizer_json_path);

    // 对话接口：传入用户问题，返回模型回复
    // bool chat(const std::string& prompt, std::string& response);
    bool chat(const std::string& input, std::string& response, const std::string& sessionId);
    bool chatStream(const std::string msg, const SessionId sessionId);

private:
    std::string buildPrompt(const std::string& user_input);
    std::string buildPromptWithHistory(const std::string& user_input, const std::string& sessionId);
    // std::string getUtf8Delta(const std::string& prev, const std::string& curr);

    Tokenizer m_tokenizer;
    // 模型输入输出维度缓存
    std::vector<int64_t> m_input_shape;
    std::vector<int64_t> m_output_shape;
};