#ifndef CLEF_OGGSPEEX_H
#define CLEF_OGGSPEEX_H

#include <expected>
#include <string>
#include <string_view>

#include <xiphcomment.h>

#include "../../include/interface.h"

namespace clef::music::handler {
    class OggSpeex : public Interface {
    private:
        constexpr static std::string_view m_type { "vorbis" };
        static void ensureClefId(std::string *clefId, TagLib::Ogg::XiphComment *tag);
    public:
        std::expected<json, std::string> listMusicTags(const std::string &filePath) override;
        crow::response removeMusicTag(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response addMusicTag(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response editMusicTags(const TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        tag::Picture getAlbumCover(const std::string& filePath) override { return tag::Picture{}; }
        void removeAlbumCover(const std::string& filePath) override {}
        void addAlbumCover(const std::string& filePath) override {}
        std::expected<std::string, std::string> resolveTag(std::string_view tag) override;
    };
} // audioFormat

#endif // CLEF_OGGSPEEX_H
