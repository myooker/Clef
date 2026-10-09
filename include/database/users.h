#ifndef CLEF_USERSDATABASE_H
#define CLEF_USERSDATABASE_H

#include <SQLiteCpp/Database.h>

namespace clef::storage {
    class Users {
    private:
        static constexpr std::string_view m_tableName { "users" };
        SQLite::Database &m_database;
    public:
        explicit Users(SQLite::Database &database)
            : m_database(database) {
            m_database.exec(R"(
                CREATE TABLE IF NOT EXISTS users (
                    id           INTEGER  PRIMARY KEY,
                    username     TEXT     NOT NULL,
                    password     TEXT     NOT NULL
                )
            )");
        }

        std::optional<bool> createUser(std::string_view username, std::string_view password) const;
        std::optional<bool> deleteUser(std::uint32_t id) const;
        std::optional<bool> deleteUser(std::string_view username) const;
        bool isUsernameTaken(std::string_view username) const;
        std::optional<bool> updateUser() const;
    };
}

#endif // CLEF_USERSDATABASE_H
