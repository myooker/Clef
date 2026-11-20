#ifndef CLEF_WMA_H
#define CLEF_WMA_H

#include <asftag.h>
#include "interface.h"

namespace clef::music::handler {
    class Wma : public Interface {
    private:
        static void ensureClefId(TagLib::ASF::Tag *tag, std::string *clefid);
    public:
        std::expected<std::string, std::string> getArtist(std::string_view filepath) override;
        std::expected<std::string, std::string> getAlbum(std::string_view filepath) override;
        std::expected<std::string, std::string> getTitle(std::string_view filepath) override;
        std::expected<std::string, std::string> getGenre(std::string_view filepath) override;
        std::expected<json, std::string> listMusicTags(const std::string &filepath) override;
        crow::response removeMusicTag(TagChangeRequest& tagStruct, std::string* clefId) override;
        crow::response addMusicTag(TagChangeRequest& tagStruct, std::string* clefId) override;
        crow::response editMusicTags(TagChangeRequest& tagStruct, std::string* clefId) override;
        tag::Picture getAlbumCover(const std::string& filePath) override { return {}; }
        void removeAlbumCover(const std::string& filePath) override {}
        void addAlbumCover(const std::string& filePath) override {}
        std::expected<std::string, std::string> resolveTag(std::string_view tag, std::string_view) override;
    };
}

#endif //CLEF_WMA_H
