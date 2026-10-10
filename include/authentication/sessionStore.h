#ifndef CLEF_SESSIONSTORE_H
#define CLEF_SESSIONSTORE_H

#include <chrono>
#include <cstdint>
#include <shared_mutex>
#include <unordered_map>

namespace clef {
    struct Session {
        std::uint32_t id {};
        std::chrono::steady_clock::time_point expiresAt {};
    };

    class SessionStore {
    private:
        std::unordered_map<std::string, Session> m_sessions;
        std::shared_mutex m_sessionsMutex;
    public:
        std::string createSession(std::uint32_t id);
        bool validateSession(std::string_view sessionToken);
    };
}



#endif //CLEF_SESSIONSTORE_H
