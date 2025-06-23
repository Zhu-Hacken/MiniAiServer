#pragma once
#include "ai/onnx_engine.h"
#include <cstdint>
#include <string>
#include <vector>
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

    // 对话接口：传入用户问题，返回模型回复（可异步拓展）
    bool chat(const std::string& prompt, std::string& response);

private:
    std::string build_prompt(const std::string& user_input);
    
    Tokenizer m_tokenizer;
    // 模型输入输出维度缓存
    std::vector<int64_t> m_input_shape;
    std::vector<int64_t> m_output_shape;
};