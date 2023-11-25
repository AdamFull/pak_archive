/**
 * @file gpak_compressors.h
 * @author AdamFull
 * @date 23.04.2023
 * @brief The gpak_compressors.h header file contains methods for compressing
 *        and decompressing files used in the Gpak library for managing game
 *        archives.
 *
 * This header file defines the compressor and decompressor functions for
 * various compression algorithms used in the Gpak library. These functions
 * are responsible for compressing and decompressing data stored within game
 * archive files, enabling efficient storage and retrieval of game assets.
 *
 * The Gpak library supports a variety of compression algorithms, including
 * (but not limited to): Deflate, LZ4, and Zstd. Additional compression
 * algorithms can be added by implementing the required compressor and
 * decompressor functions, and integrating them into the Gpak library.
 *
 * To use the compression and decompression functions provided by this header,
 * include it alongside the main gpak.h header file and link against the Gpak
 * library in your project.
 */

#ifndef GPAK_COMPRESSORS_H
#define GPAK_COMPRESSORS_H

#define _DEFAULT_BLOCK_SIZE 4 * 1024 * 1024

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	/**
	 * @brief Performs no compression on the input file.
	 *
	 * This function reads data from the input file and writes it directly to the output file without any compression.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_compressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile);

	/**
	 * @brief Performs no decompression on the input file.
	 *
	 * This function reads data from the input file and writes it directly to the output file without any decompression.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @param _read_size The number of bytes to read from the input file.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_decompressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	/**
	 * @brief Compresses the input file using the Deflate algorithm.
	 *
	 * This function compresses the input file using the Deflate algorithm and writes the compressed data to the output file.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_compressor_deflate(gpak_t* _pak, FILE* _infile, FILE* _outfile);

	/**
	 * @brief Decompresses the input file using the Inflate algorithm.
	 *
	 * This function decompresses the input file using the Inflate algorithm and writes the decompressed data to the output file.
	 *
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @param _read_size The number of bytes to read from the input file.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_decompressor_inflate(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	/**
	 * @brief Compresses the input file using the Zstandard (zstd) algorithm.
	 * 
	 * This function compresses the input file using the Zstandard (zstd) algorithm and writes the compressed data to the output file.
	 * 
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_compressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile);

	/**
	 * @brief Decompresses the input file using the Zstandard (zstd) algorithm.
	 * This function decompresses the input file using the Zstandard (zstd) algorithm and writes the decompressed data to the output file.
	 * @param _pak A pointer to the gpak_t.
	 * @param _infile A pointer to the input FILE.
	 * @param _outfile A pointer to the output FILE.
	 * @param _read_size The number of bytes to read from the input file.
	 * @return The number of bytes written to the output file.
	 */
	GPAK_API uint32_t _gpak_decompressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	/**
	 * @brief Generates a compression dictionary for the specified G-PAK archive.
	 * This function generates a compression dictionary for the specified G-PAK archive, which can be used to improve the compression ratio for certain algorithms.
	 * @param _pak A pointer to the gpak_t.
	 * @return A non-negative value if the dictionary generation is successful, or a negative value if an error occurred.
	 */
	GPAK_API int32_t _gpak_compressor_generate_dictionary(gpak_t* _pak);

#ifdef __cplusplus
}
#endif

#endif // GPAK_COMPRESSORS_H