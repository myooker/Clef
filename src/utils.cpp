#include "../include/utils.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <random>

namespace clef::utils {
    bool naturalLess(std::string_view l, std::string_view r) {
        std::size_t i = 0, j = 0;
        while (i < l.size() && j < r.size()) {
            unsigned char cl = l[i], cr = r[j];
            // If both strings starts with digits
            if (isdigit(cl) && isdigit(cr)) {
                // end of numbers in a string
                std::size_t ei = i, ej = j;
                while (ei < l.size() && isdigit(static_cast<unsigned char>(l[ei]))) ei++; //find end of numbers in l
                while (ej < r.size() && isdigit(static_cast<unsigned char>(r[ej]))) ej++; //find end of numbers in r

                while (i < ei && l[i] == '0') i++; // skip leading zeros in l
                while (j < ej && r[j] == '0') j++; // skip leading zeros in r

                // Compare length of number chunks
                // if different, shorts means lower
                if (ei - i != ej - j) return ei-i < ej-j;

                for (; i < ei; i++, j++)
                    if (l[i] != r[j]) return l[i] < r[j];
                continue;
            }
            // lower both chars
            int ll = std::tolower(cl);
            int lr = std::tolower(cr);
            if (ll != lr) return ll < lr; // if different then return
            i++; j++; // otherwise iterate further
        }
        return (l.size() - i) < (r.size() - j);
    }

    bool nameLess(const FileEntity &a, const FileEntity &b) {
        if (naturalLess(a.name, b.name)) return true;
        if (naturalLess(b.name, a.name)) return false;
        return a.name < b.name;
    }

    bool entityLess(const FileEntity &a, const FileEntity &b, const DirectoryListOptions &q) {
        using SortType = DirectoryListOptions::SortType;

        // Folders always first
        const bool ad = a.type == EntityType::directory;
        const bool bd = b.type == EntityType::directory;

        if (ad != bd) return ad;

        constexpr auto cmp3 = [](auto x, auto y) { return x < y ? -1 : (y < x ? 1 : 0); };

        int c = 0;
        if (q.sort == SortType::size)      c = cmp3(a.size, b.size);
        else if (q.sort == SortType::type) c = cmp3(a.type, b.type);

        if (c != 0) return q.ascending ? c < 0 : c > 0;
        return q.ascending ? nameLess(a, b) : nameLess(b, a);
    }

    std::optional<bool> parseBool(std::string_view a) {
        std::string s(a);
        std::ranges::transform(s, s.begin(), [](const unsigned char c) {
            return std::tolower(c);
        });

        if (s == "true" || s == "1" || s == "yes" || s == "on") return true;
        if (s == "false" || s == "0" || s == "no" || s == "off") return false;

        return std::nullopt;
    }

    std::optional<DirectoryListOptions::SortType> parseSortType(std::string_view a) {
        using SortType = DirectoryListOptions::SortType;
        if (a == "name") return SortType::name;
        if (a == "size") return SortType::size;
        if (a == "type") return SortType::type;

        return std::nullopt;
    }

    std::string generateId(const std::size_t t) {
        static constexpr std::string_view ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

        static std::mt19937 rng(std::random_device{}());
        static std::uniform_int_distribution<std::size_t> distribution(0, ALPHABET.size() - 1);

        std::string id;
        id.reserve(t);

        for (std::size_t i = 0; i < t; i++) {
            id.push_back(ALPHABET[distribution(rng)]);
        }

        return id;
    }

    std::string getExtension(const std::string &path) {
        return std::filesystem::path{path}.extension().string();
    }
}
