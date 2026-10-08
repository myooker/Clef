#ifndef CLEF_MPEG4_H
#define CLEF_MPEG4_H

#include <expected>
#include <string>
#include <string_view>

#include <mp4tag.h>

#include "../../include/interface.h"

namespace clef::music::handler {
    class Mpeg4 : public Interface {
    private:
        constexpr static std::string_view m_type { "mp4" };
        static void ensureClefId(std::string *clefId, TagLib::MP4::Tag *tag);
    public:
        std::expected<json, std::string> listMusicTags(const std::string &filePath) override;
        static void addUserDefinedAtom(const TagChangeRequest &tagStruct);
        crow::response removeMusicTag(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response addMusicTag(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response editMusicTags(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        tag::Picture getAlbumCover(const std::string& filePath) override { return tag::Picture{}; }
        void removeAlbumCover(const std::string& filePath) override {}
        void addAlbumCover(const std::string& filePath) override {}
        std::expected<std::string, std::string> resolveTag(std::string_view tag) override;
    };
} // audioFormat

#endif // CLEF_MPEG4_H
