#ifndef CLEF_INTERFACE_H
#define CLEF_INTERFACE_H

#include <expected>
#include <string>
#include <string_view>

#include <crow/http_response.h>
#include <nlohmann/json.hpp>

#include "picture.h"
#include "tagChangeRequest.h"

using json = nlohmann::json;

namespace clef::music::handler {
    class Interface {
    public:
        virtual ~Interface() = default;

        virtual std::expected<json, std::string> listMusicTags(const std::string& filePath) = 0;
        virtual crow::response removeMusicTag(const TagChangeRequest& tagStruct, std::string* clefId = nullptr) = 0;
        virtual crow::response addMusicTag(const TagChangeRequest& tagStruct, std::string* clefId = nullptr) = 0;
        virtual crow::response editMusicTags(const TagChangeRequest& tagStruct, std::string* clefId = nullptr) = 0;
        virtual tag::Picture getAlbumCover(const std::string& filePath) = 0;
        virtual void removeAlbumCover(const std::string& filePath) = 0;
        virtual void addAlbumCover(const std::string& filePath) = 0;
        virtual std::expected<std::string, std::string> resolveTag(std::string_view tag) = 0;
    };
}


#endif // CLEF_INTERFACE_H
