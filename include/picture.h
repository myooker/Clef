#ifndef CLEF_PICTURE_H
#define CLEF_PICTURE_H

#include <string>

#include <crow/http_response.h>

namespace clef::music::tag {
    // Demo implementation
    struct Picture {
        crow::response response { 500, "Not found" };
        std::string mimeType {};
        std::string data {};
        int height {};
        int width {};
    };
}

#endif // CLEF_PICTURE_H
