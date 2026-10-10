#include "../../include/database/users.h"

#include <crow/logging.h>

namespace clef::storage {
    std::optional<bool> Users::createUser(std::string_view username, std::string_view password) const {
        if (isUsernameTaken(username)) {
            return {};
        }
        SQLite::Statement createUser(m_database, R"(
            INSERT INTO users (username, password)
            VALUES (?, ?);
        )");
        int i{};
        createUser.bind(++i, username.data());
        createUser.bind(++i, password.data());
        createUser.exec();
        return true;
    }

    std::uint32_t Users::getUserId(std::string_view username) const {
        SQLite::Statement findUserId(m_database, R"(
            SELECT id FROM users WHERE username = ?;
        )");
        int i{};
        findUserId.bind(++i, username.data());
        findUserId.executeStep();
        return findUserId.getColumn(0).getInt();
    }

    std::optional<bool> Users::deleteUser(std::uint32_t id) const {
        SQLite::Statement deleteUser(m_database, R"(
            DELETE FROM users WHERE id = ?
            VALUES (?);
        )");
        int i{};
        deleteUser.bind(++i, id);
        deleteUser.exec();
        return true;
    }

    std::optional<bool> Users::deleteUser(std::string_view username) const {
        SQLite::Statement deleteUser(m_database, R"(
            DELETE FROM users WHERE username = ?
            VALUES (?);
        )");
        int i{};
        deleteUser.bind(++i, username.data());
        deleteUser.exec();
        return true;
    }

    bool Users::isUsernameTaken(std::string_view username) const {
        SQLite::Statement findUser(m_database, R"(
            SELECT 1 FROM users
            WHERE username LIKE ?
        )");
        int i{};
        findUser.bind(++i, username.data());
        return findUser.executeStep();
    }

    bool Users::validatePassword(std::string_view username, std::string_view password) const {
        SQLite::Statement findUser(m_database, R"(
            SELECT password FROM users
            WHERE username = ?
        )");
        int i{};
        findUser.bind(++i, username.data());
        if (findUser.executeStep()) {
            return findUser.getColumn(0).getString() == password.data();
        }
        return false;
    }
}
