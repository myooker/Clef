#include "wma.h"
#include <algorithm>
#include <asffile.h>
#include <tstringlist.h>
#include <utility>

#include "../../include/tagConstants.h"
#include "../../include/tagMapping.h"

using namespace clef::music::handler;

void Wma::ensureClefId(TagLib::ASF::Tag *tag, std::string *clefId) {
    using namespace TagLib;
    const String key { tag::clefId.data() };
    if (auto values = tag->attribute(key); !values.isEmpty()) {
        *clefId = values[0].toString().to8Bit();
    }
    tag->setAttribute(key, ASF::Attribute{ *clefId });
}

std::expected<std::string, std::string> Wma::getArtist(std::string_view filepath) {
    TagLib::ASF::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->artist().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wma::getAlbum(std::string_view filepath) {
    TagLib::ASF::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->album().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wma::getTitle(std::string_view filepath) {
    TagLib::ASF::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->title().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<std::string, std::string> Wma::getGenre(std::string_view filepath) {
    TagLib::ASF::File file { filepath.data() };
    if (file.isValid())
        return file.tag()->genre().to8Bit(true);
    return std::unexpected(std::string(filepath) + "is not valid");
}

std::expected<json, std::string> Wma::listMusicTags(const std::string &filepath) {
    TagLib::ASF::File file { filepath.data() };

    if (file.isValid()) {
        json j = json::object();
        auto map = file.tag()->attributeListMap();
        json asf = json::object();
        for (const auto &[key, values] : map) {
            if (key == "WM/Picture") continue;
            for (const auto &value : values) {
                asf[key.to8Bit()] += value.toString().to8Bit(true);
            }
        }
        j["asf"] = asf;
        return j;
    }
    CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << filepath.data() << " is not valid";
    return std::unexpected(std::string(filepath) + " is not valid");
}

crow::response Wma::removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace TagLib;
    ASF::File file { tagStruct.filePath.data() };
    if (file.isValid()) {
        const String key { tagStruct.fieldType, String::UTF8 };
        const auto tag = file.tag();
        auto values = tag->attribute(key);
        const auto match = std::ranges::find_if(values, [&](const ASF::Attribute &attribute) {
           return attribute.toString() == tagStruct.value;
        });
        if (match == values.end()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": specified tag was not found in"
                                                << tagStruct.filePath;
            return { 404, "Specified tag was not found" };
        }
        values.erase(match);
        if (values.isEmpty())
            tag->removeItem(key);
        else
            tag->setAttribute(key, values);
        if (clefId) ensureClefId(tag, clefId);
        file.save();
        return crow::response { 200 };
    }
    CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
    return crow::response { 500, tagStruct.filePath + " is not valid" };
}

crow::response Wma::addMusicTag(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace TagLib;
    ASF::File file { tagStruct.filePath.data() };
    if (file.isValid()) {
        const String key { tagStruct.fieldType, String::UTF8 };
        const auto tag = file.tag();
        const ASF::Attribute value { tagStruct.value };
        tag->addAttribute(key, value);
        if (clefId) ensureClefId(tag, clefId);
        file.save();
        return crow::response { 200 };
    }
    CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
    return crow::response { 500, tagStruct.filePath + " is not valid" };
}

crow::response Wma::editMusicTags(TagChangeRequest &tagStruct, std::string *clefId) {
    using namespace TagLib;
    ASF::File file { tagStruct.filePath.data() };
    if (file.isValid()) {
        const String key { tagStruct.fieldType, String::UTF8 };
        const auto tag = file.tag();
        auto values = tag->attribute(key);
        const auto match = std::ranges::find_if(values, [&](const ASF::Attribute &attribute) {
           return attribute.toString() == tagStruct.replaceWhat;
        });
        if (match == values.end()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": specified tag was not found in"
                                                << tagStruct.filePath;
            return { 404, "Specified tag was not found" };
        }
        *match = tagStruct.replaceWith;
        tag->setAttribute(key, values);
        if (clefId) ensureClefId(tag, clefId);
        if (file.save())
            return crow::response { 200 };

        return crow::response { 500 };
    }
    CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " is not valid";
    return crow::response { 500, tagStruct.filePath + " is not valid" };
}

std::expected<std::string, std::string> Wma::resolveTag(std::string_view tag, std::string_view) {
    return tag::getTagMap()->resolve(tag.data(), "asf");
}
