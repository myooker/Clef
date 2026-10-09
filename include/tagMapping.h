#ifndef CLEF_TAGMAPPING_H
#define CLEF_TAGMAPPING_H

#include <algorithm>
#include <cstddef>
#include <expected>
#include <fstream>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <nlohmann/json.hpp>

#include "tagConstants.h"

namespace clef::music::tag {
    using json = nlohmann::json;
    class TagMapping {
    private:
        const json m_map;
        const json m_amap;
        std::unordered_map<std::string, const json*> m_aumap;
        std::size_t m_maphash;

        static json buildAliasMap(const json &src) {
            json out = json::object();
            for (const auto &[fname, values] : src.items()) {
                json a = json::array();
                for (const auto &[cname, cvalues] : values.items()) {
                    if (cvalues.is_array())
                        for (const auto &x : cvalues) a += x;
                    else
                        a += cvalues;
                }
                out[fname] = a;
            }
            return out;
        }

        std::unordered_map<std::string, const json*> buildUnorderedAliasMap() {
            std::unordered_map<std::string, const json*> t;
            for (const auto &[key, values] : m_amap.items()) {
                for (const auto &value : values.items()) {
                    t.emplace(key, &value.value());
                }
            }
            return t;
        }

    public:
        explicit TagMapping(std::ifstream f)
            : m_map(json::parse(f)),
              m_amap(buildAliasMap(m_map)),
              m_aumap(buildUnorderedAliasMap()),
              m_maphash(std::hash<std::string>{}(m_map.dump())) {}

        std::size_t getMapHash() const { return m_maphash; }

        [[nodiscard]]
        const json &aliases() const noexcept { return m_amap; }

        /**
         * @brief Find occurrence of fname in mapping table
         *
         * @param fname frontend name (e.g. TALB, ALBUM, ©alb...)
         * @param cname codec name (e.g. id3v2, vorbis, mp4...)
         * @param f format type, ID3v24 by default
         * @return string_view to codec name, unexpected otherwise
         */
        [[nodiscard]]
        std::expected<const std::string_view, std::string>
        find(const std::string &fname, const std::string &cname, format f = format::ID3v24) const {
            if (auto fname_it = m_map.find(fname); fname_it != m_map.end()) {
                if (auto cname_it = fname_it->find(cname); cname_it != fname_it->end()) {
                    if (cname_it.value().is_array())
                        return cname_it->get<const std::string_view>();
                    return cname_it->get<const std::string_view>();
                }
            }

            return std::unexpected(fname + " was not found for " + cname);
        }

        [[nodiscard]]
        bool contain(const std::string &fname) const {
            return std::ranges::any_of(m_amap, [&, needle = json(fname)](const json &a) {
                return std::ranges::find(a, needle) != a.end();
            });
        }

        /**
         * @brief
         * @param fname - frontend name
         * @param cname - codec name
         * @param f - format type
         * @return
         */
        [[nodiscard]]
        std::expected<std::string, std::string>
        resolve(const std::string &fname, const std::string &cname, format f = format::ID3v24) const {
            // An empty name would resolve to an empty frame ID further down.
            if (fname.empty())
                return std::unexpected("empty fname");

            if (const auto r = find(fname, cname)) {
                if (r.has_value()) {
                    return r.value().data();
                }
            }

            if (m_map.contains(fname))
                return std::unexpected(fname + " has no " + cname + " mapping");

            // If fname appears in mapping table, return as-is
            if (contain(fname))
                return fname;

            if (fname.starts_with(prefix::mp3) || fname.starts_with(prefix::m4a))
                return fname;

            // Otherwise consider tag as user-defined and add corresponding prefix
            if (cname == "id3v2")
                return prefix::mp3.data() + fname;
            if (cname == "mp4")
                return prefix::m4a.data() + fname;

            return std::unexpected(fname + " has no " + cname + " mapping");
        }
    };

    TagMapping *getTagMap();
}

#endif // CLEF_TAGMAPPING_H
