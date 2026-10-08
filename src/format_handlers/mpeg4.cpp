#include "mpeg4.h"

#include <algorithm>
#include <utility>

#include <crow/logging.h>
#include <mp4file.h>

#include "../../include/tagConstants.h"
#include "../../include/tagMapping.h"

using namespace clef::music::handler;
using namespace clef::music::tag;

void Mpeg4::ensureClefId(std::string* clefId, TagLib::MP4::Tag* tag) {
    using namespace clef::music;
    using namespace TagLib;

    const String clefIdAtom { std::string(prefix::m4a) + tag::clefId.data() };
    const auto iMap = tag->itemMap();
    const auto findAtom = [&] (const String &name) {
        const auto normalized = name.upper();
        return std::ranges::find_if(iMap, [&](const auto &entry) {
            return entry.first.upper() == normalized;
        });
    };
    if (const auto clefIdIt = findAtom(clefIdAtom); clefIdIt != iMap.end()) {
        *clefId = clefIdIt->second.toStringList()[0].toCString(false);
    } else {
        const String rteIdAtom { std::string(prefix::m4a) + rteId.data() };
        if (const auto rteIdIt = findAtom(rteIdAtom); rteIdIt != iMap.end()) {
            *clefId = rteIdIt->second.toStringList()[0].toCString(false);
            tag->removeItem(rteIdIt->first);
        }
        tag->setItem(clefIdAtom, MP4::Item{StringList{String{*clefId, String::UTF8}}});
    }
}

std::expected<json, std::string> Mpeg4::listMusicTags(const std::string &filePath) {
    using namespace clef::music;
    const TagLib::MP4::File file { filePath.c_str() };
    using Type = TagLib::MP4::Item::Type;

    if (!file.isValid()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is not valid";
        return std::unexpected(filePath + " is not valid");
    }

    if (!file.hasMP4Tag()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " does not have mp4 tags";
        return std::unexpected(filePath + " does not have mp4 tags");
    }

    json base = json::object();

    const auto mp4tag = file.tag();
    const auto &map = mp4tag->itemMap();

    for (const auto &[key, value] : map) {
        switch (value.type()) {
            case Type::StringList: {
                    for (const auto &x : value.toStringList()) {
                        base[key.to8Bit(true)] += x.toCString(true);
                    }
                    break;
                }
            case Type::Int:
                base[key.to8Bit(true)] = value.toInt();
                break;
            case Type::IntPair: {
                const auto [first, second] = value.toIntPair();
                base[key.to8Bit(true)] = std::to_string(first) + "/" + std::to_string(second);
                break;
            }
            case Type::Bool:
                base[key.to8Bit(true)] = value.toBool();
                break;
            case Type::UInt:
                base[key.to8Bit(true)] = value.toUInt();
                break;
            case Type::LongLong:
                base[key.to8Bit(true)] = value.toLongLong();
                break;
            case Type::Byte:
                base[key.to8Bit(true)] = value.toByte();
                break;
            case Type::ByteVectorList:
            case Type::CoverArtList:
            case Type::Stem:
            case Type::Void:
                CROW_LOG_WARNING << __PRETTY_FUNCTION__ << " atom type is not implemented";
                break;
        }
    }

    return base;
}

crow::response Mpeg4::removeMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
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

    tag->removeItem(TagLib::String{tagStruct.fieldType, TagLib::String::UTF8});

    if (clefId) ensureClefId(clefId, tag);
    file.save();

    return {200, "OK"};
}

crow::response Mpeg4::addMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
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

    auto resolve = getTagMap()->resolve(tagStruct.fieldType, m_type.data());
    if (!resolve.has_value()) {
        CROW_LOG_ERROR << resolve.error();
        return crow::response { 400, resolve.error() };
    }
    const std::string &raw = resolve.value();
    auto *tag = file.tag();
    const TagLib::String atomKey { raw, TagLib::String::UTF8 };

    const auto &itemMap = tag->itemMap();
    const auto existingIt = itemMap.find(atomKey);
    const auto targetType = existingIt != itemMap.end()
        ? existingIt->second.type()
        : TagLib::MP4::Item::Type::StringList;

    switch (targetType) {
        using namespace TagLib::MP4;
        case Item::Type::StringList: {
            const Item item { TagLib::StringList{ tagStruct.value } };
            tag->setItem(atomKey, item);
            break;
        }
        case Item::Type::Int: {
            const Item item { std::stoi(tagStruct.value.toCString())};
            tag->setItem(atomKey, item);
            break;
        }
        case Item::Type::IntPair: {
            const auto sep = tagStruct.value.find('/');
            if (sep != std::string::npos) {
                const int first = std::stoi(tagStruct.value.to8Bit().substr(0, sep));
                const int second = std::stoi(tagStruct.value.to8Bit().substr(sep + 1));
                tag->setItem(atomKey, Item(first, second));
            } else {
                tag->setItem(atomKey, Item(std::stoi(tagStruct.value.toCString())));
            }
            break;
        }
        case Item::Type::Bool: {
            tag->setItem(atomKey, Item(tagStruct.value == "1" || tagStruct.value == "true"));
            break;
        }
        case Item::Type::UInt: {
            tag->setItem(atomKey, Item(static_cast<unsigned int>(std::stoul(tagStruct.value.toCString()))));
            break;
        }
        case Item::Type::LongLong: {
            tag->setItem(atomKey, Item(std::stoll(tagStruct.value.toCString())));
            break;
        }
        case Item::Type::Byte: {
            tag->setItem(atomKey, Item(static_cast<unsigned char>(std::stoi(tagStruct.value.toCString()))));
            break;
        }
        default:
            return { 400, "Not implemented method" };
    }

    if (clefId) ensureClefId(clefId, tag);
    if (file.save()) {
        CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has been saved!";
        return { 200, "OK" };
    } else {
        CROW_LOG_INFO << __PRETTY_FUNCTION__ << ": " << tagStruct.filePath << " has not been saved :(";
        return { 500, "Has not been saved" };
    }
}

crow::response Mpeg4::editMusicTags(const TagChangeRequest &tagStruct, std::string *clefId) {
    auto modified = tagStruct;
    modified.value = tagStruct.replaceWith;
    return addMusicTag(modified, clefId);
}

std::expected<std::string, std::string> Mpeg4::resolveTag(const std::string_view tag) {
    auto resolve = getTagMap()->resolve(tag.data(), m_type.data());
    if (resolve.has_value())
        return resolve.value();
    return std::unexpected(std::move(resolve).error());
}
