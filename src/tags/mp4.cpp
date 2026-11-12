#include "mp4.h"

#include <algorithm>
#include <charconv>
#include <crow/logging.h>

#include "../../include/tagConstants.h"
#include "../../include/tagMapping.h"

namespace clef::music::mp4 {
    using namespace clef::music::tag;

    namespace {
        TagLib::MP4::Item::Type defaultItemType(const std::string_view fieldType) {
            using Type = TagLib::MP4::Item::Type;
            if (fieldType == "trkn" || fieldType == "disk")
                return Type::IntPair;
            if (fieldType == "cpil" || fieldType == "pgap" || fieldType == "pcst" || fieldType == "shwm")
                return Type::Bool;
            if (fieldType == "tmpo" || fieldType == "©mvi" || fieldType == "©mvc" || fieldType == "hdvd")
                return Type::Int;
            if (fieldType == "tvsn" || fieldType == "tves" || fieldType == "sfID" || fieldType == "atID" || fieldType == "geID" ||
                fieldType == "cmID")
                return Type::UInt;
            if (fieldType == "cnID" || fieldType == "plID")
                return Type::LongLong;
            if (fieldType == "stik" || fieldType == "rtng" || fieldType == "akID")
                return Type::Byte;
            if (fieldType == "covr")
                return Type::CoverArtList;
            if (fieldType == "stem")
                return Type::Stem;
            if (fieldType == "gnre")
                return Type::Void;

            // Default for new text/freeform items; existing items may be binary.
            return Type::StringList;
        }
    }

    void ensureClefId(TagLib::MP4::Tag *tag, std::string *clefId) {
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

    json listMusicTags(const TagLib::MP4::Tag *tag) {
        using Type = TagLib::MP4::Item::Type;
        json base = json::object();

        const auto &map = tag->itemMap();

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
            default:
                CROW_LOG_WARNING << __PRETTY_FUNCTION__ << "not implemented";
                break;
            }
        }

        return base;
    }

    void removeMusicTag(TagLib::MP4::Tag *tag, const TagChangeRequest &tagStruct) {
        using namespace TagLib;
        using namespace TagLib::MP4;
        const String atomKey{ tagStruct.fieldType, String::UTF8 };
        const auto &map = tag->itemMap();
        if (const auto x = map.find(atomKey); x != map.end()) {
            switch (x->second.type()) {
            case Item::Type::Void:
            case Item::Type::Bool:
            case Item::Type::Byte:
            case Item::Type::Int:
            case Item::Type::IntPair:
            case Item::Type::UInt:
            case Item::Type::Stem:
            case Item::Type::LongLong:
                tag->removeItem(atomKey);
                break;
            case Item::Type::StringList: {
                StringList values{ x->second.toStringList() };
                if (const auto value = values.find(tagStruct.value); value != values.end())
                    values.erase(value);

                // If deleted value was the only one - delete the whole item
                // Otherwise overwrite item with edited values
                if (values.isEmpty())
                    tag->removeItem(atomKey);
                else
                    tag->setItem(atomKey, values);
                break;
            }
            default:
                CROW_LOG_ERROR << "Not implemented type";
                break;
            }
        }
    }

    std::expected<void, std::string> addMusicTag(TagLib::MP4::Tag *tag, const TagChangeRequest &tagStruct, const std::string &resolvedField) {
        using namespace TagLib;
        using namespace TagLib::MP4;

        const String atomKey{ resolvedField, String::UTF8 };
        const auto &map = tag->itemMap();
        Item item;
        switch (defaultItemType(resolvedField)) {
        case Item::Type::StringList: {
            StringList values;
            if (const auto value = map.find(atomKey); value != map.end())
                values = value->second.toStringList();
            values.append(tagStruct.value);
            item = Item{ values };
            break;
        }
        case Item::Type::Int: {
            item = Item{ std::stoi(tagStruct.value.toCString()) };
            break;
        }
        case Item::Type::IntPair: {
            const auto text = tagStruct.value.to8Bit(true);
            const auto separator = text.find('/');
            const int first = std::stoi(text.substr(0, separator));
            const int second = separator == std::string::npos ? first : std::stoi(text.substr(separator + 1));
            item = Item{ first, second };
            break;
        }
        case Item::Type::Bool: {
            item = Item{ tagStruct.value == "1" || tagStruct.value == "true" };
            break;
        }
        case Item::Type::UInt: {
            item = Item{ static_cast<unsigned int>(std::stoul(tagStruct.value.toCString())) };
            break;
        }
        case Item::Type::LongLong: {
            item = Item{ std::stoll(tagStruct.value.toCString()) };
            break;
        }
        case Item::Type::Byte: {
            item = Item{ static_cast<unsigned char>(std::stoi(tagStruct.value.toCString())) };
            break;
        }
        default:
            return std::unexpected("Not implemented method");
        }
        tag->setItem(atomKey, item);
        return {};
    }

    std::expected<void, std::string> editMusicTags(TagLib::MP4::Tag *tag, const TagChangeRequest &tagStruct, const std::string &resolvedField) {
        using namespace TagLib;
        using namespace TagLib::MP4;

        const String atomKey{ resolvedField, String::UTF8 };
        const auto &map = tag->itemMap();
        const auto existing = map.find(atomKey);
        if (existing == map.end())
            return std::unexpected("Field type does not exist");

        const auto replacement = tagStruct.replaceWith.to8Bit(true);
        Item item;
        switch (existing->second.type()) {
        case Item::Type::Bool:
            if (replacement != "0" && replacement != "1" && replacement != "false" && replacement != "true")
                return std::unexpected("Invalid boolean value (expected 0, 1, false or true)");
            item = Item{ replacement == "1" || replacement == "true" };
            break;
        case Item::Type::Byte: {
            item = Item{ static_cast<unsigned char>(std::stoi(replacement)) };
            break;
        }
        case Item::Type::Int: {
            item = Item{ stoi(replacement) };
            break;
        }
        case Item::Type::IntPair: {
            const auto separator = replacement.find('/');
            const int first = std::stoi(replacement.substr(0, separator));
            const int second = separator == std::string::npos ? first : std::stoi(replacement.substr(separator + 1));
            item = Item{ first, second };
            break;
        }
        case Item::Type::UInt: {
            item = Item{ static_cast<unsigned int>(std::stoul(replacement)) };
            break;
        }
        case Item::Type::LongLong: {
            item = Item{ std::stoll(replacement) };
            break;
        }
        case Item::Type::StringList: {
            StringList values{ existing->second.toStringList() };
            const auto value = values.find(tagStruct.replaceWhat);
            if (value == values.end())
                return std::unexpected("Specified value was not found in a file");
            *value = tagStruct.replaceWith;
            item = Item{ values };
            break;
        }
        default:
            return std::unexpected("Editing this MP4 item type is not supported");
        }

        tag->setItem(atomKey, item);
        return {};
    }

    std::expected<std::string, std::string> resolveTag(std::string_view tag) {
        return getTagMap()->resolve(std::string(tag), "mp4");
    }
}
