#include "id3v1.h"
#include <tpropertymap.h>

namespace clef::music::id3v1 {
    namespace {
        void setTagField(TagLib::ID3v1::Tag *tag, const std::string_view fieldType, const TagLib::String &value="") {
            if (fieldType == "TITLE") {
                tag->setTitle(value);
                return;
            }
            if (fieldType == "ARTIST") {
                tag->setArtist(value);
                return;
            }
            if (fieldType == "ALBUM") {
                tag->setAlbum(value);
                return;
            }
            if (fieldType == "YEAR") {
                tag->setYear(value == "" ? 0 : value.toInt());
                return;
            }
            if (fieldType == "COMMENT") {
                tag->setComment(value);
                return;
            }
            if (fieldType == "GENRE") {
                tag->setGenre(value);
                return;
            }
            if (fieldType == "TRACKNUMBER") {
                tag->setTrack(value == "" ? 0 : value.toInt());
            }
        }
    }

    json listMusicTags(const TagLib::ID3v1::Tag *tag) {
        auto map = tag->properties();
        json j = json::object();
        for (const auto &[key, value] : map) {
            j[key.toCString()] = value.toString().toCString();
        }

        return j;
    }

    void removeMusicTag(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct) {
        setTagField(tag, tagStruct.fieldType);
    }
    void addMusicTag(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct) {
        setTagField(tag, tagStruct.fieldType, tagStruct.value);
    }

    void editMusicTags(TagLib::ID3v1::Tag *tag, const TagChangeRequest &tagStruct) {
        setTagField(tag, tagStruct.fieldType, tagStruct.replaceWith);
    }
}