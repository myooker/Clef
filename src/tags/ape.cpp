#include "ape.h"

#include "../../include/tagConstants.h"
#include "../../include/tagMapping.h"

namespace clef::music::ape {
    json listMusicTags(const APE::Tag *tag) {
        const auto &map = tag->itemListMap();
        json j = json::object();
        for (const auto &[key, item] : map) {
            if (item.type() != APE::Item::Text)
                continue;
            for (const auto &value : item.values()) {
                j[key.to8Bit(true)] += value.to8Bit(true);
            }
        }
        return j;
    }

    void ensureClefId(APE::Tag *tag, std::string *clefid) {
        auto &map = tag->itemListMap();
        auto clefIdIt = map.find(String(tag::clefId.data()).upper());
        if (clefIdIt != map.end()) {
            *clefid = clefIdIt->second.toString().toCString(false);
        }
        tag->addValue(String(tag::clefId.data()).upper(), *clefid, true);
    }

    void removeMusicTag(APE::Tag *tag, const TagChangeRequest &tagStruct) {
        auto &map = tag->itemListMap();

        // Find and remove occurrence of tagStruct.fieldType
        auto it = map.find(tagStruct.fieldType);
        if (it != map.end() && it->second.type() == APE::Item::Text) {
            const String key { it->first };
            StringList values { it->second.values() };

            if (const auto rValue = values.find(tagStruct.value); rValue != values.end())
                values.erase(rValue);

            const APE::Item newFieldType { key, values };
            if (values.isEmpty())
                tag->removeItem(key);
            else
                tag->setItem(key, newFieldType);
        }
    }

    void addMusicTag(APE::Tag *tag, const TagChangeRequest &tagStruct) {
        tag->addValue(tagStruct.fieldType, tagStruct.value, false);
    }

    void editMusicTags(APE::Tag *tag, const TagChangeRequest &tagStruct) {
        auto &map = tag->itemListMap();

        // Find and edit occurrence of tagStruct.replaceWhat in tagStruct.fieldType
        auto it = map.find(tagStruct.fieldType);
        if (it != map.end() && it->second.type() == APE::Item::Text) {
            const String key { it->first };
            StringList values { it->second.values() };

            if (const auto rValue = values.find(tagStruct.replaceWhat); rValue != values.end()) {
                *rValue = tagStruct.replaceWith;
            }

            const APE::Item newFieldType { key, values };
            if (values.isEmpty())
                tag->removeItem(key);
            else
                tag->setItem(key, newFieldType);
        }
    }

    std::expected<std::string, std::string> resolveTag(const std::string_view tag) {
        return tag::getTagMap()->resolve(std::string(tag), "apev2");
    }
}