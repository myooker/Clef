#include "flac.h"

#include <crow/logging.h>
#include <flacfile.h>
#include <flacpicture.h>

#include "../tags/xiph.h"

using namespace clef::music::handler;
using namespace clef::music::tag;

std::expected<std::string, std::string> Flac::getArtist(std::string_view filepath) {
    if (TagLib::FLAC::File file { filepath.data() }; file.isValid())
        return file.xiphComment()->artist().to8Bit(true);
    return std::unexpected(std::string(filepath) + " is not valid");
}

std::expected<std::string, std::string> Flac::getAlbum(std::string_view filepath) {
    if (TagLib::FLAC::File file { filepath.data() }; file.isValid())
        return file.xiphComment()->album().to8Bit(true);
    return std::unexpected(std::string(filepath) + " is not valid");
}

std::expected<std::string, std::string> Flac::getTitle(std::string_view filepath) {
    if (TagLib::FLAC::File file { filepath.data() }; file.isValid())
        return file.xiphComment()->title().to8Bit(true);
    return std::unexpected(std::string(filepath) + " is not valid");
}

std::expected<std::string, std::string> Flac::getGenre(std::string_view filepath) {
    if (TagLib::FLAC::File file { filepath.data() }; file.isValid())
        return file.xiphComment()->genre().to8Bit(true);
    return std::unexpected(std::string(filepath) + " is not valid");
}

std::expected<json, std::string> Flac::listMusicTags(const std::string &filePath) {
    TagLib::FLAC::File file { filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }
    if (file.hasXiphComment()) {
        json j = json::object();
        const auto tag = file.xiphComment();
        j["xiph"] = xiph::listMusicTags(tag);
        return j;
    }
    CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " does not have Xiph Comments";
    return std::unexpected(filePath + " does not have Xiph Comments");
}

crow::response Flac::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::FLAC::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
        return {500, "The file is not valid"};
    }
    if (!file.hasXiphComment()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " does not have Xiph Comments";
        return {500, "The file does not have Xiph Comments"};
    }

    auto *tag = file.xiphComment();
    const auto result = xiph::removeMusicTag(tag, tagStruct);
    if (!result) {
        return {500, result.error()};
    }
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " saved!";
    return {200, "OK"};
}

crow::response Flac::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::FLAC::File file { tagStruct.filePath.c_str() };
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
    auto *tag = file.xiphComment();
    xiph::addMusicTag(tag, tagStruct, resolve.value());
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();
    CROW_LOG_INFO << "(" << __func__ << ") " << tagStruct.filePath << " saved!";
    return {200, "File/s saved!"};
}

crow::response Flac::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::FLAC::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(FLAC::" << __func__ << ".multi) " << tagStruct.filePath << " is not valid";
        return { 500, "The file is not valid" };
    }
    if (!file.hasXiphComment()) {
        CROW_LOG_ERROR << "(FLAC::" << __func__ << ".multi) " << tagStruct.filePath << " does not have Xiph Comments";
        return { 500, "The file does not have Xiph Comments"};
    }

    auto *tag = file.xiphComment();
    const auto result = xiph::editMusicTags(tag, tagStruct);
    if (!result) {
        return {500, result.error()};
    }
    if (clefId) xiph::ensureClefId(tag, clefId);
    file.save();

    return { 200, "OK" };
}

Picture Flac::getAlbumCover(const std::string &filePath) {
    using namespace clef::music;

    TagLib::FLAC::File file { filePath.c_str() };

    if (file.pictureList().isEmpty()) {
        return Picture {
            crow::response {500, "Not found" },
            "", "", 0, 0
        };
    }

    auto p = file.pictureList()[0];

    Picture picture {
        crow::response {200, "OK" },
        p->mimeType().toCString(),
        std::string(p->data().data(), p->data().size()),
        p->height(),
        p->width(),
    };

    return picture;
}

std::expected<std::string, std::string> Flac::resolveTag(const std::string_view tag, std::string_view) {
    return xiph::resolveTag(tag);
}
