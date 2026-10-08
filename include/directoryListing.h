#ifndef CLEF_DIRECTORYLISTING_H
#define CLEF_DIRECTORYLISTING_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace clef {
    enum class EntityType {
        directory,
        music,
        picture,
        file,

        max_type
    };

    struct DirectoryListOptions {
        enum class SortType { name, size, type, MAXSORT };
        const std::size_t offset { 0 };
        const std::size_t limit { 100 };
        const bool ascending { true };
        const SortType sort { SortType::name };
    };

    struct FileEntity {
        std::string name {};
        std::string ext {};
        std::uintmax_t size {};
        //modified
        EntityType type { EntityType::directory };

        std::string typeString() const {
            switch (type) {
            case EntityType::directory: return "directory";
            case EntityType::music:     return "music";
            case EntityType::picture:   return "picture";
            case EntityType::file:      return "file";
            default:                    return "file";
            }
        }
    };
}

#endif // CLEF_DIRECTORYLISTING_H
