#include "wav.h"
#include <wavfile.h>
#include "../tags/id3v2.h"
#include "../tags/riff.h"
#include "../../include/tagMapping.h"

using namespace clef::music::handler;
using namespace clef::music::tag;

std::expected<std::string, std::string> Wav::getArtist(std::string_view filepath) {
    TagLib::RIFF::WAV::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->artist().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wav::getAlbum(std::string_view filepath) {
    TagLib::RIFF::WAV::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->album().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wav::getTitle(std::string_view filepath) {
    TagLib::RIFF::WAV::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->title().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wav::getGenre(std::string_view filepath) {
    TagLib::RIFF::WAV::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->genre().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<json, std::string> Wav::listMusicTags(const std::string &filePath) {
    using namespace clef::music;
    TagLib::RIFF::WAV::File file { filePath.c_str() };

    if (!file.isValid()) {
        return std::unexpected("File is invalid");
    }

    json j = json::object();
    if (file.hasID3v2Tag()) {
        auto id3v2tag = file.ID3v2Tag();
        j["id3v2"] = id3v2::listMusicTags(id3v2tag);
    }
    if (file.hasInfoTag()) {
        auto riff = file.InfoTag();
        j["riff"] = riff::listMusicTags(riff);
    }

    return j;
}

crow::response Wav::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::RIFF::WAV::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        return crow::response { 500, "File is not valid" };
    }

    if (tagStruct.formatType == "id3v2") {
        auto *tag = file.ID3v2Tag();
        id3v2::removeMusicTag(tag, tagStruct);
    } else if (tagStruct.formatType == "riff") {
        auto resolvedTag = getTagMap()->resolve(tagStruct.fieldType, "riff");
        if (!resolvedTag.has_value()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " couldn't resolve " << tagStruct.fieldType
                           << " in " << tagStruct.filePath;
            return crow::response { 500, "Failed to resolve tag" };
        }
        riff::removeMusicTag(file.InfoTag(), tagStruct, resolvedTag.value());
    } else {
        return crow::response { 400 };
    }

    if (clefId) id3v2::ensureClefId(file.ID3v2Tag(), clefId);

    if (!file.save()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " failed to save!";
        return crow::response { 500, "Failed to save file" };
    }
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " is saved!";
    return crow::response { 200 };
}

crow::response Wav::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::RIFF::WAV::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        return crow::response { 500, "File is not valid" };
    }

    if (tagStruct.formatType == "id3v2") {
        auto resolvedTag = getTagMap()->resolve(tagStruct.fieldType, "id3v2");
        if (!resolvedTag.has_value()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " couldn't resolve " << tagStruct.fieldType
                           << " in " << tagStruct.filePath;
            return crow::response { 500, "Failed to resolve tag" };
        }
        auto *tag = file.ID3v2Tag();
        id3v2::addMusicTag(tag, tagStruct, resolvedTag.value());
    } else if (tagStruct.formatType == "riff") {
        auto resolvedTag = getTagMap()->resolve(tagStruct.fieldType, "riff");
        if (!resolvedTag.has_value()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " couldn't resolve " << tagStruct.fieldType
                           << " in " << tagStruct.filePath;
            return crow::response { 500, "Failed to resolve tag" };
        }
        riff::addMusicTag(file.InfoTag(), tagStruct, resolvedTag.value());
    } else {
        return crow::response { 400 };
    }

    if (clefId) id3v2::ensureClefId(file.ID3v2Tag(), clefId);

    if (!file.save()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " failed to save!";
        return crow::response { 500, "Failed to save file" };
    }
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " is saved!";
    return crow::response { 200 };
}

crow::response Wav::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace clef::music;
    TagLib::RIFF::WAV::File file { tagStruct.filePath.c_str() };

    if (!file.isValid()) {
        return crow::response { 500, "File is not valid" };
    }

    if (tagStruct.formatType == "id3v2") {
        auto *tag = file.ID3v2Tag();
        id3v2::editMusicTags(tag, tagStruct);
    } else if (tagStruct.formatType == "riff") {
        auto resolvedTag = getTagMap()->resolve(tagStruct.fieldType, "riff");
        if (!resolvedTag.has_value()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " couldn't resolve " << tagStruct.fieldType
                           << " in " << tagStruct.filePath;
            return crow::response { 500, "Failed to resolve tag" };
        }
        riff::editMusicTag(file.InfoTag(), tagStruct, resolvedTag.value());
    } else {
        return crow::response { 400 };
    }

    if (clefId) id3v2::ensureClefId(file.ID3v2Tag(), clefId);

    if (!file.save()) {
        CROW_LOG_ERROR << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " failed to save!";
        return crow::response { 500, "Failed to save file" };
    }
    CROW_LOG_INFO << __PRETTY_FUNCTION__ << " " << tagStruct.filePath << " is saved!";
    return crow::response { 200 };
}

std::expected<std::string, std::string> Wav::resolveTag(const std::string_view tag, const std::string_view formatType) {
    if (formatType != "id3v2" && formatType != "riff") {
        return std::unexpected("Unsupported WAV tag format");
    }

    return getTagMap()->resolve(std::string(tag), std::string(formatType));
}


