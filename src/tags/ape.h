#ifndef CLEF_APE_H
#define CLEF_APE_H

#include <apetag.h>
#include <expected>
#include <nlohmann/json.hpp>

#include "../../include/tagChangeRequest.h"

namespace clef::music::ape {
    using json = nlohmann::json;
    using namespace TagLib;

    json listMusicTags(const APE::Tag *tag);
    void ensureClefId(APE::Tag *tag, std::string *clefid);
    void removeMusicTag(APE::Tag *tag, const TagChangeRequest &tagStruct);
    void addMusicTag(APE::Tag *tag, const TagChangeRequest &tagStruct);
    void editMusicTags(APE::Tag *tag, const TagChangeRequest &tagStruct);
    std::expected<std::string, std::string> resolveTag(std::string_view tag);
}

#endif //CLEF_APE_H
