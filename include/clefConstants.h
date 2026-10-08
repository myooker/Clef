#ifndef CLEF_CLEFCONSTANTS_H
#define CLEF_CLEFCONSTANTS_H

#include <string_view>

namespace clef {
    constexpr std::string_view version { "0.0.1" };
    constexpr std::string_view name { "Clef" };
    constexpr std::string_view jsonMissingValue { "__json_missing_value" };
    constexpr std::string_view defaultDatabasePath { "data/database.db" };
    constexpr std::string_view defaultMappingPath { "data/mapping.json" };
    constexpr std::string_view defaultMountPoint { "/music" };
    constexpr bool defaultClefIdStatus { false };
    constexpr int defaultPort { 18080 };
}

#endif // CLEF_CLEFCONSTANTS_H
