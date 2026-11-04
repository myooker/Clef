#ifndef CLEF_ID3V1_H
#define CLEF_ID3V1_H

#include <expected>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>
#include <id3v1tag.h>

#include "../../include/tagChangeRequest.h"

namespace clef::music::id3v1 {
    using json = nlohmann::json;

    json listMusicTags(const TagLib::ID3v1::Tag *tag);
    void removeMusicTag(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct);
    void addMusicTag(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct);
    void editMusicTags(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct);
}

#endif //CLEF_ID3V1_H
