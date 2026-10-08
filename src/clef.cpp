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
}
