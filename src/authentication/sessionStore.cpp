#include "../../include/authentication/sessionStore.h"
#include "../../include/utils.h"

namespace clef {
    std::string SessionStore::createSession(std::uint32_t id) {
        std::string sessionId { utils::generateId(64) };
        std::lock_guard lock { m_sessionsMutex };
        m_sessions[sessionId] = { .id = id };
        return sessionId;
    }

    bool SessionStore::validateSession(std::string_view sessionToken) {
        std::shared_lock lock { m_sessionsMutex };
        auto it = m_sessions.find(sessionToken.data());
        if (it != m_sessions.end())
            return true;
        return false;
    }
}
