#include "onnx_engine.h"
#include "log_utils.h"
#include <cstddef>
#include <filesystem>
#include <memory>
#include <onnxruntime/onnxruntime_c_api.h>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>

OnnxEngine::OnnxEngine():m_env(ORT_LOGGING_LEVEL_WARNING, "OnnxEngine") {
    
}

OnnxEngine::~OnnxEngine() {
    for (auto ptr: m_input_names) {
        if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
    for (auto ptr: m_output_names) {
        if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
}

bool OnnxEngine::loadModel(const std::string& model_path) {
    // 检查模型文件是否存在
    if (!std::filesystem::exists(model_path)) {
        LOG_INFO(ONNX_BASE_TEXT + "模型文件不存在：" + model_path);
        return false;
    }
    
    // 创建会话（设置线程数、图优化等级等）
    m_session_options.SetIntraOpNumThreads(1);  // 使用单线程
    m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // 创建 Session，加载模型
    try {
        m_session = std::make_unique<Ort::Session>(m_env, model_path.c_str(), m_session_options);
    } catch (const Ort::Exception& e) {
        LOG_ERROR(ONNX_BASE_TEXT + "创建 Session 失败：" + e.what());
        return false;
    }

    // 清空旧的输入输出名（防止多次loadodel调用时出错）
    m_input_names.clear();
    m_output_names.clear();

    // 获取输入输出数量
    size_t input_count = m_session->GetInputCount();
    size_t output_count = m_session->GetOutputCount();

    for (size_t i = 0; i < input_count; ++i) {
        // TODO: 记得释放
        m_input_names.emplace_back(m_session->GetInputNameAllocated(i, m_allocator).release());
    }

    for (size_t i = 0; i < output_count; ++i) {
        m_output_names.emplace_back(m_session->GetOutputNameAllocated(i, m_allocator).release());
    }

    return true;
}