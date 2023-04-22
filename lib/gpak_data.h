#ifndef GPAK_DATA_H
#define GPAK_DATA_H

#include <stdio.h>
#include <stdint.h>

/**
 * @brief Enumeration representing the compression algorithms for G-PAK headers.
 *
 * This enumeration contains values representing the different compression algorithms that can be used for G-PAK archive headers. The available compression algorithms are none, deflate, LZ4, and Zstandard (ZST).
 */
enum gpak_header_compression_algorithm
{
	GPAK_HEADER_COMPRESSION_NONE = 0,       /**< No compression. */
	GPAK_HEADER_COMPRESSION_DEFLATE = 1 << 0, /**< Deflate compression algorithm. */
	GPAK_HEADER_COMPRESSION_LZ4 = 1 << 1,    /**< LZ4 compression algorithm. */
	GPAK_HEADER_COMPRESSION_ZST = 1 << 2     /**< Zstandard (ZST) compression algorithm. */
};

/**
 * @brief Typedef for the gpak_header_compression_algorithm enumeration.
 *
 * This typedef is used to create an alias for the gpak_header_compression_algorithm enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_header_compression_algorithm gpak_header_compression_algorithm_t;

/**
 * @brief Enumeration representing the deflate compression levels for G-PAK.
 *
 * This enumeration contains values representing the different compression levels that can be used with the deflate algorithm in G-PAK operations. The available compression levels are none, fast, and best.
 */
enum gpak_compression_deflate
{
	GPAK_COMPRESSION_DEFLATE_NONE = 0, /**< No compression. */
	GPAK_COMPRESSION_DEFLATE_FAST = 1, /**< Fast compression. */
	GPAK_COMPRESSION_DEFLATE_BEST = 9  /**< Best compression. */
};

/**
 * @brief Typedef for the gpak_compression_deflate enumeration.
 *
 * This typedef is used to create an alias for the gpak_compression_deflate enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_compression_deflate gpak_compression_deflate_t;

/**
 * @brief Enumeration representing the compression levels for LZ4 algorithm in G-PAK.
 *
 * This enumeration contains values representing the different compression levels that can be used with the LZ4 compression algorithm in G-PAK. The higher the compression level, the better the compression ratio but the slower the compression speed.
 */
enum gpak_compression_lz4
{
	GPAK_COMPRESSION_LZ4_NONE = 0, /**< No compression is applied. */
	GPAK_COMPRESSION_LZ4_FAST = 3, /**< Fast compression with lower compression ratio. */
	GPAK_COMPRESSION_LZ4_MEDIUM = 6, /**< Medium compression level, balancing compression speed and ratio. */
	GPAK_COMPRESSION_LZ4_BEST = 12, /**< Best compression level with higher compression ratio but slower compression speed. */
};

/**
 * @brief Typedef for the gpak_compression_lz4 enumeration.
 *
 * This typedef is used to create an alias for the gpak_compression_lz4 enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_compression_lz4 gpak_compression_lz4_t;

/**
 * @brief Enumeration representing the Zstandard compression levels for G-PAK.
 *
 * This enumeration contains values representing the different Zstandard compression levels that can be used in G-PAK. The compression levels range from no compression (GPAK_COMPRESSION_ZST_NONE) to best compression (GPAK_COMPRESSION_ZST_BEST).
 */
enum gpak_compression_zstd
{
	GPAK_COMPRESSION_ZST_NONE = 0, /**< No compression. */
	GPAK_COMPRESSION_ZST_FAST = 5, /**< Fast compression, lower compression ratio. */
	GPAK_COMPRESSION_ZST_MEDIUM = 10, /**< Medium compression, balance between speed and compression ratio. */
	GPAK_COMPRESSION_ZST_BEST = 22, /**< Best compression, higher compression ratio but slower speed. */
};

/**
 * @brief Typedef for the gpak_compression_zstd enumeration.
 *
 * This typedef is used to create an alias for the gpak_compression_zstd enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_compression_zstd gpak_compression_zstd_t;

/**
 * @brief Structure representing the header of a G-PAK archive.
 *
 * This structure contains information about the G-PAK archive itself, including the format identifier, compression algorithm, compression level, the number of entries in the archive, and the dictionary size.
 */
struct gpak_header
{
	char format_[5]; /**< A null-terminated string representing the G-PAK archive format identifier. */
	gpak_header_compression_algorithm_t compression_; /**< The compression algorithm used in the G-PAK archive. */
	char compression_level_; /**< The compression level applied to the entries in the G-PAK archive. */
	uint32_t entry_count_; /**< The number of entries in the G-PAK archive. */
	uint32_t dictionary_size_; /**< The size of the dictionary used for compression in the G-PAK archive. */
};

/**
 * @brief Typedef for the gpak_header structure.
 *
 * This typedef is used to create an alias for the gpak_header structure, providing a more convenient way to use the structure in the code.
 */
typedef struct gpak_header pak_header_t;


/**
 * @brief Structure representing an entry header in a G-PAK archive.
 *
 * This structure contains information about a single entry within a G-PAK archive, including its compressed and uncompressed sizes, the offset at which it is stored, and its CRC-32 checksum.
 */
struct gpak_entry_header
{
	uint32_t compressed_size_; /**< The compressed size of the entry in bytes. */
	uint32_t uncompressed_size_; /**< The uncompressed size of the entry in bytes. */
	size_t offset_; /**< The offset at which the entry is stored in the G-PAK archive. */
	uint32_t crc32_; /**< The CRC-32 checksum of the entry. */
};

/**
 * @brief Typedef for the gpak_entry_header structure.
 *
 * This typedef is used to create an alias for the gpak_entry_header structure, providing a more convenient way to use the structure in the code.
 */
typedef struct gpak_entry_header pak_entry_t;


/**
 * @brief Enumeration representing the error codes for G-PAK operations.
 *
 * This enumeration contains values representing the different error codes that can be encountered during G-PAK operations, such as issues with archive or stream handling, mode errors, and compression/decompression errors.
 */
enum gpak_error
{
	GPAK_ERROR_OK = 0,								/**< No error occurred. */
	GPAK_ERROR_ARCHIVE_IS_NULL = -1,				/**< The archive pointer is NULL. */
	GPAK_ERROR_STREAM_IS_NULL = -2,					/**< The stream pointer is NULL. */
	GPAK_ERROR_INCORRECT_MODE = -3,					/**< The mode flag is incorrect. */
	GPAK_ERROR_INCORRECT_HEADER = -4,				/**< The archive header is incorrect. */
	GPAK_ERROR_OPEN_FILE = -5,						/**< An error occurred while opening a file. */
	GPAK_ERROR_WRITE = -6,							/**< An error occurred while writing to a file. */
	GPAK_ERROR_READ = -7,							/**< An error occurred while reading from a file. */
	GPAK_ERROR_FILE_NOT_FOUND = -8,					/**< The specified file was not found. */

	// Deflate
	GPAK_ERROR_DEFLATE_INIT = -9,					/**< An error occurred while initializing deflate. */
	GPAK_ERROR_DEFLATE_FAILED = -10,				/**< Deflate operation failed. */
	GPAK_ERROR_INFLATE_INIT = -11,					/**< An error occurred while initializing inflate. */
	GPAK_ERROR_INFLATE_FAILED = -12,				/**< Inflate operation failed. */

	// LZ4 errors
	GPAK_ERROR_LZ4_WRITE_OPEN = -13,				/**< An error occurred while opening an LZ4 file for writing. */
	GPAK_ERROR_LZ4_WRITE = -14,						/**< An error occurred while writing to an LZ4 file. */
	GPAK_ERROR_LZ4_WRITE_CLOSE = -15,				/**< An error occurred while closing an LZ4 file after writing. */
	GPAK_ERROR_LZ4_READ_OPEN = -16,					/**< An error occurred while opening an LZ4 file for reading. */
	GPAK_ERROR_LZ4_READ = -17,						/**< An error occurred while reading from an LZ4 file. */
	GPAK_ERROR_LZ4_READ_CLOSE = -18,				/**< An error occurred while closing an LZ4 file after reading. */
	GPAK_ERROR_LZ4_DECOMPRESS = -19,				/**< An error occurred during LZ4 decompression. */

	// ZSTD
	GPAK_ERROR_EMPTY_INPUT = -20,					/**< The input is empty. */
	GPAK_ERROR_EOF_BEFORE_EOS = -21,				/**< End of file encountered before end of stream. */

	//crc
	GPAK_ERROR_FILE_CRC_NOT_MATCH = -22,			/**< The file CRC does not match. */

	// dictionary
	GPAK_ERROR_INVALID_DICTIONARY = -23,			/**< The dictionary is invalid. */
	GPAK_ERROR_FAILED_TO_CREATE_DICTIONARY = -24	/**< Failed to create a dictionary. */
};

/**
 * @brief Typedef for the gpak_error enumeration.
 *
 * This typedef is used to create an alias for the gpak_error enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_error gpak_error_t;

/**
 * @brief Enumeration representing the mode flags for G-PAK archive operations.
 *
 * This enumeration contains values representing the different mode flags that can be used to control the behavior of G-PAK archive operations, such as creating, reading, and updating archives.
 */
enum gpak_mode_flag
{
	GPAK_MODE_NONE = 0,					/**< No specific mode flag is set. */
	GPAK_MODE_CREATE = 1 << 0,			/**< Create mode: allows writing to the archive. Analogous to write byte operation. */
	GPAK_MODE_READ_ONLY = 1 << 1,		/**< Read-only mode: allows only reading from the archive. Analogous to read byte operation. */
	GPAK_MODE_UPDATE = 1 << 2			/**< Update mode: allows both reading and writing to the archive. */
};

/**
 * @brief Typedef for the gpak_mode_flag enumeration.
 *
 * This typedef is used to create an alias for the gpak_mode_flag enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_mode_flag gpak_mode_flag_t;

/**
 * @brief Enumeration representing the stages of the G-PAK archive operations.
 *
 * This enumeration contains values representing the different stages during which the G-PAK archive operations occur, such as compression and decompression.
 */
enum gpak_stage_flag
{
	GPAK_STAGE_COMPRESSION,  /**< The compression stage of a G-PAK operation. */
	GPAK_STAGE_DECOMPRESSION /**< The decompression stage of a G-PAK operation. */
};

/**
 * @brief Typedef for the gpak_stage_flag enumeration.
 *
 * This typedef is used to create an alias for the gpak_stage_flag enumeration, providing a more convenient way to use the enumeration in the code.
 */
typedef enum gpak_stage_flag gpak_stage_flag_t;

/**
 * @typedef gpak_error_handler_t
 * @brief A callback function for handling errors in G-PAK operations.
 */
typedef void (*gpak_error_handler_t)(const char*, int32_t, void*);

/**
 * @typedef gpak_progress_handler_t
 * @brief A callback function for handling progress in G-PAK operations.
 */
typedef void (*gpak_progress_handler_t)(const char*, size_t, size_t, int32_t, void*);


/**
 * @brief Structure representing a G-PAK archive.
 *
 * This structure contains information about a G-PAK archive, including its mode, header, file stream, filesystem tree root, error handling, progress handling, and user data. It also includes a dictionary and a pointer to the current file being processed.
 */
struct gpak
{
	int mode_; /**< The access mode of the G-PAK archive (read, write, update, etc.). */
	pak_header_t header_; /**< The header information of the G-PAK archive. */
	FILE* stream_; /**< The file stream associated with the G-PAK archive. */
	struct filesystem_tree_node* root_; /**< The root node of the filesystem tree representing the archive's directory structure. */
	int last_error_; /**< The last error code encountered during G-PAK operations. */
	char* dictionary_; /**< The compression dictionary for the G-PAK archive. */
	char* current_file_; /**< The current file being processed during G-PAK operations. */
	gpak_error_handler_t error_handler_; /**< The error handler function for G-PAK operations. */
	gpak_progress_handler_t progress_handler_; /**< The progress handler function for G-PAK operations. */
	void* user_data_; /**< User-defined data associated with the G-PAK archive. */
};

/**
 * @brief Typedef for the gpak structure.
 *
 * This typedef is used to create an alias for the gpak structure, providing a more convenient way to use the structure in the code.
 */
typedef struct gpak gpak_t;


/**
 * @brief Structure representing a file within a G-PAK archive.
 *
 * This structure contains information about a file in a G-PAK archive, including its data, file stream, and CRC32 checksum.
 */
struct gpak_file
{
	char* data_; /**< The data of the file in the G-PAK archive. */
	FILE* stream_; /**< The file stream associated with the file in the G-PAK archive. */
	uint32_t crc32_; /**< The CRC32 checksum of the file in the G-PAK archive. */
};

/**
 * @brief Typedef for the gpak_file structure.
 *
 * This typedef is used to create an alias for the gpak_file structure, providing a more convenient way to use the structure in the code.
 */
typedef struct gpak_file gpak_file_t;


/**
 * @brief Structure representing a file within a filesystem tree.
 *
 * This structure contains information about a file in a filesystem tree, including its name, path, and associated pak_entry_t data.
 */
struct filesystem_tree_file
{
	char* name_; /**< The name of the file in the filesystem tree. */
	char* path_; /**< The path of the file in the filesystem tree. */
	pak_entry_t entry_; /**< The pak_entry_t data associated with the file in the filesystem tree. */
};

/**
 * @brief Typedef for the filesystem_tree_file structure.
 *
 * This typedef is used to create an alias for the filesystem_tree_file structure, providing a more convenient way to use the structure in the code.
 */
typedef struct filesystem_tree_file filesystem_tree_file_t;


/**
 * @brief Structure representing a node within a filesystem tree.
 *
 * This structure contains information about a directory node in a filesystem tree, including its name, parent, children nodes, files, and counts of children and files.
 */
struct filesystem_tree_node
{
	char* name_; /**< The name of the directory node in the filesystem tree. */
	struct filesystem_tree_node* parent_; /**< The parent directory node of the current node in the filesystem tree. */
	struct filesystem_tree_node** children_; /**< An array of pointers to the children directory nodes of the current node in the filesystem tree. */
	size_t num_children_; /**< The number of children directory nodes in the current node of the filesystem tree. */
	filesystem_tree_file_t** files_; /**< An array of pointers to the files contained in the current directory node of the filesystem tree. */
	size_t num_files_; /**< The number of files contained in the current directory node of the filesystem tree. */
};

/**
 * @brief Typedef for the filesystem_tree_node structure.
 *
 * This typedef is used to create an alias for the filesystem_tree_node structure, providing a more convenient way to use the structure in the code.
 */
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

/**
 * @struct filesystem_tree_iterator_t
 * @brief A structure representing an iterator for the filesystem tree.
 */
typedef struct filesystem_tree_iterator filesystem_tree_iterator_t;

#endif // GPAK_DATA_H