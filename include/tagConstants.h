#ifndef CLEF_TAGCONSTANTS_H
#define CLEF_TAGCONSTANTS_H

#include <string_view>

namespace clef::music {
    enum class format {
        ID3v24, ID3v23,
        FLAC, M4A, OGG,
        OPUS, AAC, WMA,
        WAV, AIF, AIFF,
        ALAC,
    };

    namespace tag {
        namespace prefix {
            constexpr std::string_view mp3 { "TXXX:" };
            constexpr std::string_view m4a { "----:com.apple.iTunes:" };
        }
        // Program-defined tags
        constexpr std::string_view clefId { "Clef_ID" };
        constexpr std::string_view rteId { "RTEID" };
    }
}

#endif // CLEF_TAGCONSTANTS_H
