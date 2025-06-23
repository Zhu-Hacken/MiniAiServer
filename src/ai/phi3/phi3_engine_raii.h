#pragma once

#include "ai/phi3/phi3_engine.h"
#include <memory>
class Phi3EngineRAII {
public:
    Phi3EngineRAII();
    ~Phi3EngineRAII();

    Phi3Engine* operator->() const;

private:
    std::shared_ptr<Phi3Engine> m_engine;
};