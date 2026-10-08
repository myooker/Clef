#ifndef CLEF_USERSDATABASE_H
#define CLEF_USERSDATABASE_H

#include <SQLiteCpp/Database.h>

namespace clef::storage {
    class Users {
    private:
        SQLite::Database &m_database;
    public:
        explicit Users(SQLite::Database &database)
            : m_database(database) {
        }

    };
}

#endif // CLEF_USERSDATABASE_H
