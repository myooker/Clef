#include "oggTag.h"

#include <utility>

#include <crow/logging.h>
#include <fileref.h>
#include <oggflacfile.h>
#include <opusfile.h>
#include <speexfile.h>
#include <vorbisfile.h>

#include "../../include/tagMapping.h"
#include "oggFlac.h"
#include "oggOpus.h"
#include "oggSpeex.h"
#include "oggVorbis.h"

using namespace clef::music::handler;
using namespace clef::music::tag;

std::unique_ptr<Interface> OggTag::codecHandler(const std::string &filePath) {
    const TagLib::FileRef fileRef { filePath.c_str() };

    if (fileRef.isNull()) {
        CROW_LOG_ERROR << "(" << __func__ << ") " << filePath << " is invalid!";
        return nullptr;
    }

    const auto file = fileRef.file();

    if (dynamic_cast<TagLib::Ogg::Vorbis::File *>(file)) {
        return std::make_unique<OggVorbis>();
    }
    if (dynamic_cast<TagLib::Ogg::FLAC::File *>(file)) {
        return std::make_unique<OggFlac>();
    }
    if (dynamic_cast<TagLib::Ogg::Opus::File *>(file)) {
        return std::make_unique<OggOpus>();
    }
    if (dynamic_cast<TagLib::Ogg::Speex::File *>(file)) {
        return std::make_unique<OggSpeex>();
    }
    return nullptr;
}

std::expected<json, std::string> OggTag::listMusicTags(const std::string &filePath) {
    return codecHandler(filePath)->listMusicTags(filePath);
}

crow::response OggTag::removeMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
   return codecHandler(tagStruct.filePath)->removeMusicTag(tagStruct, clefId);
}

crow::response OggTag::addMusicTag(const TagChangeRequest &tagStruct, std::string *clefId) {
   return codecHandler(tagStruct.filePath)->addMusicTag(tagStruct, clefId);
}

crow::response OggTag::editMusicTags(const TagChangeRequest &tagStruct, std::string *clefId) {
   return codecHandler(tagStruct.filePath)->editMusicTags(tagStruct, clefId);
}

std::expected<std::string, std::string> OggTag::resolveTag(const std::string_view tag) {
    auto resolve = getTagMap()->resolve(tag.data(), m_type.data());
    if (resolve.has_value())
        return resolve.value();
    return std::unexpected(std::move(resolve).error());
}
