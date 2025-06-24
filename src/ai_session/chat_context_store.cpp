#include "chat_context_store.h"
#include "log_utils.h"
#include "session_manager.h"
#include <chrono>
#include <mutex>
#include <string>
#include "log/logs.h"

const std::string BASE_TEXT = "[ChatContextStore] ";

int ChatContextStore::timer_seed = 10000;

ChatContextStore& ChatContextStore::getInstance() {
    static ChatContextStore instance;
    return instance;
}

ChatContext& ChatContextStore::getContext(const SessionId& sessionId, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_timer_manager.tick();
    // auto& ctx = m_store[sessionId];
    auto it = m_store.find(sessionId);
    
    int timer_id = -1;

    if (it == m_store.end()) {
        m_store[sessionId];
        timer_id = timer_seed++;
        m_session_timers[sessionId] = timer_id;
        it = m_store.find(sessionId);
    } else {
        timer_id = m_session_timers.find(sessionId)->second;
    }



    m_timer_manager.addTimer(timer_id, [this, sessionId]() {
        this->cleanExpiredContext(sessionId);
    }, timeout_ms);

    // ctx.lastUsed = time(nullptr);
    return it->second;    
}

void ChatContextStore::removeSession(const SessionId& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it_s = m_store.find(sessionId);
    if (it_s != m_store.end()) {
        m_store.erase(it_s);
    }

    auto it_st = m_session_timers.find(sessionId);
    if (it_st != m_session_timers.end()) {
        m_session_timers.erase(it_st);
    }

}

bool ChatContextStore::hasContext(const SessionId& sessionId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_store.find(sessionId) != m_store.end();
}

void ChatContextStore::cleanExpiredContext(const SessionId& sessionId) {
    removeSession(sessionId);
    LOG_INFO(BASE_TEXT + "Context expired and cleaned: " + sessionId);
}

void ChatContextStore::refresh(const SessionId& sessionId, int timeout_ms) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_store.find(sessionId);
    if (it == m_store.end()) return;
    
    // 更新上一次使用时间
    it->second.lastUsed = std::chrono::steady_clock::now();

    auto it_timer = m_session_timers.find(sessionId);

    if (it_timer == m_session_timers.end()) return;

    int timer_id = it_timer->second;

    // 重新添加定时器
    m_timer_manager.addTimer(timer_id, [this, sessionId]() {
        this->cleanExpiredContext(sessionId);
    }, timeout_ms);
    LOG_INFO(BASE_TEXT + "续期 session_id = " + sessionId + " 的上下文缓存.");

}