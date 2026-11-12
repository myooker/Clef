#ifndef CLEF_INTERFACE_H
#define CLEF_INTERFACE_H

#include <expected>
#include <string>
#include <string_view>

#include <crow/http_response.h>
#include <nlohmann/json.hpp>

#include "../../include/picture.h"
#include "../../include/tagChangeRequest.h"

using json = nlohmann::json;

namespace clef::music::handler {
    class Interface {
    public:
        virtual ~Interface() = default;

        virtual std::expected<std::string, std::string> getArtist(std::string_view filepath) = 0;
        virtual std::expected<std::string, std::string> getAlbum(std::string_view filepath) = 0;
        virtual std::expected<std::string, std::string> getTitle(std::string_view filepath) = 0;
        virtual std::expected<std::string, std::string> getGenre(std::string_view filepath) = 0;
        virtual std::expected<json, std::string> listMusicTags(const std::string &filePath) = 0;
        virtual crow::response removeMusicTag(TagChangeRequest &tagStruct, std::string *clefId = nullptr) = 0;
        virtual crow::response addMusicTag(TagChangeRequest &tagStruct, std::string *clefId = nullptr) = 0;
        virtual crow::response editMusicTags(TagChangeRequest &tagStruct, std::string *clefId = nullptr) = 0;
        virtual tag::Picture getAlbumCover(const std::string &filePath) = 0;
        virtual void removeAlbumCover(const std::string &filePath) = 0;
        virtual void addAlbumCover(const std::string &filePath) = 0;
        virtual std::expected<std::string, std::string> resolveTag(std::string_view tag, std::string_view formatType) = 0;
    };
}


#endif // CLEF_INTERFACE_H
