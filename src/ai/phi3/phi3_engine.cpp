#include "phi3_engine.h"
#include "ai/onnx_engine.h"
#include "ai/phi3/phi3_global_model.h"
#include "log/logs.h"
#include "log_utils.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <ostream>
#include <string>
#include <vector>
#include <numeric>

const std::string BASE_TEXT = "[Phi3Engine] ";

Phi3Engine::Phi3Engine() {

}

bool Phi3Engine::init(const std::string& model_path, const std::string& tokenizer_json_path) {
    // if (!loadModel(model_path)) {
    if (!Phi3GlobalModel::getInstance().isLoaded()) {
        LOG_ERROR(BASE_TEXT + "模型加载失败：" + model_path);
        return false;
    }

    if (!m_tokenizer.load(tokenizer_json_path)) {
        LOG_ERROR(BASE_TEXT + "Tokenizer 加载失败！");
    }

    LOG_INFO(BASE_TEXT + "模型与Tokenizer加载成功！");

    // 获取并缓存输入输出维度
    // auto input_type_info = m_session->GetInputTypeInfo(0);
    auto input_type_info = Phi3GlobalModel::getInstance().getSession().GetInputTypeInfo(0);
    m_input_shape = input_type_info.GetTensorTypeAndShapeInfo().GetShape();

    auto output_type_info = Phi3GlobalModel::getInstance().getSession().GetOutputTypeInfo(0);
    m_output_shape = output_type_info.GetTensorTypeAndShapeInfo().GetShape();

    return true;
}

size_t totalElements(const std::vector<int64_t>& s) {
    size_t t = 1; for (auto d : s) t *= d; return t;
}

std::string Phi3Engine::build_prompt(const std::string& user_input) {
    return "<|user|>\n" + user_input + " <|end|>\n<|assistant|>";
}

bool Phi3Engine::chat(const std::string& input, std::string& response) {
    const std::string prompt = build_prompt(input);
    LOG_DEBUG(BASE_TEXT + "收到 Prompt: " + prompt);

    // 编码输入的文本prompt
    std::vector<int64_t> input_ids = m_tokenizer.encodeFromPython(prompt);      
    if (input_ids.empty()) {
        LOG_ERROR(BASE_TEXT + "Tokenizer 编码失败！");
        return false;
    }

    // 初始化状态
    response.clear();
    std::vector<int64_t> generated_ids = input_ids;
    std::vector<int64_t> answered_ids;
    int64_t context_len = static_cast<int64_t>(input_ids.size()) - 1;   // 累积已缓存token
    size_t max_steps = 2048;     // 最大生成步数
    std::vector<Ort::Value> past_kv_tensors;        // 缓存KV
    
    // 构造初始 KV（全0）
    for (size_t i = 2; i < Phi3GlobalModel::getInstance().getInputShapes().size(); ++i) {
        auto shape = fillDynamicShape(Phi3GlobalModel::getInstance().getInputShapes()[i], 0);
        // Ort::Value tensor = Ort::Value::CreateTensor<float>(m_allocator, shape.data(), shape.size());
        Ort::Value tensor = Ort::Value::CreateTensor<float>(Phi3GlobalModel::getInstance().getAllocator(), shape.data(), shape.size());
        std::fill_n(tensor.GetTensorMutableData<float>(), totalElements(shape), 0.0f);
        past_kv_tensors.emplace_back(std::move(tensor));
    }

    // 打印初始 tokens
    // for (auto id : input_ids) std::cout << id << " ";
    // std::cout << std::endl;

    // step-by-step 推理生成
    for (size_t step = 0; step < max_steps; ++step) {
        std::vector<int64_t> cur_input;
        int past_len = (step == 0) ? 0: context_len;
        
        if (step == 0) {
            cur_input = input_ids;  // 初始使用完整 prompt
        } else {
            cur_input = {generated_ids.back()}; // 后续只使用上轮生成的 token
        }
        
        // 构造输入张量
        std::vector<Ort::Value> inputs;
        {
            std::vector<int64_t> input_shape = fillDynamicShape(Phi3GlobalModel::getInstance().getInputShapes()[0], cur_input.size());
            Ort::Value input_tensor = Ort::Value::CreateTensor<int64_t>(Phi3GlobalModel::getInstance().getAllocator(), input_shape.data(), input_shape.size());
            std::copy(cur_input.begin(), cur_input.end(), input_tensor.GetTensorMutableData<int64_t>());
            inputs.emplace_back(std::move(input_tensor));
        }
        // 构造attention_mask张量（全部为1）
        {
            int64_t total_len = past_len + static_cast<int64_t>(cur_input.size());
            std::vector<int64_t> attention_mask(total_len, 1);
            std::vector<int64_t> mask_shape = fillDynamicShape(Phi3GlobalModel::getInstance().getInputShapes()[1], attention_mask.size());
            Ort::Value mask_tensor = Ort::Value::CreateTensor<int64_t>(Phi3GlobalModel::getInstance().getAllocator(), mask_shape.data(), mask_shape.size());
            std::copy(attention_mask.begin(), attention_mask.end(), mask_tensor.GetTensorMutableData<int64_t>());
            inputs.emplace_back(std::move(mask_tensor));
        }

        // past_key_values
        for (auto& kv: past_kv_tensors) {
            inputs.emplace_back(std::move(kv));
        }
        past_kv_tensors.clear();

        // 执行推理
        std::vector<Ort::Value> outputs;
        if (!run(inputs, outputs) || outputs.empty() || !outputs[0].IsTensor()) {
            LOG_ERROR(BASE_TEXT + "推理失败！");
            return false;
        }

        // 获取下一个 token
        auto logits_info = outputs[0].GetTensorTypeAndShapeInfo();
        auto shape = logits_info.GetShape();
        float* logits = outputs[0].GetTensorMutableData<float>();

        if (shape.size() != 3 || shape[0] != 1) {
            LOG_ERROR(BASE_TEXT + "logits shape 异常！");
            return false;
        }

        int vocab_size = shape[2];
        float* last_token_logits = logits + (shape[1] - 1) * vocab_size;
        // 取最大值 token
        int64_t next_token_id = std::distance(
            last_token_logits,
            std::max_element(last_token_logits, last_token_logits + vocab_size)
        );

        // 检查是否为 eos
        if (next_token_id == m_tokenizer.getEosTokenId()) {
            // LOG_INFO(BASE_TEXT + "生成结束（遇到</s>）");
            break;
        }

        // 添加token
        generated_ids.push_back(next_token_id);
        answered_ids.push_back(next_token_id);

        // 更新下一轮输入
        if (step == 0) context_len = static_cast<int64_t>(cur_input.size());
        else ++context_len;

        for (size_t i = 1; i < outputs.size(); ++i) {
            past_kv_tensors.emplace_back(std::move(outputs[i]));
        }

    }
    response = m_tokenizer.decodeFromPython(answered_ids);
    LOG_DEBUG(BASE_TEXT + "AI响应：" + response);
    return true;
}
