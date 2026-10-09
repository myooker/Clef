cmake_minimum_required(VERSION 3.25)
project(Clef)

set(CMAKE_CXX_STANDARD 23)

find_package(PkgConfig REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(ZLIB REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib>=2.0)

include(FetchContent)
FetchContent_Declare(
    SQLiteCpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.3
)
FetchContent_MakeAvailable(SQLiteCpp)

set(CLEF_SOURCES
    src/main.cpp
        src/database/tagHistory.cpp
    src/tagMapping.cpp
    src/utils.cpp

    src/format_handlers/factory.cpp
    src/format_handlers/flac.cpp
    src/format_handlers/mpeg.cpp
    src/format_handlers/mpeg4.cpp
    src/format_handlers/oggFlac.cpp
    src/format_handlers/oggOpus.cpp
    src/format_handlers/oggSpeex.cpp
    src/format_handlers/oggTag.cpp
    src/format_handlers/oggVorbis.cpp
        src/clef.cpp
        src/database/users.cpp
)

set(CLEF_HEADERS
        include/authentication/authMiddleware.h
    include/clef.h
    include/clefConstants.h
    include/directoryListing.h
    include/interface.h
    include/music.h
    include/picture.h
    include/tagChangeRequest.h
    include/tagConstants.h
        include/database/tagHistory.h
    include/tagMapping.h
        include/database/users.h
    include/utils.h

    src/format_handlers/factory.h
    src/format_handlers/flac.h
    src/format_handlers/mpeg.h
    src/format_handlers/mpeg4.h
    src/format_handlers/oggFlac.h
    src/format_handlers/oggOpus.h
    src/format_handlers/oggSpeex.h
    src/format_handlers/oggTag.h
    src/format_handlers/oggVorbis.h
        include/authentication/sessionStore.h
)

add_executable(Clef ${CLEF_SOURCES} ${CLEF_HEADERS})

target_include_directories(Clef PRIVATE ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(Clef PRIVATE
    ${TAGLIB_LIBRARIES}
    nlohmann_json::nlohmann_json
    SQLiteCpp
    sqlite3
    pthread
    dl
    ZLIB::ZLIB
)

target_compile_options(Clef PRIVATE ${TAGLIB_CFLAGS_OTHER})
target_compile_definitions(Clef PRIVATE
    $<$<CONFIG:Debug>:APP_DEBUG>
    CROW_ENABLE_COMPRESSION
)
