#ifndef gpak_H
#define gpak_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "pak_export.h"

	#include <stdio.h>

	enum gpak_header_compression_algorithm
	{
		GPAK_HEADER_COMPRESSION_NONE = 0,
		GPAK_HEADER_COMPRESSION_DEFLATE = 1 << 0,
		GPAK_HEADER_COMPRESSION_LZ4 = 1 << 1,
		GPAK_HEADER_COMPRESSION_ZST = 1 << 2
	};
	typedef enum gpak_header_compression_algorithm gpak_header_compression_algorithm_t;

	enum gpak_header_encryption_mode
	{
		GPAK_HEADER_ENCRYPTION_NONE = 0,
		GPAK_HEADER_ENCRYPTION_AES128 = 1 << 0,
		GPAK_HEADER_ENCRYPTION_AES256 = 1 << 1
	};
	typedef enum gpak_header_encryption_mode gpak_header_encryption_mode_t;

	struct gpak_header
	{
		char format_[4];
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
		GPAK_ERROR_FILE_NOT_FOUND = -8
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

	struct gpak
	{
		int mode_;
		pak_header_t header_;
		FILE* stream_;
		char* password_;
		struct filesystem_tree_node* root_;
		int last_error_;
	};
	typedef struct gpak gpak_t;

	struct gpak_file
	{
		char* data_;
		const char* begin_;
		const char* end_;
		const char* current_;
		int ungetc_buffer_;
	};
	typedef struct gpak_file gpak_file_t;

	gpak_t* gpak_open(const char* _path, int _mode);
	int gpak_close(gpak_t* _pak);

	void gpak_set_compression_algorithm(gpak_t* _pak, gpak_header_compression_algorithm_t _algorithm);
	void gpak_set_compression_level(gpak_t* _pak, int _level);
	void gpak_set_encryption_mode(gpak_t* _pak, gpak_header_compression_algorithm_t _mode);
	void gpak_set_encryption_password(gpak_t* _pak, const char* _password);

	int gpak_add_directory(gpak_t* _pak, const char* _internal_path);
	int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path);

	struct filesystem_tree_node* gpak_get_root(gpak_t* _pak);
	struct filesystem_tree_node* gpak_find_directory(gpak_t* _pak, const char* _path);
	struct filesystem_tree_file* gpak_find_file(gpak_t* _pak, const char* _path);

	// File functions
	gpak_file_t* gpak_fopen(gpak_t* _pak, const char* _path);
	int gpak_fgetc(gpak_file_t* _file);
	char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max);
	int gpak_ungetc(gpak_file_t* _file, int _character);
	size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file);
	long gpak_ftell(gpak_file_t* _file);
	long gpak_fseek(gpak_file_t* _file, long _offset, int _origin);
	long gpak_feof(gpak_file_t* _file);
	void gpak_fclose(gpak_file_t* _file);

#ifdef __cplusplus
}
#endif

#endif // gpak_H