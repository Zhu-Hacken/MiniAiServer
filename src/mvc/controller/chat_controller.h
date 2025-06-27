#pragma once

#include "global_router.h"
#include "http_request.h"
#include "http_response.h"
#include "session_manager.h"
#include "websocket/websocket_conn.h"
#include <string>

class ChatController {
public:
    static void registerRoutes() {
        GlobalRouter::getInstance().registerPost("/ai/history", getHistory);
        GlobalRouter::getInstance().registerPost("/ai/chat", chat);
        GlobalRouter::getInstance().registerWebSocket("/ai/streamInfer", streamInfer);

    }

private:
    static void chat(HttpRequest& http_request, HttpResponse& http_response);
    static void getHistory(HttpRequest& http_request, HttpResponse& http_response);
    static void streamInfer(WebSocketConn& conn, SessionId& sessionId, const std::string& msg);

};