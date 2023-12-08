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