#ifndef CLEF_OGGOPUS_H
#define CLEF_OGGOPUS_H

#include <expected>
#include <string>
#include <string_view>

#include "interface.h"

namespace clef::music::handler {
    class OggOpus : public Interface {
    public:
        std::expected<std::string, std::string> getArtist(std::string_view filepath) override;
        std::expected<std::string, std::string> getAlbum(std::string_view filepath) override;
        std::expected<std::string, std::string> getTitle(std::string_view filepath) override;
        std::expected<std::string, std::string> getGenre(std::string_view filepath) override;
        std::expected<json, std::string> listMusicTags(const std::string &filePath) override;
        crow::response removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response addMusicTag(TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        crow::response editMusicTags(TagChangeRequest &tagStruct, std::string *clefId = nullptr) override;
        tag::Picture getAlbumCover(const std::string &filePath) override { return tag::Picture{}; }
        void removeAlbumCover(const std::string &filePath) override {}
        void addAlbumCover(const std::string &filePath) override {}
        std::expected<std::string, std::string> resolveTag(std::string_view tag, std::string_view formatType) override;
    };
} // audioFormat

#endif // CLEF_OGGOPUS_H
