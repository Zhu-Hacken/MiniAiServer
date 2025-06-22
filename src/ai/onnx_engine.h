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
    OnnxEngine();
    ~OnnxEngine();

    // 加载模型
    bool loadModel(const std::string& model_path);

    // 执行推理：输入 -> 输出
    bool run(std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs);
    // bool run(const std::vector<Ort::Value>& inputs, std::vector<Ort::Value>& outputs);

    // 获取输入名称
    const std::vector<const char*>& getInputNames() const {return m_input_names;}

    // 获取输出名称
    const std::vector<const char*>& getOutputNames() const {return m_output_names;}

    // 获取输入shape模板
    const std::vector<std::vector<int64_t>>& getInputShapes() const {return m_input_shapes;}

protected:
    // 供子类构造张量时使用
    Ort::Env m_env;                                 // ONNX 运行时环境
    Ort::SessionOptions m_session_options;          // 会话配置
    Ort::AllocatorWithDefaultOptions m_allocator;   // 默认分配器
    std::unique_ptr<Ort::Session> m_session;        // 推理 session     

    std::vector<const char*> m_input_names;         
    std::vector<const char*> m_output_names;
    std::vector<std::vector<int64_t>> m_input_shapes;

    // 填补动态shape
    static std::vector<int64_t> fillDynamicShape(const std::vector<int64_t>& template_shape, int64_t seq_len = 1);
};


// template<typename T>
// bool OnnxEngine::infer(const std::vector<T>& input, std::vector<T>& output) {
//     try {
//         // === 准备输入张量 ===
//         Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
//         std::vector<Ort::Value> input_tensors;

//         std::vector<int64_t> input_shape = {1, static_cast<int64_t>(input.size())};

//         // 创建主输入张量
//         Ort::Value input_tensor = Ort::Value::CreateTensor<T>(
//             memory_info,
//             const_cast<T*>(input.data()),
//             input.size(),
//             input_shape.data(),
//             input_shape.size()
//         );
//         input_tensors.emplace_back(std::move(input_tensor));

//         // 构造attention_mask张量
//         std::vector<int64_t> attention_mask(input.size(), 1);
//         Ort::Value attention_tensor = Ort::Value::CreateTensor<T>(
//             memory_info,
//             attention_mask.data(),
//             attention_mask.size(),
//             input_shape.data(),
//             input_shape.size()
//         );
//         input_tensors.emplace_back(std::move(attention_tensor));



//         for (size_t i = 2; i < m_input_names.size(); ++i) {
//             const auto& shape_template = m_input_shapes[i];
//             std::vector<int64_t> shape = shape_template;


//             // for (auto& dim:shape) {
//             //     if (dim < 0) dim = 0;
//             // }

//             for (size_t j = 0; j < shape.size(); ++j) {
//                 if (shape[j] < 0) {
//                     if (shape.size() == 4) {
//                         if (j == 0) shape[j] = 1;
//                         else if (j == 2) shape[j] = 0;
//                         else shape[j] = 1;
//                     } else if (shape.size() != 4) {
//                         LOG_ERROR(ONNX_BASE_TEXT + "动态 shape 结构异常！");
//                         return false;
//                     }
//                 }
//             }


//             size_t total = 1;
//             for (auto d: shape) total *= d;


//             std::vector<float> zero_data (total, 0.0f);

//             Ort::Value past_tensor = Ort::Value::CreateTensor<float>(
//                 memory_info,
//                 zero_data.data(),
//                 zero_data.size(),
//                 shape.data(),
//                 shape.size()
//             );
//             input_tensors.emplace_back(std::move(past_tensor));
//         }

//         std::vector<const char*> input_names;
//         for (const auto& name: m_input_names) {
//             input_names.push_back(name);
//         }

//         // 执行推理
//         auto output_tensors = m_session->Run(
//             Ort::RunOptions(nullptr),
//             input_names.data(),
//             input_tensors.data(),
//             input_tensors.size(),
//             m_output_names.data(),
//             m_output_names.size()
//         );


//         // === 读取输出张量 ===
//         if (output_tensors.empty() || !output_tensors[0].IsTensor()) {
//             LOG_ERROR(ONNX_BASE_TEXT + "Output tensor is invalid.");
//             return false;
//         }

//         // === 提取输出结果 ===
//         auto output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
//         size_t output_size = output_info.GetElementCount();

//         T* output_data = output_tensors[0].GetTensorMutableData<T>();
//         output.assign(output_data, output_data + output_size);

//         std::vector<int64_t> output_shape = output_info.GetShape();
//         LOG_INFO(ONNX_BASE_TEXT + "Output shape: " + std::to_string(output_shape[0]) + " ... ");
//         return true;

//     } catch(const Ort::Exception& e) {
//         LOG_ERROR(ONNX_BASE_TEXT + "ONNX Inference failed: " + e.what());
//         return false;
//     }
// }