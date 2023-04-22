#ifndef GPAK_HELPER_H
#define GPAK_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	/**
	 * @brief Sets the error code for the specified G-PAK archive.
	 *
	 * This function sets the error code for the specified G-PAK archive and calls the error handler, if one has been set.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _error_code The error code to set.
	 * @return The error code that was set.
	 */
	GPAK_API int _gpak_make_error(gpak_t* _pak, int _error_code);

	/**
	 * @brief Notifies the progress handler for the specified G-PAK archive.
	 *
	 * This function notifies the progress handler for the specified G-PAK archive, if one has been set, with the current progress information.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _done The number of bytes processed.
	 * @param _total The total number of bytes to be processed.
	 * @param _stage The current stage of the operation (e.g., compression, decompression).
	 */
	GPAK_API void _gpak_pass_progress(gpak_t* _pak, size_t _done, size_t _total, int32_t _stage);

	/**
	 * @brief Writes binary data to a file.
	 *
	 * This function writes binary data from the specified buffer to the given file. It is an internal utility function used by the G-PAK library.
	 *
	 * @param _data A pointer to the buffer containing the data to write.
	 * @param _elemSize The size of each element in bytes.
	 * @param _elemCount The number of elements to write.
	 * @param _file A pointer to the FILE to write to.
	 * @return The number of elements successfully written to the file.
	 */
	GPAK_API size_t _fwriteb(const void* _data, size_t _elemSize, size_t _elemCount, FILE* _file);

	/**
	 * @brief Reads binary data from a file.
	 *
	 * This function reads binary data from the specified file into the given buffer. It is an internal utility function used by the G-PAK library.
	 *
	 * @param _data A pointer to the buffer to store the read data.
	 * @param _elemSize The size of each element in bytes.
	 * @param _elemCount The number of elements to read.
	 * @param _file A pointer to the FILE to read from.
	 * @return The number of elements successfully read from the file.
	 */
	GPAK_API size_t _freadb(void* _data, size_t _elemSize, size_t _elemCount, FILE* _file);

#ifdef __cplusplus
}
#endif

#endif // GPAK_HELPER_H