#ifndef PAK_ARCHIVE_H
#define PAK_ARCHIVE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pak_export.h"

#include <stdio.h>

struct pak_archive_header;
struct pak_archive_entry_header;
struct pak_archive;

typedef struct pak_archive_header pak_header_t;
typedef struct pak_archive_entry_header pak_entry_t;
typedef struct pak_archive pak_t;


enum pak_archive_header_compression_algorithm
{
    PAK_ARCHIVE_HEADER_COMPRESSION_NONE = 0,
    PAK_ARCHIVE_HEADER_COMPRESSION_DEFLATE = 1 << 0,
    PAK_ARCHIVE_HEADER_COMPRESSION_LZ4 = 1 << 1,
    PAK_ARCHIVE_HEADER_COMPRESSION_ZST = 1 << 2
};
typedef enum pak_archive_header_compression_algorithm pak_archive_header_compression_algorithm_t;

enum pak_archive_header_encryption_mode
{
    PAK_ARCHIVE_HEADER_ENCRYPTION_NONE = 0,
    PAK_ARCHIVE_HEADER_ENCRYPTION_AES128 = 1 << 0,
    PAK_ARCHIVE_HEADER_ENCRYPTION_AES256 = 1 << 1
};
typedef enum pak_archive_header_encryption_mode pak_archive_header_encryption_mode_t;

struct pak_archive_header
{
    char format_[3];
    pak_archive_header_compression_algorithm_t compression_;
    pak_archive_header_encryption_mode_t encryption_;
    int compression_level_;
    size_t compressed_size_;
    size_t uncompressed_size_;
    size_t entry_count_;
};


enum pak_archive_entry_type
{
    PAK_ARCHIVE_ENTRY_NULL = 0,
    PAK_ARCHIVE_ENTRY_DIRECTORY = 1,
    PAK_ARCHIVE_ENTRY_FILE = 2
};
typedef enum pak_archive_entry_type pak_archive_entry_type_t;

struct pak_archive_entry_header
{
    char name[256];
    size_t offset_;
    size_t compressed_size_;
    size_t uncompressed_size_;
    pak_archive_entry_type_t type_;
};

enum pak_archive_error
{
    PAK_ARCHIVE_ERROR_OK = 0,
    PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL = -1,
    PAK_ARCHIVE_ERROR_STREAM_IS_NULL = -2,
    PAK_ARCHIVE_ERROR_INCORRECT_MODE = -3,
    PAK_ARCHIVE_ERROR_INCORRECT_HEADER = -4,
    PAK_ARCHIVE_ERROR_OPEN_FILE = -5,
    PAK_ARCHIVE_ERROR_WRITE = -6,
    PAK_ARCHIVE_ERROR_READ = -7
};
typedef enum pak_archive_error pak_archive_error_t;

enum pak_archive_mode_flag
{
    PAK_ARCHIVE_MODE_NONE = 0,
    PAK_ARCHIVE_MODE_CREATE = 1 << 0,       // Analog write byte
    PAK_ARCHIVE_MODE_READ_ONLY = 1 << 1     // Analog read byte
};
typedef enum pak_archive_mode_flag pak_archive_mode_flag_t;

struct pak_archive
{
    int mode_;
    pak_header_t header_;
    FILE* stream_;
    int last_error_;
};

 pak_t* pak_archive_open(const char* _path, int _mode);
 int pak_archive_close(pak_t* _pak);

 int pak_archive_add_directory(pak_t* _pak, const char* _internal_path);
 int pak_archive_add_file(pak_t* _pak, const char* _external_path, const char* _internal_path);

#ifdef __cplusplus
}
#endif

#endif // PAK_ARCHIVE_H