#include "oggVorbis.h"

#include <cstddef>
#include <utility>

#include <crow/logging.h>
#include <vorbisfile.h>

#include "../../include/tagConstants.h"
#include "../../include/tagMapping.h"

using namespace clef::music::handler;
using namespace clef::music::tag;

void OggVorbis::ensureClefId(std::string *clefId, TagLib::Ogg::XiphComment *tag) {
    using namespace clef::music::tag;
    using namespace TagLib;

    const auto fieldList = tag->fieldListMap();
    const auto clefIdIt = fieldList.find(String(tag::clefId.data()).upper());
    if (clefIdIt != fieldList.end()) {
        *clefId = clefIdIt->second[0].toCString(false);
    } else {
        const auto rteIdIt = fieldList.find(rteId.data());
        if (rteIdIt != fieldList.end()) {
            *clefId = rteIdIt->second[0].toCString(false);
            tag->removeFields(rteId.data());
        }
        tag->addField(tag::clefId.data(), *clefId, true);
    }
}

std::expected<json, std::string> OggVorbis::listMusicTags(const std::string &filePath) {
    TagLib::Vorbis::File file{filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }

    json j;
    const auto tag = file.tag();
    for (const auto &[key, values]: tag->fieldListMap()) {
        if (values.size() > 1) {
            const std::size_t temp{values.size()};
            for (std::size_t i{0}; i < temp; ++i) {
                std::string value{values[i].to8Bit(true)};
                j[key.to8Bit(true)] += value;
            }
            continue;
        }
        std::string value{values[0].to8Bit(true)};
        j[key.to8Bit(true)] += value;
    }
    CROW_LOG_DEBUG << __PRETTY_FUNCTION__ << ": returning JSON";
    return j;
}

crow::response OggVorbis::removeMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
    TagLib::Ogg::Vorbis::File file{tagStruct.filePath.c_str()};
    if (!file.isValid()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }

    auto *tag = file.tag();
    const auto f_it = tag->fieldListMap().find(tagStruct.fieldType);

    // Save values of key (tagStruct.fieldType) to values
    TagLib::StringList values {};
    if (f_it != tag->fieldListMap().end()) {
        values = f_it->second;
    } else {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.fieldType << " was not found in file "
            << tagStruct.filePath;
        return { 500, "Error" };
    }

    // Find occurrence of tagStruct.value. If so, delete it.
    if (const auto values_it = values.find(tagStruct.value); values_it != values.end()) {
        values.erase(values_it);
    } else {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.value << " was not found in file "
            << tagStruct.filePath;
        return { 500, "Specified value was not found!" };
    }

    // Remove all values (fields) of a specified key (tagStruct.fieldType)
    // And fill the key with values
    tag->removeFields(tagStruct.fieldType);
    for (const auto &s : values) {
        tag->addField(tagStruct.fieldType, s, false);
    }
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.fieldType << " field was removed!";
    if (clefId) ensureClefId(clefId, tag);
    file.save();
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " saved!";
    return {200, "OK"};
}

crow::response OggVorbis::addMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
    TagLib::Ogg::Vorbis::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }

    auto resolve = tag::getTagMap()->resolve(tagStruct.fieldType, m_type.data());
    if (!resolve.has_value()) {
        CROW_LOG_ERROR << resolve.error();
        return crow::response { 400, resolve.error() };
    }
    const std::string &raw = resolve.value();
    auto *tag = file.tag();
    tag->addField(raw, tagStruct.value, false);
    if (clefId) ensureClefId(clefId, tag);
    file.save();
    CROW_LOG_INFO << "(" << __func__ << ") " << tagStruct.filePath << " saved!";
    return {200, "File/s saved!"};
}

crow::response OggVorbis::editMusicTags(const TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::Ogg::Vorbis::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(FLAC::" << __func__ << ".multi) " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }

    auto *tag = file.tag();
    const auto filedType_it = tag->fieldListMap().find(tagStruct.fieldType);
    TagLib::StringList values {}; // Here we store old values of a music file

    // Check whether tagStruct.tagType was found
    // If yes, fill StringList oldValues with tagStruct.values
    if (filedType_it != tag->fieldListMap().end()) {
        values = filedType_it->second; // Get an array of old values inside music file
    } else {
        CROW_LOG_ERROR << "(FLAC::" << __func__ << ".multi) " << tagStruct.fieldType << " was not found in " << tagStruct.filePath;
        return { 500, "Field type does not exist" };
    }

    if (const auto v_it = values.find(tagStruct.replaceWhat); v_it != values.end()) {
        *v_it = tagStruct.replaceWith;
    } else {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.replaceWhat << " was not found in file "
            << tagStruct.filePath;
        return { 500, "Specified value was not found in a file. Check logs" };
    }

    // After filling up StringList newValues, we need to clear current tags inside a file
    // Then we write newValues to requested tag field (tagStruct.tagType) without replacing.
    tag->removeFields(tagStruct.fieldType);
    for (const auto &a : values) {
        tag->addField(tagStruct.fieldType, a, false);
        CROW_LOG_INFO << "(FLAC::" << __func__ << ".multi) " << tagStruct.fieldType << " of " << tagStruct.filePath << " has changed to " << a.toCString();
    }
    if (clefId) ensureClefId(clefId, tag);
    file.save();
    CROW_LOG_INFO << "(FLAC::" << __func__ << ".multi) " << tagStruct.filePath << " saved!\n";

    return { 200, "OK" };
}

std::expected<std::string, std::string> OggVorbis::resolveTag(const std::string_view tag) {
    auto resolve = getTagMap()->resolve(tag.data(), m_type.data());
    if (resolve.has_value())
        return resolve.value();
    return std::unexpected(std::move(resolve).error());
}
