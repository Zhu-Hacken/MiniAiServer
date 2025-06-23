#pragma once

#include "ai/phi3/phi3_engine.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

class Phi3EnginePool {
public:
    static Phi3EnginePool& getInstance();
    std::shared_ptr<Phi3Engine> getEngine();
    void releaseEngine(std::shared_ptr<Phi3Engine> engine);
    void init(int engine_num);
    void shutdown();
    int getFreeCount() const {return m_free_engine;}
    
private:
    Phi3EnginePool() = default;
    ~Phi3EnginePool();

    // 禁用拷贝构造和赋值
    Phi3EnginePool(const Phi3EnginePool&) = delete;
    Phi3EnginePool operator=(const Phi3EnginePool&) = delete;

private:
    std::queue<std::shared_ptr<Phi3Engine>> m_engine_queue; // 引擎池
    std::mutex m_mutex;                             // 互斥锁
    std::condition_variable m_cond;                 // 条件变量（阻塞等待）

    int m_max_engine = 0;                           // 最大引擎数量
    int m_free_engine = 0;                             // 当前空闲引擎数量

};