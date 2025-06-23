#include "onnx_engine.h"
#include "ai/phi3/phi3_global_model.h"
#include "log_utils.h"
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <string>
#include <vector>

const std::string BASE_TEXT = "[OnnxEngine] ";

// OnnxEngine::OnnxEngine():m_env(ORT_LOGGING_LEVEL_WARNING, "OnnxEngine") {
    
// }

OnnxEngine::~OnnxEngine() {
    // for (auto ptr: m_input_names) {
    //     if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    // }
    // for (auto ptr: m_output_names) {
    //     if (ptr) Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
    // }
}

// bool OnnxEngine::loadModel(const std::string& model_path) {
//     // 检查模型文件是否存在
//     if (!std::filesystem::exists(model_path)) {
//         LOG_INFO(BASE_TEXT + "模型文件不存在：" + model_path);
//         return false;
//     }
    
//     // 配置 Session 选项（设置线程数、图优化等级等）
//     m_session_options.SetIntraOpNumThreads(1);  // 单线程
//     m_session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_EXTENDED);

//     // 创建 Session，加载模型
//     try {
//         m_session = std::make_unique<Ort::Session>(m_env, model_path.c_str(), m_session_options);
//     } catch (const Ort::Exception& e) {
//         LOG_ERROR(BASE_TEXT + "创建 Session 失败：" + e.what());
//         return false;
//     }

//     // 清理旧资源
//     for (auto ptr : m_input_names) {
//         Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
//     }
//     for (auto ptr : m_output_names) {
//         Ort::AllocatorWithDefaultOptions().Free((void *)ptr);
//     }
//     m_input_names.clear();
//     m_output_names.clear();
//     m_input_shapes.clear();

//     // 获取输入信息
//     size_t input_count = m_session->GetInputCount();
//     for (size_t i = 0; i < input_count; ++i) {
//         auto type_info = m_session->GetInputTypeInfo(i);
//         auto shape = type_info.GetTensorTypeAndShapeInfo().GetShape();
//         m_input_shapes.push_back(shape);
//         m_input_names.emplace_back(m_session->GetInputNameAllocated(i, m_allocator).release());



//         auto name = m_input_names.back();
//         // LOG_INFO(BASE_TEXT + "input_name = " + name);
//         // auto info = m_session->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();

//         // auto type = info.GetElementType();
//         // std::string dtype = "UNKNOWN (" + std::to_string(type) + ")";
//         // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT) dtype = "float32";
//         // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64) dtype = "int64";
//         // if (type == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16) dtype = "float16";
//         // 可能扩展其他类型

//         // LOG_INFO("🔹输入 " + std::to_string(i) + ": " + name + ", dtype=" + dtype);

//     }

//     // 获取输出信息
//     size_t output_count = m_session->GetOutputCount();
//     for (size_t i = 0; i < output_count; ++i) {
//         m_output_names.emplace_back(m_session->GetOutputNameAllocated(i, m_allocator).release());
//         // LOG_INFO(BASE_TEXT + m_output_names.back());    
//     }

//     LOG_INFO(BASE_TEXT + "模型加载成功：" + model_path);
//     return true;
// }

// bool OnnxEngine::run(const std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs) {
bool OnnxEngine::run(std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs) {
    // if (!m_session) {
    if (!Phi3GlobalModel::getInstance().getSession()) {
        LOG_ERROR(BASE_TEXT + "Run前未加载模型！");
        return false;
    }

    // LOG_INFO(BASE_TEXT + "输入张量数量 = " + std::to_string(inputs.size()));
    // LOG_INFO(BASE_TEXT + "模型预期输入数量 = " + std::to_string(m_input_names.size()));


    // // =============================================
    // for (size_t i = 0; i < inputs.size(); ++i) {
    //     // if ((i != 0 && i != 1 && i != 2)) continue;
    //     if (true) break;
    //     if (!inputs[i].IsTensor()) {
    //         LOG_ERROR("Input[" + std::to_string(i) + "] is not a tensor!");
    //         continue;
    //     }

    //     auto tensor_info = inputs[i].GetTensorTypeAndShapeInfo();
    //     auto shape = tensor_info.GetShape();
    //     size_t total_elements = tensor_info.GetElementCount();
    //     ONNXTensorElementDataType type = tensor_info.GetElementType();

    //     std::ostringstream oss;
    //     oss << "Input[" << i << "]: type = " << type
    //         << ", shape = [";
    //     for (size_t j = 0; j < shape.size(); ++j) {
    //         oss << shape[j];
    //         if (j != shape.size() - 1) oss << ", ";
    //     }
    //     oss << "], total_elements = " << total_elements;

    //     switch (type) {
    //         case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT: {
    //             float* data = inputs[i].GetTensorMutableData<float>();
    //             oss << ", data[0:4] = [";
    //             for (size_t j = 0; j < std::min<size_t>(4, total_elements); ++j) {
    //                 oss << data[j];
    //                 if (j < 3) oss << ", ";
    //             }
    //             oss << "]";
    //             break;
    //         }
    //         case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64: {
    //             int64_t* data = inputs[i].GetTensorMutableData<int64_t>();
    //             oss << ", data[0:4] = [";
    //             for (size_t j = 0; j < std::min<size_t>(4, total_elements); ++j) {
    //                 oss << data[j];
    //                 if (j < 3) oss << ", ";
    //             }
    //             oss << "]";
    //             break;
    //         }
    //         default:
    //             oss << ", data dump not supported for type: " << type;
    //             break;
    //     }

    //     LOG_INFO(oss.str());
    // }
    // // =============================================


    try {
        // outputs = m_session->Run(
        //     Ort::RunOptions(nullptr),       // 默认 run options
        //     m_input_names.data(),           // 输入张量名
        //     inputs.data(),                  // 输入张量名数组
        //     inputs.size(),                  // 输入张量数量
        //     m_output_names.data(),          // 输出张量名数组
        //     m_output_names.size()           // 输出张量数量
        // );

        outputs = Phi3GlobalModel::getInstance().getSession().Run(
            Ort::RunOptions(nullptr),       // 默认 run options
            Phi3GlobalModel::getInstance().getInputNames().data(),           // 输入张量名
            inputs.data(),                  // 输入张量名数组
            inputs.size(),                  // 输入张量数量
            Phi3GlobalModel::getInstance().getOutputNames().data(),          // 输出张量名数组
            Phi3GlobalModel::getInstance().getOutputNames().size()           // 输出张量数量
        );

        if (outputs.empty()) {
            LOG_ERROR(BASE_TEXT + "Run 执行成功但无输出！");
            return false;
        }

        // LOG_INFO(BASE_TEXT + "推理完成，输出数量：" + std::to_string(outputs.size()));
        return true;

    } catch (const Ort::Exception& e ) {
        LOG_ERROR(BASE_TEXT + "Run 执行异常：" + std::string(e.what()));
        return false;
    }

}

std::vector<int64_t> OnnxEngine::fillDynamicShape(const std::vector<int64_t>& template_shape, int64_t seq_len) {
    std::vector<int64_t> filled_shape = template_shape;

    for (size_t i = 0; i < filled_shape.size(); ++i) {
        if (filled_shape[i] < 0) {
            // 常见处理：
            // - 第 0 维 -> batch_size 默认为1
            // - 第 1 维 -> seq_len（动态长度）使用传入参数
            if (i == 0) filled_shape[i] = 1;
            else filled_shape[i] = seq_len;
        }
    
    }
    return filled_shape;
}