#pragma once
#include <cstdint>
#include <string_view>
#include "array"

enum Languages: uint8_t {
    None = 0,
    English = 1,
    French = 2,
    Italian = 3,
    German = 4,
    Spanish = 5,
    Russian = 6,
    Polish = 7,
    Czech = 8,
    Turkish = 9,
    Chinese = 10,
    Japan = 11,
    Portuguese = 12,
    Ukrainian = 13,
    Serbian = 14,
    Hungarian = 15,
    Korean = 16,
    Belarusian = 17,
    Romanian = 18,
    TChinese = 19,
    HChinese = 20,
    Vietnamese = 21,
    Comments = 22,
    max_chars = 23,
    LENGTH
};

constexpr std::array<const char *, LENGTH> fullLangList = {
    "None",
    "English",
    "French",
    "Italian",
    "German",
    "Spanish",
    "Russian",
    "Polish",
    "Czech",
    "Turkish",
    "Chinese",
    "Japan",
    "Portuguese",
    "Ukrainian",
    "Serbian",
    "Hungarian",
    "Korean",
    "Belarusian",
    "Romanian",
    "TChinese",
    "HChinese",
    "Vietnamese",
    "Comments",
    "max_chars"
};


namespace translate {
    /// the return policy to be used when a key is missing
    /// ReturnKey: return your key when no value is found
    /// ReturnNull: return nullptr when no value is found
    enum class ReturnPolicy {
        ReturnKey = 0,
        ReturnNull = 1,
    };

    /// gets a key from the loaded table based on your selected language.
    /// if no key is found, it returns the key you used to look up
    /// @param value the key to look up
    /// @param lang the optinal langauge to use, if no langauge is passed, then it uses the langauge set via set_default_language
    /// @param return_policy the optional return_policy to use, default is ReturnKey
    /// @return returns your translated key or
    const char *localize(std::string_view value, Languages lang = Languages::None,
                         ReturnPolicy return_policy = ReturnPolicy::ReturnKey);

    /// see translate::localize
    /// explicitly calls localize with ReturnPolicy::ReturnKey
    inline const char *localize_rKey(std::string_view value, Languages lang = Languages::None) {
        return localize(value, lang, ReturnPolicy::ReturnKey);
    }

    /// see translate::localize
    /// explicitly calls localize with ReturnPolicy::ReturnNull
    inline const char *localize_rNull(std::string_view value, Languages lang = Languages::None) {
        return localize(value, lang, ReturnPolicy::ReturnNull);
    }

    /// sets the default language returned when calling localize
    /// the default langauge is english
    /// @param lang langauge enum value
    void set_default_language(Languages lang);

    void load_csv(std::string_view path);
}
