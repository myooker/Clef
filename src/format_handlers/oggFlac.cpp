#include "oggFlac.h"

#include <crow/logging.h>
#include <oggflacfile.h>

#include "../tags/xiph.h"

using namespace clef::music::handler;
namespace xiph = clef::music::xiph;

std::expected<json, std::string> OggFlac::listMusicTags(const std::string &filePath) {
    TagLib::Ogg::FLAC::File file{filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }
    if (file.hasXiphComment()) {
        json j = json::object();
        const auto tag = file.tag();
        j["xiph"] = xiph::listMusicTags(tag);
        return j;
    }
    CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " does not have Xiph Comments";
    return std::unexpected(filePath + " does not have Xiph Comments");
}

crow::response OggFlac::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::Ogg::FLAC::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }
    if (!file.hasXiphComment()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " does not have Xiph Comments";
        return {500, "The file does not have Xiph Comments"};
    }

    auto *tag = file.tag();
    const auto result = xiph::removeMusicTag(tag, tagStruct);
    if (!result) {
        return {500, result.error()};
    }
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " saved!";
    return {200, "OK"};
}

crow::response OggFlac::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    TagLib::Ogg::FLAC::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }
    if (!file.hasXiphComment()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " does not have Xiph Comments";
        return {500, "The file does not have Xiph Comments"};
    }

    auto resolve = resolveTag(tagStruct.fieldType, tagStruct.formatType);
    if (!resolve.has_value()) {
        CROW_LOG_ERROR << resolve.error();
        return crow::response { 400, resolve.error() };
    }
    auto *tag = file.tag();
    xiph::addMusicTag(tag, tagStruct, resolve.value());
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();
    CROW_LOG_INFO << "(" << __func__ << ") " << tagStruct.filePath << " saved!";
    return {200, "File/s saved!"};
}

crow::response OggFlac::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::Ogg::FLAC::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(OggFlac::" << __func__ << ".multi) " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }
    if (!file.hasXiphComment()) {
        CROW_LOG_ERROR << "(OggFlac::" << __func__ << ".multi) " << tagStruct.filePath << " does not have Xiph Comments";
        return { 500, "The file does not have Xiph Comments"};
    }

    auto *tag = file.tag();
    const auto result = xiph::editMusicTags(tag, tagStruct);
    if (!result)
        return { 500, result.error() };
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();

    return { 200, "OK" };
}

std::expected<std::string, std::string> OggFlac::resolveTag(const std::string_view tag, std::string_view) {
    return clef::music::xiph::resolveTag(tag);
}
