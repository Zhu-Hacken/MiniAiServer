#pragma once
#include "tokenizer/tokenizer.h"
#include <memory>
#include <onnxruntime/onnxruntime_cxx_api.h>


class Phi3GlobalModel {
public:
    ~Phi3GlobalModel();

    static Phi3GlobalModel& getInstance();

    Ort::Session& getSession();

    // 获取输入名称
    const std::vector<const char*>& getInputNames() const {return m_input_names;}

    // 获取输出名称
    const std::vector<const char*>& getOutputNames() const {return m_output_names;}

    // 获取 m_allocator
    const Ort::AllocatorWithDefaultOptions& getAllocator() const {return m_allocator;}

    // 获取输入 shape 模板
    const std::vector<std::vector<int64_t>>& getInputShapes() const {return m_input_shapes;}

    // 获取 tokenizer
    const Tokenizer& getTokenizer() const {return m_tokenizer;}

    // 获取模型是否加载
    bool isLoaded() const {return m_loaded;}

private:
    Phi3GlobalModel();

    Phi3GlobalModel(const Phi3GlobalModel&) = delete;
    Phi3GlobalModel& operator=(const Phi3GlobalModel&) = delete;

private:
    Ort::Env m_env;                                 // ONNX 运行时环境
    Ort::SessionOptions m_session_options;          // 会话配置
    Ort::AllocatorWithDefaultOptions m_allocator;   // 默认分配器
    std::unique_ptr<Ort::Session> m_session;        // 推理 session     

    std::vector<const char*> m_input_names;         
    std::vector<const char*> m_output_names;
    std::vector<std::vector<int64_t>> m_input_shapes;

    Tokenizer m_tokenizer;

    bool m_loaded;
};