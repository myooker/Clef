#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

#include <crow.h>
#include <CLI/CLI.hpp>
#include <crow/compression.h>
#include <crow/middlewares/cors.h>
#include <crow/middlewares/cookie_parser.h>
#include <nlohmann/json.hpp>
#include <SQLiteCpp/SQLiteCpp.h>

//#include "../include/authentication/authMiddleware.h"
#include "../include/directoryListing.h"
#include "../include/clef.h"
#include "../include/clefConstants.h"
#include "../include/tagChangeRequest.h"
#include "../include/tagConstants.h"
#include "../include/database/library.h"
#include "../include/database/tagHistory.h"
//#include "../include/database/users.h"
#include "../include/tagMapping.h"
#include "../include/utils.h"
#include "format_handlers/factory.h"

using json = nlohmann::json;
using ordered_json = nlohmann::ordered_json;
namespace fs = std::filesystem;

static clef::EntityType fileExtensionToEntityType(const std::string_view ext) {
    using namespace clef;
    std::string a { ext };
    std::ranges::transform(a, a.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    const static std::unordered_map<std::string, EntityType> s_extensionMap {
        {".mp3",    EntityType::music},
        {".flac",   EntityType::music},
        {".m4a",    EntityType::music},
        {".ogg",    EntityType::music},
        {".opus",   EntityType::music},
        {".aac",    EntityType::music},
        {".wma",    EntityType::music},
        {".wav",    EntityType::music},
        {".aif",    EntityType::music},
        {".aiff",   EntityType::music},
        {".alac",   EntityType::music},
        {".jpg",    EntityType::picture},
        {".jpeg",   EntityType::picture},
        {".png",    EntityType::picture}
    };

    if (const auto it = s_extensionMap.find(a); it != s_extensionMap.end())
        return it->second;

    return EntityType::file;
}

static ordered_json buildDirectoryTree(const std::string &basePath, const clef::DirectoryListOptions &query) {
    using namespace clef;

    std::vector<FileEntity> entities;
    const fs::path path { basePath };

    // Get size (number of entities) of inside a path and reserve it for vector
    {
        std::size_t count {};
        for (fs::directory_iterator it(path); it != fs::directory_iterator(); ++it)
            count++;
        entities.reserve(count);
        CROW_LOG_DEBUG << __PRETTY_FUNCTION__ << "Reserved: " << count;
    }

    for (const auto &e : fs::directory_iterator(path)) {
        if (e.is_symlink()) continue;
        bool isDirectory { e.is_directory() };
        std::string filename { e.path().filename() };
        std::string ext { e.path().extension() };
        EntityType type { isDirectory ? EntityType::directory : fileExtensionToEntityType(ext) };
        if (isDirectory) {
            entities.push_back({
                .name = std::move(filename), .type = EntityType::directory
            });
            continue;
        }
        const uintmax_t size = e.file_size();
        entities.push_back({
            .name = std::move(filename), .ext = std::move(ext), .size = size, .type = type
        });
    }

    std::ranges::sort(entities, [&query](const auto &a, const auto &b) {
        return utils::entityLess(a, b, query);
    });

    ordered_json j = json::object();

    const std::size_t esize = entities.size();
    const std::size_t begin = std::min(query.offset, esize);
    const std::size_t end = begin + std::min(
        query.limit == 0 ? esize - begin : query.limit, esize - begin);

    j["path"] = basePath;
    j["total"] = esize;
    j["offset"] = begin;
    j["limit"] = query.limit;
    j["entities"] = json::array();

    for (std::size_t o = begin; o < end; o++) {
        ordered_json t = json::object();
        t["name"] = std::move(entities[o].name);
        t["type"] = entities[o].typeString();
        t["extension"] = std::move(entities[o].ext);
        t["size"] = entities[o].size;
        j["entities"].push_back(std::move(t));
    }

    return j;
}

int main (int argc, char **argv) {
    using namespace clef::music;
    using namespace clef::music::handler;

    std::unique_ptr<clef::Application> application;
    int debugLevel {};
    auto logLevel { crow::LogLevel::Info };

    {
        CLI::App cli {
            "Backend API that edits music file tags (ID3/Vorbis) on request from a web‑based editor.",
            clef::name.data()
        };

        std::string databasePath { clef::defaultDatabasePath };
        std::string mappingPath { clef::defaultMappingPath };
        std::string mountPoint { clef::defaultMountPoint };
        bool useClefId { clef::defaultClefIdStatus };
        int port { clef::defaultPort };

        cli.add_option("-m,--mount-point", mountPoint,
                "The directory of your music library")->required();
        cli.add_option("-p,--port", port,
            "The application's port to bind in. Default is 18080.")->default_val(18080);
        cli.add_option("-l,--log-level", debugLevel,
                "temp")->default_val(crow::LogLevel::WARNING);
        cli.add_option("--database-path", databasePath,
            "Database path location. Default is /");
        cli.add_flag("--use-clefid", useClefId, "");
        CLI11_PARSE(cli, argc, argv);

        const char *clefId = std::getenv(clef::environments::useClefId.data());
        if (clefId) {
            useClefId = clef::utils::parseBool(clefId).value_or(false);
        }

        try {
            application = std::make_unique<clef::Application>(
                databasePath,
                mappingPath,
                mountPoint,
                useClefId,
                port
            );
            tag::getTagMap(); // just creates a static tagMap
        } catch (std::exception &e) {
            CROW_LOG_CRITICAL << e.what();
            return EXIT_FAILURE;
        }
    }

    switch (debugLevel) {
        case 0: logLevel = crow::LogLevel::DEBUG; break;
        case 1: logLevel = crow::LogLevel::INFO; break;
        case 2: logLevel = crow::LogLevel::WARNING; break;
        case 3: logLevel = crow::LogLevel::ERROR; break;
        case 4: logLevel = crow::LogLevel::CRITICAL; break;
        default: logLevel = crow::LogLevel::INFO; break;
    }

    //crow::App<crow::CORSHandler, crow::CookieParser, clef::AuthMiddleware> app;
    //app.get_middleware<clef::AuthMiddleware>().setSessionStore(application->getSessionStore());
    crow::App<crow::CORSHandler> app;
    CROW_LOG_INFO << clef::name << " v" << clef::version << " is running now";
    CROW_LOG_INFO << "Mountpoint: " << application->getMountPoint();
    CROW_LOG_INFO << "Listenting port: " << application->getPort();

    // CROW_ROUTE(app, "/api/auth/login").methods("POST"_method)
    // ([&](const crow::request &req) {
    //     constexpr std::string_view logPrefix { "(api/auth/login): " };
    //     crow::response res;
    //     const json reqbody = json::parse(req.body);
    //     std::string username { reqbody.value("username", clef::jsonMissingValue.data()) };
    //     std::string password { reqbody.value("password", clef::jsonMissingValue.data()) };
    //
    //     auto sessionToken = application->userLogin(username, password);
    //     CROW_LOG_WARNING << logPrefix << "sessionToken: " << sessionToken;
    //     res.set_header(
    //       "Set-Cookie",
    //       "clef_session="+ sessionToken +
    //       "; HttpOnly; SameSite=Lax; Path=/"
    //     );
    //     res.set_header("Cache-Control", "no-cache");
    //
    //     return res;
    // });

    // CROW_ROUTE(app, "/api/auth/signup").methods("POST"_method)
    // ([&](const crow::request &req) {
    //     constexpr std::string_view logPrefix { "(api/auth/signup): " };
    //     const json reqbody = json::parse(req.body);
    //     std::string username { reqbody.value("username", clef::jsonMissingValue.data()) };
    //     std::string password { reqbody.value("password", clef::jsonMissingValue.data()) };
    //
    //     CROW_LOG_WARNING << logPrefix << username;
    //     CROW_LOG_WARNING << logPrefix << password;
    //
    //     if (application->getUsersDB().createUser(username, password)) {
    //         CROW_LOG_WARNING << logPrefix << username << " is created successfully";
    //         return crow::response { 200 };
    //     }
    //
    //     CROW_LOG_WARNING << logPrefix << username << " is taken. Please choose something else!";
    //     return crow::response { 500, "This username is already taken." };
    // });

    CROW_ROUTE(app, "/api/settings").methods("GET"_method)
    ([&]() {
        json j = {
            {"clef_id", application->getClefIdStatus()},
            {"mountpoint", application->getMountPoint()},
            {"version", clef::version },
        };
        crow::response response { j.dump() };
        response.set_header("Content-Type", "application/json");
        return response;
    });

    CROW_ROUTE(app, "/api/undo").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        using namespace TagLib;

        constexpr std::string_view logPrefix { "(api/undo): " };
        json j = json::parse(req.body);

        crow::response response { 500, http::messages::internalError.data() };

        // 1 - Parse information from request to query database
        // All we need to have is the following variables:
        const int id               { j.value("id", -1) }; // add enum NOT_FOUND instead of -1
        const std::string clefId    { j.value("clef_id", clef::jsonMissingValue) };
        const std::string path      { j.value("path", clef::jsonMissingValue) };
        std::string tag             { j.value("tag", clef::jsonMissingValue) };

        CROW_LOG_WARNING << logPrefix << "id: " << id;
        CROW_LOG_WARNING << logPrefix << "clefId: " << clefId;
        CROW_LOG_WARNING << logPrefix << "path: " << path;
        CROW_LOG_WARNING << logPrefix << "tag: " << tag;

        auto handler = Factory::create(clef::utils::getExtension(path));
        if (!handler) {
            return crow::response { 400, http::messages::unsupportedFileFormat.data() };
        }

        std::string format;
        {
            SQLite::Statement selected { application->getDatabase(),
                "SELECT tag, format FROM tag_history "
                "WHERE id = ? AND (clefId = ? OR path = ?);"
            };
            selected.bind(1, id);
            selected.bind(2, clefId);
            selected.bind(3, path);
            if (!selected.executeStep()) {
                return crow::response { 404, "History entry was not found" };
            }
            tag = selected.getColumn("tag").getString();
            format = selected.getColumn("format").getString();
        }

        // 2 - Get information from query
        SQLite::Statement q { application->getDatabase(),
            "SELECT action, old_value, new_value FROM tag_history "
            "WHERE id >= ? AND (clefId = ? OR path = ?) AND tag = ? AND format IS ? "
            "ORDER BY id DESC;"
        };
        q.bind(1, id);
        q.bind(2, clefId);
        q.bind(3, path);
        q.bind(4, tag);
        q.bind(5, format);

        bool isGood { true };
        while (isGood && q.executeStep()) {
            String action     { q.getColumn(0).getString(), String::UTF8 };
            String oldValue   { q.getColumn(1).getString(), String::UTF8 };
            String newValue   { q.getColumn(2).getString(), String::UTF8 };

            clef::TagChangeRequest tagStruct {
                .filePath = path,
                .fieldType = tag,
                .formatType = format
            };
            if (action == "add") {
                tagStruct.value = newValue;
                isGood = handler->removeMusicTag(tagStruct).code == 200;
            } else if (action == "change") {
                tagStruct.replaceWhat = newValue;
                tagStruct.replaceWith = oldValue;
                isGood = handler->editMusicTags(tagStruct).code == 200;
            } else if (action == "remove") {
                tagStruct.value = oldValue;
                isGood = handler->addMusicTag(tagStruct).code == 200;
            } else {
                isGood = false;
            }
        }

        if (isGood) {
            SQLite::Statement d { application->getDatabase(),
                "DELETE FROM tag_history "
                "WHERE id >= ? AND (clefId = ? OR path = ?) AND tag = ? AND format IS ?;"
            };
            d.bind(1, id);
            d.bind(2, clefId);
            d.bind(3, path);
            d.bind(4, tag);
            d.bind(5, format);
            d.exec();

            response.code = 200;
            response.body = "Changes undone.";
        }

        return response;
    });

    CROW_ROUTE(app, "/api/getalbumcover").methods("GET"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        constexpr std::string_view logPrefix { "(api/getalbumcover): " };

        crow::response response{501, "Album artwork extraction is not implemented for this file format." };
        const char *path = req.url_params.get("path");
        if (path) {
            if (!application->isMountPoint(path)) {
                CROW_LOG_ERROR << logPrefix << http::messages::pathForbidden.data();
                return crow::response { 403, http::messages::pathForbidden.data() };
            }
            auto handler = Factory::create(clef::utils::getExtension(path));
            if (!handler) {
                CROW_LOG_ERROR << logPrefix;
                return crow::response { 422, http::messages::unsupportedFileFormat.data() };
            }
            auto picture = handler->getAlbumCover(path);

            response.body.assign(picture.data.data(), picture.data.size());
            response.set_header("Content-Type", picture.mimeType);
            response.code = 200;

            return response;
        }
        CROW_LOG_ERROR << logPrefix;
        return crow::response { 400, http::messages::pathRequired.data() };
    });

    CROW_ROUTE(app, "/api/gethistory").methods("GET"_method)
    ([&](const crow::request &req) {
        std::string fileIdentifier = req.url_params.get("identifier");
        std::string clause { "path = ?" }; // By default, it searches by path

        if (application->getClefIdStatus()) // Match history by Clef_ID when enabled
            clause = "clefId = ?";

        SQLite::Statement query(application->getDatabase(), "SELECT id, path, clefId, action, tag, old_value, new_value, changed_at FROM tag_history WHERE "
            +clause +" ORDER BY changed_at DESC");
        query.bind(1, fileIdentifier.c_str());
        json result = json::array();

        while (query.executeStep()) {
            int i { -1 };
            result.push_back({
                {"id",              query.getColumn(++i).getInt()},
                {"path",            query.getColumn(++i).getString()},
                {"clef_id",         query.getColumn(++i).getString()},
                {"action",          query.getColumn(++i).getString()},
                {"tag",             query.getColumn(++i).getString()},
                {"old_value",       query.getColumn(++i).getString()},
                {"new_value",       query.getColumn(++i).getString()},
                {"changed_at",      query.getColumn(++i).getString()},
            });
        }

        crow::response response { result.dump() };
        response.set_header("Content-Type", "application/json");

        return response;
    });

    CROW_ROUTE(app, "/api/getmntpoint").methods("GET"_method)
    ([&]() {
        json mountpoint;
        mountpoint["path"] = application->getMountPoint();
        crow::response response{ 200, mountpoint.dump() };
        response.set_header("Content-Type", "application/json");

        return response;
    });

    CROW_ROUTE(app, "/api/edittag").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        using namespace TagLib;

        constexpr std::string_view logPrefix { "(api/edittag): " };
        if (!req.body.empty()) {
            const ordered_json body = json::parse(req.body);

            clef::TagChangeRequest tagStruct {
                .filePath = body.value("path", clef::jsonMissingValue.data()),
                .fieldType = body.value("tagType", clef::jsonMissingValue.data()),
                .formatType = body.value("format", clef::jsonMissingValue.data()),
                .replaceWhat = { body.value("replaceWhat", clef::jsonMissingValue.data()), String::UTF8 },
                .replaceWith = { body.value("replaceWith", clef::jsonMissingValue.data()), String::UTF8 },
            };
            if (!tagStruct.isValid()) {
                CROW_LOG_ERROR << logPrefix << "tagStruct is invalid. Please check sending requests.";
                return crow::response { 400, http::messages::invalidRequestFields.data() };
            }
            if (!application->isMountPoint(tagStruct.filePath)) {
                CROW_LOG_ERROR << logPrefix << "requested filepath is not a mount-point";
                return crow::response { 403, http::messages::pathForbidden.data() };
            }
            clef::storage::id id {};
            const std::string fileExtension { clef::utils::getExtension(tagStruct.filePath) };

            CROW_LOG_WARNING << "(api/edittag) requested path: " << tagStruct.filePath;

            const auto handler = Factory::create(fileExtension);
            if (!handler) {
                CROW_LOG_CRITICAL << logPrefix << "Cannot create a handler for " << tagStruct.filePath;
                return crow::response { 400, http::messages::unsupportedFileFormat.data() };
            }
            const auto rtag = handler->resolveTag(tagStruct.fieldType, tagStruct.formatType);
            if (!rtag.has_value()) {
                CROW_LOG_ERROR << rtag.error();
                return crow::response { 500, http::messages::unsupportedTag.data() };
            }
            tagStruct.fieldType = rtag.value();
            CROW_LOG_WARNING << logPrefix << "resolved tag: " << tagStruct.fieldType;

            if (application->getClefIdStatus()) id.clefId = clef::utils::generateId();
            crow::response response(handler->editMusicTags(tagStruct, application->getClefIdStatus() ? &id.clefId : nullptr));

            if (response.code == 200) {
                return application->getTagHistoryDB().insertEdit(tagStruct, id);
            }
            return response;
        }
        CROW_LOG_ERROR << logPrefix << "request body is empty!";
        return crow::response { 400, http::messages::bodyRequired.data() };
    });

    CROW_ROUTE(app, "/api/addfieldtag").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        using namespace TagLib;

        constexpr std::string_view logPrefix { "(api/addfieldtag): " };

        if (!req.body.empty()) {
            const ordered_json body = json::parse(req.body);

            clef::TagChangeRequest tagStruct {
                .filePath = body.value("path", clef::jsonMissingValue.data()),
                .fieldType = body.value("fieldType", clef::jsonMissingValue.data()),
                .formatType = body.value("format", clef::jsonMissingValue.data()),
                .value = { body.value("value", clef::jsonMissingValue.data()), String::UTF8 }
            };
            if (!tagStruct.isValid()) {
                CROW_LOG_ERROR << logPrefix << "tagStruct is invalid. Please check sending requests.";
                return crow::response { 400, http::messages::invalidRequestFields.data() };
            }
            if (!application->isMountPoint(tagStruct.filePath)) {
                CROW_LOG_ERROR << logPrefix << "requested filepath is not a mount-point";
				return crow::response { 403, http::messages::pathForbidden.data() };
            }
            clef::storage::id id {};
            const std::string fileExtension { clef::utils::getExtension(tagStruct.filePath) };

            CROW_LOG_WARNING << logPrefix << "requested path: " << tagStruct.filePath;

            const auto handler = Factory::create(fileExtension);
            if (!handler) {
                CROW_LOG_CRITICAL << logPrefix << "Cannot create a handler for " << tagStruct.filePath;
                return crow::response { 400, http::messages::unsupportedFileFormat.data() };
            }
            const auto rtag = handler->resolveTag(tagStruct.fieldType, tagStruct.formatType);
            if (!rtag.has_value()) {
                CROW_LOG_ERROR << rtag.error();
                return crow::response { 500, http::messages::unsupportedTag.data() };
            }
            tagStruct.fieldType = rtag.value();
            CROW_LOG_WARNING << logPrefix << "resolved tag: " << tagStruct.fieldType;

            if (application->getClefIdStatus()) id.clefId = clef::utils::generateId();
            crow::response response(handler->addMusicTag(tagStruct, application->getClefIdStatus() ? &id.clefId : nullptr));

            if (response.code == 200) {
                return application->getTagHistoryDB().insertAdd(tagStruct, id);
            }
            return response;
        }
        CROW_LOG_ERROR << logPrefix << "request body is empty!";
        return crow::response { 400, http::messages::bodyRequired.data() };
    });

    CROW_ROUTE(app, "/api/removefieldtag").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        using namespace TagLib;
        using namespace clef::music::tag;

        constexpr std::string_view logPrefix { "(api/removefieldtag): " };

        if (!req.body.empty()) {
            const ordered_json body = json::parse(req.body);

            clef::TagChangeRequest tagStruct {
                .filePath = body.value("path", "none"),
                .fieldType = body.value("fieldType", "none"),
                .formatType = body.value("format", clef::jsonMissingValue.data()),
                .value = { body.value("value", "none"), String::UTF8 }
            };
            if (!tagStruct.isValid()) {
                CROW_LOG_ERROR << logPrefix << "tagStruct is invalid. Please check sending requests.";
                return crow::response { 400, http::messages::invalidRequestFields.data() };
            }
            if (!application->isMountPoint(tagStruct.filePath)) {
                CROW_LOG_ERROR << logPrefix << "requested filepath is not a mount-point";
				return crow::response { 403, http::messages::pathForbidden.data() };
            }
            clef::storage::id id {};
            const std::string fileExtension { clef::utils::getExtension(tagStruct.filePath) };

            CROW_LOG_WARNING << "(api/removefieldtag) requested path: " << tagStruct.filePath;

            if (application->getClefIdStatus()) {
                std::string_view fieldType { tagStruct.fieldType };
                for (const auto prefix : { prefix::mp3, prefix::m4a }) {
                    if (fieldType.starts_with(prefix)) {
                        fieldType.remove_prefix(prefix.size());
                        break;
                    }
                }
                if (String(std::string(fieldType), String::UTF8).upper()
                    == String(std::string(tag::clefId), String::UTF8).upper())
                    return crow::response { 400, "Clef ID is managed automatically and cannot be modified." };
            }

            const auto handler = Factory::create(fileExtension);
            if (!handler) {
                CROW_LOG_CRITICAL << logPrefix << "Cannot create a handler for " << tagStruct.filePath;
                return crow::response { 400, http::messages::unsupportedFileFormat.data() };
            }
            const auto rtag = handler->resolveTag(tagStruct.fieldType, tagStruct.formatType);
            if (!rtag.has_value()) {
                CROW_LOG_ERROR << logPrefix << rtag.error();
                return crow::response { 422, http::messages::unsupportedTag.data() };
            }
            tagStruct.fieldType = rtag.value();
            CROW_LOG_WARNING << logPrefix << "resolved tag: " << tagStruct.fieldType;

            if (application->getClefIdStatus()) id.clefId = clef::utils::generateId();
            crow::response response(handler->removeMusicTag(tagStruct, application->getClefIdStatus() ? &id.clefId : nullptr));

            if (response.code == 200) {
                return application->getTagHistoryDB().insertRemove(tagStruct, id);
            }
            return response;
        }
        CROW_LOG_ERROR << logPrefix << "request body is empty!";
        return crow::response { 400, http::messages::bodyRequired.data() };
    });

    CROW_ROUTE(app, "/api/store").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        constexpr std::string_view logPrefix { "(api/store): " };
        crow::multipart::message_view msg (req);
        const std::string_view *fileBinary { nullptr }; // Store binary data of a file
        std::string_view filepath {};
        std::string_view filename {};

        // Parse multipart map
        for (const auto & [fieldName, part] : msg.part_map) {
            // Find "path" in part map, assign file's path to filepath and log it
            if (fieldName == "path") {
                filepath = part.body;
                CROW_LOG_INFO << logPrefix << "requested path: " << filepath;
                continue;
            }
            // Find "file" in part map, assign binary data to filepart variable
            // Search for "Content-Disposition" header, search "filename" in it
            // Assign it to filename variable and log it
            if (fieldName == "file") {
                fileBinary = &part.body; // Binary data
                auto header_it = part.headers.find("Content-Disposition");
                if (header_it == part.headers.end()) {
                    CROW_LOG_ERROR << logPrefix << "The file part is missing its Content-Disposition header.";
                    return crow::response{ 400, "The file part is missing its Content-Disposition header." };
                }
                for (const auto &[key, value] : header_it->second.params) {
                    CROW_LOG_DEBUG << logPrefix << key << " = " << value;
                    if (key == "filename") {
                        filename = value;
                        break;
                    }
                }
            }
        }
        if (!application->isMountPoint(filepath)) {
            CROW_LOG_ERROR << logPrefix << "requested filepath is not a mount-point";
            return crow::response{ 403, http::messages::pathForbidden.data() };
        }

        // Now we need to store files on a drive
        fs::path destinationPath = fs::path(filepath) / fs::path(filename).filename();
        CROW_LOG_WARNING << logPrefix << "destinationPath.string(): " << destinationPath.string();
        CROW_LOG_WARNING << logPrefix << "destinationPath.filename(): " << destinationPath.filename();
        std::ofstream outfile { destinationPath, std::ios::binary };
        if (fileBinary) {
            CROW_LOG_DEBUG << logPrefix << "outFile.write() starts";
            outfile.write(fileBinary->data(), fileBinary->size());
            CROW_LOG_DEBUG << logPrefix << "outFile.write() ends";
            outfile.close();
            CROW_LOG_DEBUG << logPrefix << "outFile.close()";
            return crow::response{ 200, "File uploaded."};
        }
        CROW_LOG_CRITICAL << logPrefix << filename << ": file part not found";
        return crow::response { 400, "Multipart field 'file' is required." };
    });

    CROW_ROUTE(app, "/api/rename").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        constexpr std::string_view logPrefix { "(api/rename): " };

		if (!req.body.empty()) {
			const ordered_json body = json::parse(req.body);
			const std::string_view path = body.value("path", clef::jsonMissingValue);
			const std::string_view newName = body.value("newName", clef::jsonMissingValue);
			if (!application->isJsonValid(path, newName)) {
				CROW_LOG_ERROR << logPrefix << "Fields 'path' and 'newName' must be non-empty strings.";
				return crow::response { 400, http::messages::invalidRequestFields.data() };
			}
			if (!application->isMountPoint(path)) {
				CROW_LOG_ERROR << logPrefix << http::messages::pathForbidden;
				return crow::response { 403, http::messages::pathForbidden.data() };
			}
			const fs::path newDirictoryName { "/" / fs::path (path).parent_path() / fs::path(newName) };
			const fs::path oldDirectoryName { path };
		    std::error_code ec;
			fs::rename(oldDirectoryName, newDirictoryName, ec);
		    if (ec) {
		        CROW_LOG_ERROR << logPrefix << ec.value() << ": " << ec.message();
		        return crow::response { 400, "The file or directory could not be renamed." };
		    }
			CROW_LOG_WARNING << logPrefix << oldDirectoryName << " has been renamed to " << newDirictoryName;
			return crow::response { 200 };
		}
		CROW_LOG_ERROR << logPrefix << http::messages::bodyRequired;
        return crow::response{ 400, http::messages::bodyRequired.data() };
    });

    CROW_ROUTE(app, "/api/mkdir").methods("POST"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        constexpr std::string_view logPrefix {"(api/mkdir): "};

        if (!req.body.empty()) {
            const ordered_json body = json::parse(req.body);
            const std::string_view path = body.value("path", clef::jsonMissingValue);
            const std::string_view name = body.value("name", clef::jsonMissingValue);
            if (!application->isJsonValid(path, name)) {
                CROW_LOG_ERROR << logPrefix << "Fields 'path' and 'name' must be non-empty strings.";
                return crow::response { 400, http::messages::invalidRequestFields.data() };
            }
            if (!application->isMountPoint(path)) {
                CROW_LOG_ERROR << logPrefix << http::messages::pathForbidden;
                return crow::response { 403, http::messages::pathForbidden.data() };
            }
            const fs::path directoryName { fs::path(path) / fs::path(name) };
            if (fs::exists(directoryName)) {
                CROW_LOG_ERROR << logPrefix << http::messages::destinationExists;
                return crow::response { 500, http::messages::destinationExists.data() };
            }
            std::error_code ec;
            fs::create_directory(directoryName, ec);
            if (ec) {
                CROW_LOG_ERROR << ec.value() << ": " << ec.message();
                return crow::response { 400, "The directory could not be created." };
            }
            CROW_LOG_WARNING << logPrefix << directoryName << " has been created!";
            return crow::response{ 201 };
        }
        CROW_LOG_ERROR << logPrefix << "request body is empty!";
        return crow::response { 400, http::messages::bodyRequired.data() };
    });

    CROW_ROUTE(app, "/api/tag").methods("GET"_method)
    ([&](const crow::request &req) {
        using namespace clef;
        constexpr std::string_view logPrefix { "(api/tag): " };

        const char *path = req.url_params.get("path");
        if (path) {
            if (!application->isMountPoint(path)) {
                CROW_LOG_ERROR << logPrefix << http::messages::pathForbidden;
                return crow::response { 403, http::messages::pathForbidden.data() };
            }
            const std::string_view filePath = path;
            const std::string fileExtension = fs::path(filePath).extension().string();
            CROW_LOG_WARNING << logPrefix << "requested file: " << filePath;

            const auto handler = Factory::create(fileExtension);
            const auto result = handler->listMusicTags(filePath.data());

            if (!result.has_value()) {
                CROW_LOG_ERROR << logPrefix << "error occurred: " << result.error();
                crow::response res(500, http::messages::internalError.data());
                return res;
            }

            crow::response res(result.value().dump());
            res.set_header("Content-Type", "application/json");
            return res;
        }
        CROW_LOG_ERROR << logPrefix << http::messages::pathRequired;
        return crow::response { 400, http::messages::pathRequired.data() };
    });

    CROW_ROUTE(app, "/api/tag-registry")
    ([]() {
        using namespace clef::music::tag;
        const auto map = getTagMap();
        if (!map)
            return crow::response { 500, "The tag registry is unavailable." };

        crow::response res { map->aliases().dump() };
        res.set_header("Content-Type", "application/json");
        return res;
    });

    CROW_ROUTE(app, "/api/heartbeat")
    ([]() {
        return crow::response{ 200 };
    });

    CROW_ROUTE(app, "/api/list-v2").methods("GET"_method)
    ([&] (const crow::request &req){
        using namespace clef;
        using SortType = DirectoryListOptions::SortType;

        constexpr std::string_view logPrefix { "(api/list-v2): "};
        try {
            const char *path = req.url_params.get("path");
            const char *limit = req.url_params.get("limit");
            const char *offset = req.url_params.get("offset");
            const char *sort = req.url_params.get("sort");
            const char *asc = req.url_params.get("asc");

            if (!limit) limit = "0";
            if (!offset) offset = "0";

            auto sortType { SortType::name };
            if (sort) {
                const auto p = utils::parseSortType(sort);
                if (!p) return crow::response { 400, "Query parameter 'sort' specifies an unsupported field." };
                sortType = *p;
            }

            bool ascending { true };
            if (asc) {
                const auto p = utils::parseBool(asc);
                if (!p) return crow::response { 400, "Query parameter 'asc' must be a Boolean." };
                ascending = *p;
            }

            if (path) {
                if (!application->isMountPoint(path)) {
                    CROW_LOG_ERROR << logPrefix << http::messages::pathForbidden;
                    return crow::response { 403, http::messages::pathForbidden.data() };
                }
                fs::path fpath;
                {
                    std::string spath = path;
                    while (spath.ends_with('/'))
                        spath.pop_back();
                    fpath = std::move(spath);
                }
                if (fs::is_regular_file(fpath))
                    fpath = fpath.parent_path();
                const DirectoryListOptions q {
                    .offset = static_cast<std::size_t>(std::max(0, std::stoi(offset))),
                    .limit = static_cast<std::size_t>(std::max(0, std::stoi(limit))),
                    .ascending = ascending,
                    .sort = sortType
                };
                crow::response res { buildDirectoryTree(fpath.string(), q).dump() };
                res.set_header("Content-Type", "application/json");

                return res;
            }
            if (!path) {
                CROW_LOG_ERROR << logPrefix << http::messages::pathRequired;
                return crow::response { 400, http::messages::pathRequired.data() };
            }
            return crow::response { 500, http::messages::internalError.data() };
        } catch (std::invalid_argument&) {
            CROW_LOG_ERROR << logPrefix << "Requested limit or offset is not a number";
            return crow::response { 400, "Query parameters 'limit' and 'offset' must be integers." };
        } catch (std::out_of_range&) {
            CROW_LOG_ERROR << logPrefix << "Requested limit or offset is out of range";
            return crow::response { 400, "Query parameter 'limit' or 'offset' exceeds the supported range." };
        } catch (std::exception &e) {
            CROW_LOG_ERROR << logPrefix << e.what();
            return crow::response { 500, http::messages::internalError.data() };
        }

    });

    std::jthread scanner { [&](std::stop_token stop) {
        try {
            clef::storage::Library library { application->getDatabase().getFilename() };
            library.scan(application->getMountPoint(), stop);
        } catch (std::exception &e) {
            CROW_LOG_ERROR << "Library scan failed: " << e.what();
        }
    } };

    app.loglevel(logLevel);
    app.use_compression(crow::compression::GZIP);
    app.port(application->getPort()).multithreaded().run();

    return 0;
}
