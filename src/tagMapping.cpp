#include "../include/tagMapping.h"

#include <fstream>
#include <stdexcept>
#include <utility>

#include <crow/logging.h>

namespace clef::music::tag {
    TagMapping* getTagMap() {
        std::ifstream f { "data/mapping.json" };
        if (!f.is_open()) {
            CROW_LOG_ERROR << __PRETTY_FUNCTION__ << ": mapping.json file was not found.";
            throw std::runtime_error("mapping.json file was not found");
        }
        static TagMapping tm { (std::move(f)) };
        f.close();
        return &tm;
    }
}
