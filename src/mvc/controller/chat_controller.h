#pragma once

#include "global_router.h"
#include "http_request.h"
#include "http_response.h"
class ChatController {
public:
    static void registerRoutes() {
        GlobalRouter::getInstance().registerPost("/ai/chat", chat);
    }

private:
    static void chat(HttpRequest& http_request, HttpResponse& http_response);
};