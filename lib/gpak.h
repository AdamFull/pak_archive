#ifndef gpak_H
#define gpak_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"

	#include <stdio.h>

	enum gpak_header_compression_algorithm
	{
		GPAK_HEADER_COMPRESSION_NONE = 0,
		GPAK_HEADER_COMPRESSION_DEFLATE = 1 << 0,
		GPAK_HEADER_COMPRESSION_LZ4 = 1 << 1,
		GPAK_HEADER_COMPRESSION_ZST = 1 << 2
	};
	typedef enum gpak_header_compression_algorithm gpak_header_compression_algorithm_t;

	enum gpak_compression_deflate
	{
		GPAK_COMPRESSION_DEFLATE_NONE = 0,
		GPAK_COMPRESSION_DEFLATE_FAST = 1,
		GPAK_COMPRESSION_DEFLATE_BEST = 9
	};
	typedef enum gpak_compression_deflate gpak_compression_deflate_t;

	enum gpak_compression_lz4
	{
		GPAK_COMPRESSION_LZ4_NONE = 0,
		GPAK_COMPRESSION_LZ4_FAST = 3,
		GPAK_COMPRESSION_LZ4_MEDIUM = 6,
		GPAK_COMPRESSION_LZ4_BEST = 12,
	};
	typedef enum gpak_compression_lz4 gpak_compression_lz4_t;

	enum gpak_compression_zstd
	{
		GPAK_COMPRESSION_ZST_NONE = 0,
		GPAK_COMPRESSION_ZST_FAST = 5,
		GPAK_COMPRESSION_ZST_MEDIUM = 10,
		GPAK_COMPRESSION_ZST_BEST = 20,
	};
	typedef enum gpak_compression_zstd gpak_compression_zstd_t;

	enum gpak_header_encryption_mode
	{
		GPAK_HEADER_ENCRYPTION_NONE = 0,
		GPAK_HEADER_ENCRYPTION_AES128 = 1 << 0,
		GPAK_HEADER_ENCRYPTION_AES256 = 1 << 1
	};
	typedef enum gpak_header_encryption_mode gpak_header_encryption_mode_t;

	struct gpak_header
	{
		char format_[5];
		gpak_header_compression_algorithm_t compression_;
		gpak_header_encryption_mode_t encryption_;
		int compression_level_;
		size_t compressed_size_;
		size_t uncompressed_size_;
		size_t entry_count_;
		int crc32_;
	};
	typedef struct gpak_header pak_header_t;


	enum gpak_entry_type
	{
		GPAK_ENTRY_NULL = 0,
		GPAK_ENTRY_DIRECTORY = 1,
		GPAK_ENTRY_FILE = 2
	};
	typedef enum gpak_entry_type gpak_entry_type_t;

	struct gpak_entry_header
	{
		char name[256];
		size_t offset_;
		size_t compressed_size_;
		size_t uncompressed_size_;
		int crc32_;
	};
	typedef struct gpak_entry_header pak_entry_t;

	enum gpak_error
	{
		GPAK_ERROR_OK = 0,
		GPAK_ERROR_ARCHIVE_IS_NULL = -1,
		GPAK_ERROR_STREAM_IS_NULL = -2,
		GPAK_ERROR_INCORRECT_MODE = -3,
		GPAK_ERROR_INCORRECT_HEADER = -4,
		GPAK_ERROR_OPEN_FILE = -5,
		GPAK_ERROR_WRITE = -6,
		GPAK_ERROR_READ = -7,

		GPAK_ERROR_FILE_NOT_FOUND = -8,

		// Deflate
		GPAK_ERROR_DEFLATE_INIT = -9,
		GPAK_ERROR_DEFLATE_FAILED = -10,
		GPAK_ERROR_INFLATE_INIT = -11,
		GPAK_ERROR_INFLATE_FAILED = -12,

		// LZ4 errors
		GPAK_ERROR_LZ4_WRITE_OPEN = -13,
		GPAK_ERROR_LZ4_WRITE = -14,
		GPAK_ERROR_LZ4_WRITE_CLOSE = -15,
		GPAK_ERROR_LZ4_READ_OPEN = -16,
		GPAK_ERROR_LZ4_READ = -17,
		GPAK_ERROR_LZ4_READ_CLOSE = -18,
		GPAK_ERROR_LZ4_DECOMPRESS = -19,

		// ZSTD
		GPAK_ERROR_EMPTY_INPUT = -20,
		GPAK_ERROR_EOF_BEFORE_EOS = -21
	};
	typedef enum gpak_error gpak_error_t;

	enum gpak_mode_flag
	{
		GPAK_MODE_NONE = 0,
		GPAK_MODE_CREATE = 1 << 0,			// Analog write byte
		GPAK_MODE_READ_ONLY = 1 << 1,		// Analog read byte
		GPAK_MODE_UPDATE = 1 << 2
	};
	typedef enum gpak_mode_flag gpak_mode_flag_t;

	typedef void (*gpak_error_handler_t)(int, const char*);
	typedef void (*gpak_progress_handler_t)(size_t, size_t);

	struct gpak
	{
		int mode_;
		pak_header_t header_;
		FILE* stream_;
		char* password_;
		struct filesystem_tree_node* root_;
		int last_error_;
		gpak_error_handler_t* error_handler_;
		gpak_progress_handler_t* progress_handler_;
	};
	typedef struct gpak gpak_t;

	struct gpak_file
	{
		size_t size_;
		char* data_;
		FILE* stream_;
	};
	typedef struct gpak_file gpak_file_t;

	GPAK_API gpak_t* gpak_open(const char* _path, int _mode);
	GPAK_API int gpak_close(gpak_t* _pak);

	GPAK_API void gpak_set_compression_algorithm(gpak_t* _pak, int _algorithm);
	GPAK_API void gpak_set_compression_level(gpak_t* _pak, int _level);
	GPAK_API void gpak_set_encryption_mode(gpak_t* _pak, int _mode);
	GPAK_API void gpak_set_encryption_password(gpak_t* _pak, const char* _password);

	GPAK_API int gpak_add_directory(gpak_t* _pak, const char* _internal_path);
	GPAK_API int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path);

	GPAK_API struct filesystem_tree_node* gpak_get_root(gpak_t* _pak);
	GPAK_API struct filesystem_tree_node* gpak_find_directory(gpak_t* _pak, const char* _path);
	GPAK_API struct filesystem_tree_file* gpak_find_file(gpak_t* _pak, const char* _path);

	// File functions
	GPAK_API gpak_file_t* gpak_fopen(gpak_t* _pak, const char* _path);
	GPAK_API int gpak_fgetc(gpak_file_t* _file);
	GPAK_API char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max);
	GPAK_API int gpak_ungetc(gpak_file_t* _file, int _character);
	GPAK_API size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file);
	GPAK_API long gpak_ftell(gpak_file_t* _file);
	GPAK_API long gpak_fseek(gpak_file_t* _file, long _offset, int _origin);
	GPAK_API long gpak_feof(gpak_file_t* _file);
	GPAK_API void gpak_fclose(gpak_file_t* _file);

#ifdef __cplusplus
}
#endif

#endif // gpak_H