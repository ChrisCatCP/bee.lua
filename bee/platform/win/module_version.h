#pragma once

#include <bee/utility/dynarray.h>

#include <string>
#include <string_view>
#include <cstdint>

namespace bee::win {
    class module_version {
    public:
        module_version(const wchar_t* module_path);
        bool select_language(uint16_t langid) noexcept;
        std::wstring_view get_value(const wchar_t* key) const;

    protected:
        struct translation {
            uint16_t language;
            uint16_t code_page;
        };
        size_t current_;
        dynarray<translation> translation_;
        dynarray<std::byte> version_info_;
    };
}
