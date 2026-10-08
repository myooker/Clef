#ifndef CLEF_FACTORY_H
#define CLEF_FACTORY_H

#include <memory>
#include <string_view>

#include "../../include/interface.h"

namespace clef::music::handler {
    class Factory {
        public:
        static std::unique_ptr<Interface> create(std::string_view extension);
    };
}

#endif // CLEF_FACTORY_H
