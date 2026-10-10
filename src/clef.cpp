#include "../include/clef.h"

namespace clef {
    bool Application::isExist(std::string_view path) const {
        return std::filesystem::exists(path);
    }

    bool Application::isMountPoint(std::string_view rpath) const {
        const std::string mp{ canonical(m_mountPoint) }; // canonical mount point
        const std::string rp{fs::canonical(rpath)}; // canonical requested path

        if (rp.starts_with(mp)) {
            return true;
        }

        return false;
    }

    std::string Application::userLogin(std::string_view username, std::string_view password) {
        if (m_users.validatePassword(username, password)) {
            auto id = m_users.getUserId(username);
            //CROW_LOG_WARNING << "id: " << id;
            return m_sessionStore.createSession(id);
        }
        return "";
    }
}
