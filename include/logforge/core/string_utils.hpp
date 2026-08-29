#pragma once

#include <algorithm>
#include <cctype>
#include <string>
#include <string_view>

namespace logforge {

/// @brief Convert a string to lowercase (ASCII).
[[nodiscard]] inline std::string to_lower(std::string_view input) {
    std::string result(input);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return result;
}

/// @brief Case-insensitive equality check for two string views.
[[nodiscard]] inline bool equals_ignore_case(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace logforge
