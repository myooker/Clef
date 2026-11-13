#ifndef CLEF_LIBRARY_H
#define CLEF_LIBRARY_H

#include <filesystem>
#include <stop_token>
#include <string>

#include <SQLiteCpp/SQLiteCpp.h>

namespace clef::storage {
    class Library {
    private:
        SQLite::Database m_database;
    public:
        explicit Library(const std::string &databasePath)
            : m_database { databasePath, SQLite::OPEN_READWRITE }
        {
            m_database.setBusyTimeout(5000);
            m_database.exec(R"(
                CREATE TABLE IF NOT EXISTS library (
                    path        TEXT    PRIMARY KEY,
                    directory   INTEGER NOT NULL,
                    size        INTEGER NOT NULL,
                    mtime       INTEGER NOT NULL,
                    title       TEXT,
                    artist      TEXT,
                    album       TEXT,
                    genre       TEXT
                )
            )");
        }

        void scan(const std::filesystem::path &mountPoint, std::stop_token stop);
    };
}

#endif // CLEF_LIBRARY_H
