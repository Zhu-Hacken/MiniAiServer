#pragma once
#include "log_utils.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <onnxruntime/onnxruntime_c_api.h>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>
#include <memory>
#include <vector>

class OnnxEngine {
public:
    OnnxEngine() = default;
    ~OnnxEngine();

    // // 加载模型
    // bool loadModel(const std::string& model_path);

    // 执行推理：输入 -> 输出
    bool run(std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs);
    // bool run(const std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs);

    // // 获取输入名称
    // const std::vector<const char*>& getInputNames() const {return m_input_names;}

    // // 获取输出名称
    // const std::vector<const char*>& getOutputNames() const {return m_output_names;}

    // // 获取输入shape模板
    // const std::vector<std::vector<int64_t>>& getInputShapes() const {return m_input_shapes;}

protected:
    // // 供子类构造张量时使用
    // Ort::Env m_env;                                 // ONNX 运行时环境
    // Ort::SessionOptions m_session_options;          // 会话配置
    // Ort::AllocatorWithDefaultOptions m_allocator;   // 默认分配器
    // std::unique_ptr<Ort::Session> m_session;        // 推理 session     

    // std::vector<const char*> m_input_names;         
    // std::vector<const char*> m_output_names;
    // std::vector<std::vector<int64_t>> m_input_shapes;

    // 填补动态shape
    static std::vector<int64_t> fillDynamicShape(const std::vector<int64_t>& template_shape, int64_t seq_len = 1);
};

