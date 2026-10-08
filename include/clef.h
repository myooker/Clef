#ifndef CLEF_CLEF_H
#define CLEF_CLEF_H

#include <filesystem>
#include <string_view>

#include <SQLiteCpp/Database.h>

#include "clefConstants.h"
#include "tagHistoryDatabase.h"
#include "usersDatabase.h"
#include "tagMapping.h"

namespace clef {
    namespace fs = std::filesystem;

    namespace environments {
        constexpr std::string_view useClefId { "CLEF_USEID" };
    }

    class Application {
    private:
        fs::path m_databasePath { "data/database.db" };
        SQLite::Database m_database;
        storage::Users m_users;
        storage::TagHistory m_tagHistory;
        fs::path m_mappingPath { "data/mapping.json" };
        fs::path m_mountPoint { "/music" };
        bool m_useClefId { false };
        int m_port { 18080 };

        [[nodiscard]] bool isExist(std::string_view path) const;
    public:
        Application() = delete;

        explicit
        Application(std::string_view databasePath = defaultDatabasePath,
                    std::string_view mappingPath = defaultMappingPath,
                    std::string_view mountPoint = defaultMountPoint,
                    bool useClefId = defaultClefIdStatus,
                    int port = defaultPort) :
        m_databasePath { databasePath },
        m_database { m_databasePath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE },
        m_users { m_database },
        m_tagHistory { m_database },
        m_mappingPath { mappingPath },
        m_mountPoint { mountPoint },
        m_useClefId { useClefId },
        m_port { port } {
            if (!isExist(m_mountPoint.c_str())) {
                throw std::runtime_error ("Mount point does not exist: " + m_mountPoint.string());
            }
        }

        Application(Application &&) = delete;
        Application& operator=(Application &&) = delete;

        [[nodiscard]] bool isMountPoint(std::string_view rpath) const;

        SQLite::Database &getDatabase() { return m_database; }
        [[nodiscard]] const storage::TagHistory &getTagHistoryDB() const { return m_tagHistory; }
        [[nodiscard]] const storage::Users &getUsersDB() const { return m_users; }
        [[nodiscard]] std::string_view getMountPoint() const { return m_mountPoint.c_str(); }
        [[nodiscard]] bool getClefIdStatus() const { return m_useClefId; }
        [[nodiscard]] int getPort() const { return m_port; }

    };

    // struct Settings {
    //     std::string mountpoint { "/music" };
    //     std::string dbpath { "data/database.db" };
    //     std::string mappingpath { "data/mapping.json" };
    //     bool useClefId { false };
    //     int port{ 18080 };
    //
    //     [[nodiscard]] bool isExist() const {
    //         const fs::path p { mountpoint };
    //         return !std::filesystem::exists(p);
    //     }
    //
    //     [[nodiscard]] bool isMountPoint(const std::string &requestedPath) const {
    //         const std::string mp { fs::canonical(mountpoint) };        // canonical mount point
    //         const std::string rp { fs::canonical(requestedPath) };     // canonical requested path
    //
    //         if (rp.starts_with(mp)) {
    //             return true;
    //         }
    //
    //         return false;
    //     }
    // };
}

#endif // CLEF_CLEF_H
