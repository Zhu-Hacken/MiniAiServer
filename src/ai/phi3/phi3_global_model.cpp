#include "phi3_global_model.h"
#include <string>
#include <filesystem>
#include "util/sys_utils.h"
#include "log/logs.h"

const std::string BASE_TEXT = "[Phi3GlobalModel] ";

Phi3GlobalModel& Phi3GlobalModel::getInstance() {
    static Phi3GlobalModel instance;
    return instance;
}

Phi3GlobalModel::Phi3GlobalModel(): m_env(ORT_LOGGING_LEVEL_WARNING, "OnnxEngine"), m_loaded(false)  {
    std::string model_path = SysUtils::getRootPath() + "/model/phi3/phi3-mini-4k-instruct-cpu-int4-rtn-block-32-acc-level-4.onnx";
    std::string tokenizer_json_path = SysUtils::getRootPath() + "/model/phi3/tokenizer.json";
    // 检查模型文件是否存在
    if (!std::filesystem::exists(model_path)) {
        LOG_INFO(BASE_TEXT + "模型文件不存在：" + model_path);
        return;
    }
    // 检查 tokenizer.json 文件是否存在
    if (!m_tokenizer.load(tokenizer_json_path)) {
        LOG_INFO(BASE_TEXT + "tokenizer.json 加载失败：" + tokenizer_json_path);
        return;
    }

    // 配置 Session 选项（设置线程数、图优化等级等）
    m_session_options.SetIntraOpNumThreads(1);  // 单线程
    m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

    // 创建 Session，加载模型
    try {
        m_session = std::make_unique<Ort::Session>(m_env, model_path.c_str(), m_session_options);
        m_loaded = true;
    } catch (const Ort::Exception& e) {
        LOG_ERROR(BASE_TEXT + "创建 Session 失败：" + e.what());
        return;
    }

    // 清理旧资源
    for (auto ptr : m_input_names) {
        Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
    for (auto ptr : m_output_names) {
        Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
    m_input_names.clear();
    m_output_names.clear();
    m_input_shapes.clear();

    // 获取输入信息
    size_t input_count = m_session->GetInputCount();
    for (size_t i = 0; i < input_count; ++i) {
        auto type_info = m_session->GetInputTypeInfo(i);
        auto shape = type_info.GetTensorTypeAndShapeInfo().GetShape();
        m_input_shapes.push_back(shape);
        m_input_names.emplace_back(m_session->GetInputNameAllocated(i, m_allocator).release());



        // auto name = m_input_names.back();
        // LOG_INFO(BASE_TEXT + "input_name = " + name);
        // auto info = m_session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();

        // auto type = info.GetElementType();
        // std::string dtype = "UNKNOWN (" + std::to_string(type) + ")";
        // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) dtype = "float32";
        // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) dtype = "int64";
        // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16) dtype = "float16";
        // 可能扩展其他类型

        // LOG_INFO("🔹输入 " + std::to_string(i) + ": " + name + ", dtype=" + dtype);

    }

    // 获取输出信息
    size_t output_count = m_session->GetOutputCount();
    for (size_t i = 0; i < output_count; ++i) {
        m_output_names.emplace_back(m_session->GetOutputNameAllocated(i, m_allocator).release());
        // LOG_INFO(BASE_TEXT + m_output_names.back());    
    }

    LOG_INFO(BASE_TEXT + "模型加载成功：" + model_path);

}

Phi3GlobalModel::~Phi3GlobalModel() {
    for (auto ptr: m_input_names) {
        if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
    for (auto ptr: m_output_names) {
        if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    }
}

Ort::Session& Phi3GlobalModel::getSession() {
    return *m_session;
}