#include "oggOpus.h"

#include <crow/logging.h>
#include <opusfile.h>

#include "../tags/xiph.h"

using namespace clef::music::handler;
namespace xiph = clef::music::xiph;

std::expected<std::string, std::string> OggOpus::getArtist(std::string_view filepath) {
    TagLib::Ogg::Opus::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->artist().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> OggOpus::getAlbum(std::string_view filepath) {
    TagLib::Ogg::Opus::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->album().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> OggOpus::getTitle(std::string_view filepath) {
    TagLib::Ogg::Opus::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->title().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> OggOpus::getGenre(std::string_view filepath) {
    TagLib::Ogg::Opus::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->genre().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<json, std::string> OggOpus::listMusicTags(const std::string &filePath) {
    TagLib::Ogg::Opus::File file{filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }

    json j = json::object();
    j["xiph"] = xiph::listMusicTags(file.tag());
    return j;
}

crow::response OggOpus::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    TagLib::Ogg::Opus::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
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

crow::response OggOpus::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    TagLib::Ogg::Opus::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
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

crow::response OggOpus::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::Ogg::Opus::File file{tagStruct.filePath.c_str()};

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(OggOpus::" << __func__ << ".multi) " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }

    auto *tag = file.tag();
    const auto result = xiph::editMusicTags(tag, tagStruct);
    if (!result) {
        return {500, result.error()};
    }
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();
    CROW_LOG_INFO << "(OggOpus::" << __func__ << ".multi) " << tagStruct.filePath << " saved!\n";

    return {200, "OK"};
}

std::expected<std::string, std::string> OggOpus::resolveTag(const std::string_view tag, std::string_view) {
    return clef::music::xiph::resolveTag(tag);
}
