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