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
    #ifdef RAUM_EXPORT
        #define RAUM_API __declspec(dllexport)
    #else
        #define RAUM_API __declspec(dllimport)
    #endif
#else
    #define RAUM_API
#endif