#ifndef GPAK_COMPRESSORS_H
#define GPAK_COMPRESSORS_H

#define _DEFAULT_BLOCK_SIZE 4 * 1024 * 1024

#ifdef __cplusplus
extern "C" {
#endif

#include "gpak_export.h"
#include "gpak_data.h"

	GPAK_API uint32_t _gpak_compressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile);
	GPAK_API uint32_t _gpak_decompressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	GPAK_API uint32_t _gpak_compressor_deflate(gpak_t* _pak, FILE* _infile, FILE* _outfile);
	GPAK_API uint32_t _gpak_decompressor_inflate(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	GPAK_API uint32_t _gpak_compressor_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile);
	GPAK_API uint32_t _gpak_decompressor_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	GPAK_API uint32_t _gpak_compressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile);
	GPAK_API uint32_t _gpak_decompressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size);

	GPAK_API int32_t _gpak_compressor_generate_dictionary(gpak_t* _pak);

#ifdef __cplusplus
}
#endif

#endif // GPAK_COMPRESSORS_H