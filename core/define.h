#pragma once
#ifndef RAUM_RELEASE
    #define RAUM_DEBUG 1
#endif

#if defined(_WIN32)
    #define WINDOWS 1
#endif

#ifdef RAUM_DEBUG
    #define DEBUG(exp) exp
#else
    #define DEBUG(exp)
#endif

#ifdef WINDOWS
    #ifdef RAUM_IMPORT
        #define RAUM_API __declspec(dllimport)
    #else
        #define RAUM_API __declspec(dllexport)
    #endif
#else
    #define RAUM_API
#endif

#include <stddef.h>
#include <string>

// https://www.reddit.com/r/cpp/comments/phqxiq/transparent_find_is_a_pain_to_get_right/
struct hash_string {
    using is_transparent = void;
    size_t operator()(const std::string& v) const
    {
        return std::hash<std::string>{}(v);
    }
    size_t operator()(const char*v) const
    {
        return std::hash<std::string_view>{}(v);
    }

    size_t operator()(const std::string_view &v) const
    {
        return std::hash<std::string_view>{}(v);
    }
};