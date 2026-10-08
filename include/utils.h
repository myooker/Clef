#ifndef CLEF_UTILS_H
#define CLEF_UTILS_H

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>

#include "directoryListing.h"

namespace clef::utils {
    bool naturalLess(std::string_view l, std::string_view r);
    bool nameLess(const FileEntity &a, const FileEntity &b);
    bool entityLess(const FileEntity &a, const FileEntity &b, const DirectoryListOptions &q);
    std::optional<bool> parseBool(std::string_view a);
    std::optional<DirectoryListOptions::SortType> parseSortType(std::string_view a);
    std::string generateId(std::size_t t=16);
    std::string getExtension(const std::string &path);

}

#endif // CLEF_UTILS_H
