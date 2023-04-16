#ifndef PAK_ARCHIVE_EXPORT_H
#define PAK_ARCHIVE_EXPORT_H

#ifdef _WIN32
    #ifdef PAK_ARCHIVE_EXPORTS
        #define PAK_ARCHIVE_API __declspec(dllexport)
    #else
        #define PAK_ARCHIVE_API __declspec(dllimport)
    #endif
#else
    #define PAK_ARCHIVE_API
#endif

#endif // PAK_ARCHIVE_EXPORT_H