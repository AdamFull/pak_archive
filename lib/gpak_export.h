#ifndef GPAK_EXPORT_H
#define GPAK_EXPORT_H

#ifdef _WIN32
    #ifdef GPAK_EXPORTS
        #define GPAK_API __declspec(dllexport)
    #else
        #define GPAK_API
    #endif
#else
    #define GPAK_API
#endif

#endif // GPAK_EXPORT_H