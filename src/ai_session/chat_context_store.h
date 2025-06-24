#pragma once

#include "session_manager.h"
#include "timer_manager.h"
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


// 每个用户的上下文结构
struct ChatContext {
    std::vector<std::pair<std::string, std::string>> historyText;   // 每轮 <user, ai>
    TimePoint lastUsed = std::chrono::steady_clock::now();
};


class ChatContextStore {
public:
    static ChatContextStore& getInstance();

    ChatContext& getContext(const SessionId& sessionId, int timeout_ms = 15 * 60 * 1000);
    void removeSession(const SessionId& sessionId);
    bool hasContext(const SessionId& sessionId) const;
    void cleanExpiredContext(const SessionId& sessionId);
    void refresh(const SessionId& sessionId, int timeout_ms = 15 * 60 * 1000);

private:
    ChatContextStore() = default;
    ~ChatContextStore() = default;

    ChatContextStore(const ChatContextStore&) = delete;
    ChatContextStore& operator=(const ChatContextStore&) = delete;

    mutable std::mutex m_mutex;
    std::unordered_map<SessionId, ChatContext> m_store;

    TimerManager m_timer_manager;
    std::unordered_map<SessionId, int>  m_session_timers;
    static int timer_seed;  // 下一个可用的逻辑 定时器id
};