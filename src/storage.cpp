#include "../include/tagHistoryDatabase.h"

namespace clef::storage {
    crow::response TagHistory::insertAdd(const TagChangeRequest &tagStruct, const id &idStruct) const {
        int i {};
        SQLite::Statement query(m_database,
        "INSERT INTO tag_history (clefId, action, path, tag, new_value) "
        "VALUES (?, ?, ?, ?, ?)");
        query.bind(++i, idStruct.clefId);
        query.bind(++i, add.begin());
        query.bind(++i, tagStruct.filePath);
        query.bind(++i, tagStruct.fieldType);
        query.bind(++i, tagStruct.value.to8Bit(true));
        query.exec();

        return crow::response{ 200 };
    }

    crow::response TagHistory::insertEdit(const TagChangeRequest &tagStruct, const id &idStruct) const {
        int i {};
        SQLite::Statement query(m_database,
        "INSERT INTO tag_history (clefId, action, path, tag, old_value, new_value) "
        "VALUES (?, ?, ?, ?, ?, ?)");
        query.bind(++i, idStruct.clefId);
        query.bind(++i, change.begin());
        query.bind(++i, tagStruct.filePath);
        query.bind(++i, tagStruct.fieldType);
        query.bind(++i, tagStruct.replaceWhat.to8Bit(true));
        query.bind(++i, tagStruct.replaceWith.to8Bit(true));
        query.exec();

        return crow::response{ 200 };
    }

    crow::response TagHistory::insertRemove(const TagChangeRequest &tagStruct, const id &idStruct) const {
        int i {};
        SQLite::Statement query(m_database,
            "INSERT INTO tag_history (clefId, action, path, tag, old_value) "
        "VALUES (?, ?, ?, ?, ?)");
        query.bind(++i, idStruct.clefId);
        query.bind(++i, remove.begin());
        query.bind(++i, tagStruct.filePath);
        query.bind(++i, tagStruct.fieldType);
        query.bind(++i, tagStruct.value.to8Bit(true));
        query.exec();

        return crow::response{ 200 };
    }

    crow::response TagHistory::deleteFile(const std::string& path) const {
        SQLite::Statement deletePath(m_database,
        "DELETE FROM tag_history WHERE path = ?");
        deletePath.bind(1, path);
        deletePath.exec();

        return crow::response{ 200 };
    }
} // program