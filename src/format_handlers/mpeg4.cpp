#include "mpeg4.h"

#include <crow/logging.h>
#include <mp4file.h>

#include "../tags/mp4.h"

using namespace clef::music::handler;

std::expected<std::string, std::string> Mpeg4::getArtist(std::string_view filepath) {
    TagLib::MP4::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->artist().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Mpeg4::getAlbum(std::string_view filepath) {
    TagLib::MP4::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->album().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Mpeg4::getTitle(std::string_view filepath) {
    TagLib::MP4::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->title().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Mpeg4::getGenre(std::string_view filepath) {
    TagLib::MP4::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->genre().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<json, std::string> Mpeg4::listMusicTags(const std::string &filePath) {
    using namespace clef::music;
    const TagLib::MP4::File file { filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }

    if (file.hasMP4Tag()) {
        json j = json::object();
        j["mp4"] = mp4::listMusicTags(file.tag());
        return j;
    }
    CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " does not have mp4 tags";
    return std::unexpected(filePath + " does not have mp4 tags");
}

crow::response Mpeg4::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::MP4::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "Not valid"};
    }

    if (!file.hasMP4Tag()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " does not have mp4 tags";
        return {500, "does not have mp4 tags"};
    }

    auto *tag = file.tag();

    mp4::removeMusicTag(tag, tagStruct);

    if (clefId) mp4::ensureClefId(tag, clefId);
    file.save();

    return {200, "OK"};
}

crow::response Mpeg4::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::MP4::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "Not valid"};
    }

    if (!file.hasMP4Tag()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " does not have mp4 tags";
        return {500, "does not have mp4 tags"};
    }

    auto resolve = resolveTag(tagStruct.fieldType, tagStruct.formatType);
    if (!resolve.has_value()) {
        CROW_LOG_ERROR << resolve.error();
        return crow::response { 400, resolve.error() };
    }
    auto *tag = file.tag();
    const auto result = mp4::addMusicTag(tag, tagStruct, resolve.value());
    if (!result) {
        return {400, result.error()};
    }

    if (clefId) mp4::ensureClefId(tag, clefId);
    if (file.save()) {
        CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has been saved!";
        return { 200, "OK" };
    }

    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has not been saved :(";
    return { 500, "Has not been saved" };
}

crow::response Mpeg4::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::MP4::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " is not valid";
        return {500, "Not valid"};
    }

    if (!file.hasMP4Tag()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << tagStruct.filePath << " does not have mp4 tags";
        return {500, "does not have mp4 tags"};
    }

    auto resolve = resolveTag(tagStruct.fieldType, tagStruct.formatType);
    if (!resolve.has_value()) {
        CROW_LOG_ERROR << resolve.error();
        return crow::response { 400, resolve.error() };
    }
    auto *tag = file.tag();
    const auto result = mp4::editMusicTags(tag, tagStruct, resolve.value());
    if (!result) {
        return {400, result.error()};
    }

    if (clefId) mp4::ensureClefId(tag, clefId);
    if (file.save()) {
        CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has been saved!";
        return { 200, "OK" };
    }
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has not been saved :(";
    return { 500, "Has not been saved" };
}

std::expected<std::string, std::string> Mpeg4::resolveTag(const std::string_view tag, std::string_view) {
    return mp4::resolveTag(tag);
}
