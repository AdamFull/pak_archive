#ifndef GPAK_DATA_H
#define GPAK_DATA_H

#include <stdio.h>
#include <stdint.h>

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
	char compression_level_;
	uint32_t entry_count_;
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
	uint32_t compressed_size_;
	uint32_t uncompressed_size_;
	size_t offset_;
	uint32_t crc32_;
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
	GPAK_ERROR_EOF_BEFORE_EOS = -21,

	//crc
	GPAK_ERROR_FILE_CRC_NOT_MATCH = -22
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

enum gpak_stage_flag
{
	GPAK_STAGE_COMPRESSION,
	GPAK_STAGE_DECOMPRESSION
};
typedef enum gpak_stage_flag gpak_stage_flag_t;

typedef void (*gpak_error_handler_t)(int, void*);
typedef void (*gpak_progress_handler_t)(size_t, size_t, int32_t, void*);

struct gpak
{
	int mode_;
	pak_header_t header_;
	FILE* stream_;
	char* password_;
	struct filesystem_tree_node* root_;
	int last_error_;
	gpak_error_handler_t error_handler_;
	gpak_progress_handler_t progress_handler_;
	void* user_data_;
};
typedef struct gpak gpak_t;

struct gpak_file
{
	char* data_;
	FILE* stream_;
	uint32_t crc32_;
};
typedef struct gpak_file gpak_file_t;



struct filesystem_tree_file
{
	char* name_;
	char* path_;
	pak_entry_t entry_;
};
typedef struct filesystem_tree_file filesystem_tree_file_t;

struct filesystem_tree_node
{
	char* name_;
	struct filesystem_tree_node* parent_;
	struct filesystem_tree_node** children_;
	size_t num_children_;
	filesystem_tree_file_t** files_;
	size_t num_files_;
};
typedef struct filesystem_tree_node filesystem_tree_node_t;

struct filesystem_iterator_state
{
	filesystem_tree_node_t* node_;
	size_t child_index_;
	size_t file_index_;
};
typedef struct filesystem_iterator_state filesystem_iterator_state_t;

struct filesystem_tree_iterator
{
	filesystem_iterator_state_t* stack_;
	size_t stack_size_;
	size_t stack_capacity_;
};
typedef struct filesystem_tree_iterator filesystem_tree_iterator_t;

#endif // GPAK_DATA_H