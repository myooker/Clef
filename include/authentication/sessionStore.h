#ifndef CLEF_SESSIONSTORE_H
#define CLEF_SESSIONSTORE_H

#include <chrono>
#include <cstdint>
#include <unordered_map>

namespace clef {
    struct Session {
        std::uint64_t id {};
        std::chrono::steady_clock::time_point expiresAt;
    };

    class SessionStore {
    private:
        std::unordered_map<std::string, Session> sessions;
    public:

    };
}



#endif //CLEF_SESSIONSTORE_H
