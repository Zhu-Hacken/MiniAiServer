#include "phi3_engine_raii.h"
#include "phi3_engine_pool.h"

Phi3EngineRAII::Phi3EngineRAII() {
    m_engine = Phi3EnginePool::getInstance().getEngine();
}

Phi3EngineRAII::~Phi3EngineRAII() {
    if (m_engine) {
        Phi3EnginePool::getInstance().releaseEngine(m_engine);
    }
}

Phi3Engine* Phi3EngineRAII::operator->() const {
    return m_engine.get();
}