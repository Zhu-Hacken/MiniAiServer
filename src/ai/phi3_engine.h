#pragma once
#include "onnx_engine.h"
#include <cstdint>
#include <string>
#include <vector>
#include "tokenizer/tokenizer.h"

/*
 * Phi3 对话引擎：封装对 phi3.onnx 的加载与推理流程
*/
class Phi3Engine : public OnnxEngine {
public:
    Phi3Engine() = default;
    ~Phi3Engine() = default;

    // 加载模型：指定模型路径
    bool init(const std::string& model_path, const std::string& tokenizer_json_path);

    // 对话接口：传入用户问题，返回模型回复（可异步拓展）
    bool chat(const std::string& prompt, std::string& response);

private:
    // 将输入文本转换为模型的输入张量（示例：token -> embedding）
    bool preprocess(const std::string& text, std::vector<float>& input_tensor);

    // 将输出张量还原为文本
    std::string postprocess(const std::vector<float>& output_tensor);

    // 
    // 用模板 shape & 动态 len  创建  int64 tensor
    Ort::Value makeInt64Tensor(const std::vector<int64_t>& tmpl,const std::vector<int64_t>& data);

    // 构造全 1 mask
    Ort::Value makeOnesInt64Tensor(const std::vector<int64_t>& tmpl, int64_t seq_len);

    // 初始化空 kv
    void initEmptyKv(int64_t past_len, std::vector<Ort::Value>& kv_vec);
private:
    Tokenizer m_tokenizer;

    // 模型输入输出维度缓存
    std::vector<int64_t> m_input_shape;
    std::vector<int64_t> m_output_shape;
};