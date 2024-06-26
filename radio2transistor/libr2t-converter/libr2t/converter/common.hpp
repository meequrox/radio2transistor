#pragma once

#include <libr2t/reader/reader.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace r2t::converter {
class Common {
   public:
    explicit Common(const reader::Reader& reader);

    nlohmann::json json() const;
    std::string m3u() const;

   private:
    const reader::Reader& reader_;
    nlohmann::json json_;
    std::string m3u_;

    void build_json();
    void build_m3u();
};
}  // namespace r2t::converter
