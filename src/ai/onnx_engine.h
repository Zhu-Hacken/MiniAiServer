#pragma once
#include "log_utils.h"
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

    // 推理接口（输入输出均为T向量）
    template<typename T>
    bool infer(const std::vector<T>& input, std::vector<T>& output);

protected:
    std::unique_ptr<Ort::Session> m_session;        // 推理 session     

private:
    Ort::Env m_env;                                 // ONNX 运行时环境
    Ort::SessionOptions m_session_options;          // 会话配置
    Ort::AllocatorWithDefaultOptions m_allocator;   // 默认分配器

    std::vector<const char*> m_input_names;         
    std::vector<const char*> m_output_names;
};

const std::string ONNX_BASE_TEXT = "[OnnxEngine] ";

template<typename T>
bool OnnxEngine::infer(const std::vector<T>& input, std::vector<T>& output) {
    try {
        // === 准备输入张量 ===
        Ort::MemoryInfo memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);

        // 获取输入张量维度（例如：1xN）
        Ort::TypeInfo input_type_info = m_session->GetInputTypeInfo(0);
        auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
        std::vector<int64_t> input_shape = input_tensor_info.GetShape();

        // 检查输入尺寸匹配
        size_t expected_input_size = 1;
        for (auto dim: input_shape) {
            if (dim > 0) expected_input_size *= dim;
        }

        if (input.size() != expected_input_size) {
            LOG_ERROR(ONNX_BASE_TEXT + "Input size mismatch. Expected: " + std::to_string(expected_input_size) + ", got: " + std::to_string(input.size()));
            return false;
        }

        // 创建张量对象
        Ort::Value input_tensor = Ort::Value::CreateTensor<T>(
            memory_info,
            const_cast<T*>(input.data()),
            input.size(),
            input_shape.data(),
            input_shape.size()
        );

        // 执行推理
        auto output_tensors = m_session->Run(
            Ort::RunOptions(nullptr),
            m_input_names.data(),
            &input_tensor,
            1,
            m_output_names.data(),
            1
        );

        // === 读取输出张量 ===
        if (output_tensors.empty() || !output_tensors[0].IsTensor()) {
            LOG_ERROR(ONNX_BASE_TEXT + "Output tensor is invalid.");
            return false;
        }

        // === 提取输出结果 ===
        auto output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
        size_t output_size = output_info.GetElementCount();

        T* output_data = output_tensors[0].GetTensorMutableData<T>();
        output.assign(output_data, output_data + output_size);

        std::vector<int64_t> output_shape = output_info.GetShape();
        LOG_INFO(ONNX_BASE_TEXT + "Output shape: " + std::to_string(output_shape[0]) + " ... ");
        return true;

    } catch(const Ort::Exception& e) {
        LOG_ERROR(ONNX_BASE_TEXT + "ONNX Inference failed: " + e.what());
        return false;
    }
}