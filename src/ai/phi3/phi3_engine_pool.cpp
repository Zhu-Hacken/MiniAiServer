#include "phi3_engine_pool.h"
#include "ai/phi3/phi3_engine.h"
#include "log_utils.h"
#include <memory>
#include <mutex>
#include <string>

const std::string BASE_TEXT = "[Phi3EnginePool] ";

Phi3EnginePool& Phi3EnginePool::getInstance() {
    static Phi3EnginePool instance;
    return instance;
}

std::shared_ptr<Phi3Engine> Phi3EnginePool::getEngine() {
    std::unique_lock<std::mutex> lock(m_mutex);
    while (m_engine_queue.empty()) {
        m_cond.wait(lock);
    }
    std::shared_ptr<Phi3Engine> engine = m_engine_queue.front();
    m_engine_queue.pop();
    --m_free_engine;
    return engine;
}

void Phi3EnginePool::releaseEngine(std::shared_ptr<Phi3Engine> engine) {
    if (!engine) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_engine_queue.push(engine);
    ++m_free_engine;
    m_cond.notify_one();
}

void Phi3EnginePool::init(int engine_num) {
    m_max_engine = engine_num;

    for (int i = 0; i < m_max_engine; ++i) {
        std::shared_ptr<Phi3Engine> engine = std::make_shared<Phi3Engine>();
        m_engine_queue.push(engine);
        ++m_free_engine;
    }
    LOG_INFO(BASE_TEXT + "Initialized: " + std::to_string(engine_num) + " engines.");
}


Phi3EnginePool::~Phi3EnginePool() {
    shutdown();
}

void Phi3EnginePool::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_engine_queue.empty()) {
        m_engine_queue.pop();
    }
    m_free_engine = 0;
    m_max_engine = 0;
    LOG_INFO( BASE_TEXT + "Phi3引擎池已关闭。");
}