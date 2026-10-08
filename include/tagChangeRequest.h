#ifndef CLEF_TAGCHANGEREQUEST_H
#define CLEF_TAGCHANGEREQUEST_H

#include <string>
#include <tstring.h>

#include "clefConstants.h"

namespace clef {
    struct TagChangeRequest {
        std::string filePath       { "none" };
        std::string fieldType      { "none" };
        TagLib::String replaceWhat { "none", TagLib::String::UTF8 };
        TagLib::String replaceWith { "none", TagLib::String::UTF8 };
        TagLib::String value       { "none", TagLib::String::UTF8 };

        /**
         * @brief This function validates whenever TagModification struct is valid.
         *
         * If one of TagModification members is not valid (equal to "__json_missing_value"), then a struct is not valid.
         *
         * @return True if a struct doesn't have "__json_missing_value" members, otherwise false.
         */
        [[nodiscard]] bool isValid() const {
            if (const std::string &x { jsonMissingValue.data() };
                filePath == x || fieldType == x || replaceWhat == x || replaceWith == x || value == x)
                return false;
            return true;
        }
    };
}

#endif // CLEF_TAGCHANGEREQUEST_H
