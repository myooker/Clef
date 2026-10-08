#include "factory.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "flac.h"
#include "mpeg4.h"
#include "mpeg.h"
#include "oggOpus.h"
#include "oggTag.h"

using namespace clef::music::handler;

std::unique_ptr<Interface> Factory::create(const std::string_view extension) {
    std::string ext { extension };
    std::ranges::transform(extension, ext.begin(), [](unsigned char c) {
       return static_cast<char>(std::tolower(c));
    });
    if (ext == ".mp3")
        return std::make_unique<Mpeg>();
    if (ext == ".flac")
        return std::make_unique<Flac>();
    if (ext == ".m4a")
        return std::make_unique<Mpeg4>();
    if (ext == ".ogg")
        return std::make_unique<OggTag>();
    if (ext == ".opus")
        return std::make_unique<OggOpus>();

    return nullptr;
}
