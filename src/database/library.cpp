#include "../../include/database/library.h"

#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/inotify.h>
#include <unistd.h>

#include <crow/logging.h>
#include <fileref.h>

#include "../format_handlers/factory.h"

namespace fs = std::filesystem;

namespace clef::storage {
    struct Entry {
        std::string path;
        bool directory {};
        std::int64_t size {};
        std::int64_t mtime {};
        std::string title;
        std::string artist;
        std::string album;
        std::string genre;
    };

    void Library::scan(const fs::path &mountPoint, std::stop_token stop) {
        CROW_LOG_WARNING << "Library scan started on " << mountPoint;

        std::unordered_map<std::string, std::pair<std::int64_t, std::int64_t>> stored;
        SQLite::Statement select { m_database, "SELECT path, size, mtime FROM library" };
        while (select.executeStep())
            stored[select.getColumn(0).getString()] = { select.getColumn(1).getInt64(), select.getColumn(2).getInt64() };

        std::vector<Entry> changed;

        std::error_code error;
        fs::recursive_directory_iterator it { mountPoint, fs::directory_options::skip_permission_denied, error };
        for (; it != fs::recursive_directory_iterator(); it.increment(error)) {
            if (stop.stop_requested()) return;
            if (it->is_symlink()) continue;

            Entry entry { .path = it->path().string(), .directory = it->is_directory() };
            if (!entry.directory)
                entry.size = static_cast<std::int64_t>(it->file_size());
            entry.mtime = std::chrono::file_clock::to_sys(it->last_write_time()).time_since_epoch().count();

            if (const auto found = stored.find(entry.path); found != stored.end()) {
                const bool same = found->second == std::pair { entry.size, entry.mtime };
                stored.erase(found);
                if (same) continue;
            }

            const auto handler = music::handler::Factory::create(it->path().extension().string());
            if (handler && !TagLib::FileRef { entry.path.c_str() }.isNull()) {
                entry.title = handler->getTitle(entry.path).value_or("");
                entry.artist = handler->getArtist(entry.path).value_or("");
                entry.album = handler->getAlbum(entry.path).value_or("");
                entry.genre = handler->getGenre(entry.path).value_or("");
            }
            CROW_LOG_DEBUG << "Library scan: " << entry.path;
            changed.push_back(std::move(entry));
        }

        if (error) {
            CROW_LOG_ERROR << "Library scan stopped at " << mountPoint << ": " << error.message();
            return;
        }

        SQLite::Transaction transaction { m_database };
        SQLite::Statement insert { m_database,
            "INSERT OR REPLACE INTO library (path, directory, size, mtime, title, artist, album, genre) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)" };
        for (const auto &entry : changed) {
            insert.bind(1, entry.path);
            insert.bind(2, entry.directory ? 1 : 0);
            insert.bind(3, entry.size);
            insert.bind(4, entry.mtime);
            insert.bind(5, entry.title);
            insert.bind(6, entry.artist);
            insert.bind(7, entry.album);
            insert.bind(8, entry.genre);
            insert.exec();
            insert.reset();
        }
        SQLite::Statement remove { m_database, "DELETE FROM library WHERE path = ?" };
        for (const auto &[path, values] : stored) {
            remove.bind(1, path);
            remove.exec();
            remove.reset();
        }
        transaction.commit();

        CROW_LOG_INFO << "Library scan: " << changed.size() << " changed, " << stored.size() << " removed";
    }

    void Library::watch(const std::filesystem::path &mountPoint, std::stop_token stop) {
        int fd = inotify_init();
        const uint32_t mask = IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM | IN_MOVED_TO;
        inotify_add_watch(fd, mountPoint.c_str(), mask);
        char buff [4096];

        for (const auto &entry : fs::recursive_directory_iterator(mountPoint)) {
            if (entry.is_directory() && !entry.is_symlink()) {
                CROW_LOG_WARNING << "Adding watcher to " << entry.path().c_str();
                inotify_add_watch(fd, entry.path().c_str(), mask);
            }
        }
        CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " has started.";
        while (!stop.stop_requested()) {
            ssize_t bytesRead = read(fd, buff, sizeof(buff));
            ssize_t offset = 0;
            while (offset < bytesRead) {
                auto *event = reinterpret_cast<struct inotify_event*>(buff + offset);
                if (event->mask & IN_CREATE)
                    CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " - created";
                if (event->mask & IN_DELETE)
                    CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " - deleted";
                if (event->mask & IN_MODIFY)
                    CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " - modified";
                if (event->mask & IN_MOVED_FROM)
                    CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " - moved from";
                if (event->mask & IN_MOVED_TO)
                    CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " - moved to";
                offset += sizeof(inotify_event) + event->len;
            }
        }
    }
}
