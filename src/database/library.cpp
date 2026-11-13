#include "../../include/database/library.h"

#include <chrono>
#include <cstdint>
#include <vector>

#include <crow/logging.h>

namespace fs = std::filesystem;

namespace clef::storage {
    struct Entry {
        std::string path;
        bool directory {};
        std::int64_t size {};
        std::int64_t mtime {};
    };

    void Library::scan(const fs::path &mountPoint, std::stop_token stop) {
        std::vector<Entry> entries;

        std::error_code error;
        fs::recursive_directory_iterator it { mountPoint, fs::directory_options::skip_permission_denied, error };
        for (; it != fs::recursive_directory_iterator(); it.increment(error)) {
            if (stop.stop_requested()) return;
            if (it->is_symlink()) continue;

            Entry entry { .path = it->path().string(), .directory = it->is_directory() };
            if (!entry.directory)
                entry.size = static_cast<std::int64_t>(it->file_size());
            entry.mtime = std::chrono::file_clock::to_sys(it->last_write_time()).time_since_epoch().count();
            entries.push_back(std::move(entry));
        }

        SQLite::Transaction transaction { m_database };
        SQLite::Statement insert { m_database,
            "INSERT OR REPLACE INTO library (path, directory, size, mtime) VALUES (?, ?, ?, ?)" };
        for (const auto &entry : entries) {
            insert.bind(1, entry.path);
            insert.bind(2, entry.directory ? 1 : 0);
            insert.bind(3, entry.size);
            insert.bind(4, entry.mtime);
            insert.exec();
            insert.reset();
        }
        transaction.commit();

        CROW_LOG_INFO << "Library scan: " << entries.size() << " entries";
    }
}
