/**
 * @file gpak.h
 * @author AdamFull
 * @date 23.04.2023
 * @brief Gpak (Game Pack) is a file archive library specifically designed for
 *        game archives, providing functionality for creating, modifying and
 *        extracting archive files.
 *
 * This library allows users to create, read and manipulate game archive files
 * using a variety of compression algorithms. It provides a simple API
 * for managing the contents of an archive and supports various
 * compression methods such as Deflate, LZ4, and ZSTD.
 *
 * The gpak.h header file contains the main API for the Gpak library,
 * including functions for creating and manipulating archives, reading
 * and writing files within archives, and managing the archive's
 * metadata.
 *
 * To use the Gpak library, simply include this header file and link
 * against the Gpak library in your project.
 */

#ifndef GPAK_LIB_H
#define GPAK_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	/**
	 * @brief Opens a G-PAK archive.
	 *
	 * This function opens a G-PAK archive at the specified _path and with the given _mode.
	 *
	 * @param _path A string containing the path to the G-PAK archive.
	 * @param _mode The mode in which to open the G-PAK archive.
	 * @return A pointer to the opened gpak_t or NULL if an error occurred.
	 */
	GPAK_API gpak_t* gpak_open(const char* _path, int _mode);

	/**
	 * @brief Closes a G-PAK archive.
	 *
	 * This function closes the specified G-PAK archive and deallocates any associated resources.
	 *
	 * @param _pak A pointer to the gpak_t to close.
	 * @return An integer value indicating success (0) or failure (-1).
	 */
	GPAK_API int gpak_close(gpak_t* _pak);

	/**
	 * @brief Sets user data for a G-PAK archive.
	 *
	 * This function associates user-defined data with the specified G-PAK archive.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _user_data A pointer to the user-defined data.
	 */
	GPAK_API void gpak_set_user_data(gpak_t* _pak, void* _user_data);

	/**
	 * @brief Sets an error handler for a G-PAK archive.
	 *
	 * This function sets a callback function to handle errors that occur during G-PAK operations.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _error_handler A callback function for handling errors.
	 */
	GPAK_API void gpak_set_error_handler(gpak_t* _pak, gpak_error_handler_t _error_handler);

	/**
	 * @brief Sets a progress handler for a G-PAK archive.
	 *
	 * This function sets a callback function to handle progress updates during G-PAK operations.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _progress_handler A callback function for handling progress updates.
	 */
	GPAK_API void gpak_set_process_handler(gpak_t* _pak, gpak_progress_handler_t _progress_handler);

	/**
	 * @brief Sets the compression algorithm for a G-PAK archive.
	 *
	 * This function sets the compression algorithm to use when adding files to the G-PAK archive.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _algorithm An integer value representing the compression algorithm.
	 */
	GPAK_API void gpak_set_compression_algorithm(gpak_t* _pak, int _algorithm);

	/**
	 * @brief Sets the compression level for a G-PAK archive.
	 *
	 * This function sets the compression level to use when adding files to the G-PAK archive.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _level An integer value representing the compression level.
	 */
	GPAK_API void gpak_set_compression_level(gpak_t* _pak, int _level);

	/**
	 * @brief Adds a directory to a G-PAK archive.
	 *
	 * This function adds a new directory to the G-PAK archive with the specified _internal_path.
	 *
	 * @param _pak A pointer to the gpak_t
	 * @param _internal_path A string containing the internal path of the directory to add.
	 * @return An integer value indicating success (0) or failure (-1).
	 */
	GPAK_API int gpak_add_directory(gpak_t* _pak, const char* _internal_path);

	/**
	 * @brief Adds a file to a G-PAK archive.
	 * 
	 * This function adds a new file to the G-PAK archive with the specified _external_path and _internal_path.
	 * 
	 * @param _pak A pointer to the gpak_t.
	 * @param _external_path A string containing the external path of the file to add.
	 * @param _internal_path A string containing the internal path of the file within the G-PAK archive.
	 * @return An integer value indicating success (0) or failure (-1).
	 */
	GPAK_API int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path);

	/**
	 * @brief Retrieves the root directory node of a G-PAK archive.
	 * 
	 * This function retrieves the root directory node of the G-PAK archive.
	 * 
	 * @param _pak A pointer to the gpak_t.
	 * @return A pointer to the root filesystem_tree_node.
	 */
	GPAK_API struct filesystem_tree_node* gpak_get_root(gpak_t* _pak);

	/**
	 * @brief Finds a directory in a G-PAK archive.
	 * 
	 * This function searches for a directory with the specified _path in the G-PAK archive.
	 * 
	 * @param _pak A pointer to the gpak_t.
	 * @param _path A string containing the path of the directory to find.
	 * @return A pointer to the found directory node or NULL if not found.
	 */
	GPAK_API struct filesystem_tree_node* gpak_find_directory(gpak_t* _pak, const char* _path);

	/**
	 * @brief Finds a file in a G-PAK archive.
	 * 
	 * This function searches for a file with the specified _path in the G-PAK archive.
	 * 
	 * @param _pak A pointer to the gpak_t.
	 * @param _path A string containing the path of the file to find.
	 * @return A pointer to the found file node or NULL if not found.
	 */
	GPAK_API struct filesystem_tree_file* gpak_find_file(gpak_t* _pak, const char* _path);


	/**
	 * @brief Opens a file in a G-PAK archive.
	 *
	 * This function opens a file with the specified _path in the G-PAK archive.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _path A string containing the path of the file to open.
	 * @return A pointer to the opened gpak_file_t or NULL if an error occurred.
	 */
	GPAK_API gpak_file_t* gpak_fopen(gpak_t* _pak, const char* _path);

	/**
	 * @brief Reads the next character from a G-PAK file.
	 *
	 * This function reads the next character from the specified G-PAK file.
	 *
	 * @param _file A pointer to the gpak_file_t.
	 * @return The next character as an integer or EOF if the end of the file is reached.
	 */
	GPAK_API int gpak_fgetc(gpak_file_t* _file);

	/**
	 * @brief Reads a line from a G-PAK file.
	 *
	 * This function reads a line from the specified G-PAK file and stores it in the provided _buffer.
	 *
	 * @param _file A pointer to the gpak_file_t.
	 * @param _buffer A pointer to the buffer where the line will be stored.
	 * @param _max The maximum number of characters to read (including the null-terminator).
	 * @return A pointer to the _buffer or NULL if the end of the file is reached.
	 */
	GPAK_API char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max);

	/**
	 * @brief Unreads a character from a G-PAK file.
	 *
	 * This function pushes back the specified _character into the G-PAK file's read buffer.
	 *
	 * @param _file A pointer to the gpak_file_t.
	 * @param _character The character to unget.
	 * @return The character pushed back or EOF if an error occurred.
	 */
	GPAK_API int gpak_ungetc(gpak_file_t* _file, int _character);

	/**
	 * @brief Reads data from a G-PAK file.
	 *
	 * This function reads _elemCount elements of size _elemSize from the specified G-PAK file into the provided _buffer.
	 *
	 * @param _buffer A pointer to the buffer where the data will be stored.
	 * @param _elemSize The size of each element to read.
	 * @param _elemCount The number of elements to read.
	 * @param _file A pointer to the gpak_file_t.
	 * @return The number of elements successfully read.
	 */
	GPAK_API size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file);

	/**
	 * @brief Returns the current position in a G-PAK file.
	 *
	 * This function returns the current position in the specified G-PAK file.
	 *
	 * @param _file A pointer to the gpak_file_t.
	 * @return The current position in the file or -1 if an error occurred.
	 */
	GPAK_API long gpak_ftell(gpak_file_t* _file);

	/**
	 * @brief Sets the file position in a G-PAK file.
	 *
	 * This function sets the file position in the specified G-PAK file according to the _offset and _origin.
	 *
	 * @param _file A pointer to the gpak_file_t.
	 * @param _offset The offset in bytes from
	 * the _origin to set the new file position.
	 * @param _origin The reference position in the file (SEEK_SET, SEEK_CUR, or SEEK_END).
	 * @return Zero if the operation is successful, or -1 if an error occurred.
	 */
	GPAK_API long gpak_fseek(gpak_file_t* _file, long _offset, int _origin);

	/**
	 * @brief Checks if the end of a G-PAK file has been reached.
	 * 
	 * This function returns a non-zero value if the end of the specified G-PAK file has been reached.
	 * 
	 * @param _file A pointer to the gpak_file_t.
	 * @return A non-zero value if the end of the file is reached, or zero otherwise.
	 */
	GPAK_API long gpak_feof(gpak_file_t* _file);

	/**
	 * @brief Closes a G-PAK file.
	 * This function closes the specified G-PAK file and deallocates any associated resources.
	 * @param _file A pointer to the gpak_file_t to close.
	 */
	GPAK_API void gpak_fclose(gpak_file_t* _file);

#ifdef __cplusplus
}
#endif

#endif // GPAK_LIB_H