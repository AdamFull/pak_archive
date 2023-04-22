#include "gpak_compressors.h"

#include "gpak_helper.h"
#include "filesystem_tree.h"

#include <stdlib.h>
#include <assert.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/sysinfo.h>
#endif

// Zlib
#include <zlib.h>

// LZ4
#include <lz4.h>
#include <lz4file.h>
#include <lz4hc.h>

// Z-standard
#include <zstd.h>
#include <zdict.h>

#define _DICTIONARY_SAMPLE_COUNT 200


int get_num_threads() 
{
	int numThreads;

#ifdef _WIN32
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	numThreads = sysinfo.dwNumberOfProcessors;
#else
	numThreads = get_nprocs();
#endif

	return numThreads;
}


uint32_t _gpak_compressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);
	size_t _readed = 0ull;
	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	fseek(_infile, 0, SEEK_END);
	size_t _total_size = ftell(_infile);
	fseek(_infile, 0, SEEK_SET);

	size_t bytes_readed = 0ull;
	do
	{
		_readed = _freadb(_bufferIn, 1ull, _DEFAULT_BLOCK_SIZE, _infile);
		_crc32 = crc32(_crc32, _bufferIn, _readed);
		bytes_readed += _readed;
		_gpak_pass_progress(_pak, bytes_readed, _total_size, GPAK_STAGE_COMPRESSION);
		_fwriteb(_bufferIn, 1ull, _readed, _outfile);
	} while (_readed);

	free(_bufferIn);

	return _crc32;
}

uint32_t _gpak_decompressor_none(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);
	size_t bytesReaded = 0ull;
	size_t nextBlockSize = _DEFAULT_BLOCK_SIZE;

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	do
	{
		size_t _readed = _freadb(_bufferIn, 1ull, nextBlockSize, _infile);
		_crc32 = crc32(_crc32, _bufferIn, _readed);

		_fwriteb(_bufferIn, 1ull, _readed, _outfile);
		bytesReaded += _readed;
		_gpak_pass_progress(_pak, bytesReaded, _read_size, GPAK_STAGE_DECOMPRESSION);

		if (_read_size - bytesReaded < nextBlockSize)
			nextBlockSize = _read_size - bytesReaded;
	} while (bytesReaded != _read_size);

	if (bytesReaded != _read_size)
		return _gpak_make_error(_pak, GPAK_ERROR_READ);

	free(_bufferIn);

	return _crc32;
}


uint32_t _gpak_compressor_deflate(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);
	char* _bufferOut = (char*)malloc(_DEFAULT_BLOCK_SIZE);

	fseek(_infile, 0, SEEK_END);
	size_t _total_size = ftell(_infile);
	fseek(_infile, 0, SEEK_SET);

	int ret = deflateInit(&strm, _pak->header_.compression_level_);
	if (ret != Z_OK)
		return _gpak_make_error(_pak, GPAK_ERROR_DEFLATE_INIT);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	int flush;
	unsigned have;
	size_t _total_readed = 0ull;
	do
	{
		strm.avail_in = _freadb(_bufferIn, 1ull, _DEFAULT_BLOCK_SIZE, _infile);
		_total_readed += strm.avail_in;
		_crc32 = crc32(_crc32, _bufferIn, strm.avail_in);
		_gpak_pass_progress(_pak, _total_readed, _total_size, GPAK_STAGE_COMPRESSION);

		if (ferror(_infile))
		{
			_gpak_make_error(_pak, GPAK_ERROR_READ);
			goto end;
		}

		flush = feof(_infile) ? Z_FINISH : Z_NO_FLUSH;
		strm.next_in = _bufferIn;

		do
		{
			strm.avail_out = _DEFAULT_BLOCK_SIZE;
			strm.next_out = _bufferOut;
			ret = deflate(&strm, flush);
			assert(ret != Z_STREAM_ERROR);

			have = _DEFAULT_BLOCK_SIZE - strm.avail_out;
			if (_fwriteb(_bufferOut, 1ull, have, _outfile) != have || ferror(_outfile))
			{
				_gpak_make_error(_pak, GPAK_ERROR_WRITE);
				goto end;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);

	} while (flush != Z_FINISH);

end:
	deflateEnd(&strm);
	free(_bufferIn);
	free(_bufferOut);

	return _crc32;
}

uint32_t _gpak_decompressor_inflate(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);
	char* _bufferOut = (char*)malloc(_DEFAULT_BLOCK_SIZE);

	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		return _gpak_make_error(_pak, GPAK_ERROR_INFLATE_INIT);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	unsigned have;
	size_t total_read = 0ull;

	do
	{
		size_t bytes_to_read = _read_size - total_read < _DEFAULT_BLOCK_SIZE ? _read_size - total_read : _DEFAULT_BLOCK_SIZE;
		strm.avail_in = _freadb(_bufferIn, 1, bytes_to_read, _infile);
		total_read += strm.avail_in;
		_gpak_pass_progress(_pak, total_read, _read_size, GPAK_STAGE_DECOMPRESSION);

		strm.next_in = _bufferIn;

		do
		{
			strm.avail_out = _DEFAULT_BLOCK_SIZE;
			strm.next_out = _bufferOut;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);

			switch (ret) {
			case Z_NEED_DICT:
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				goto end;
			}

			have = _DEFAULT_BLOCK_SIZE - strm.avail_out;

			_crc32 = crc32(_crc32, _bufferOut, have);

			if (_fwriteb(_bufferOut, 1, have, _outfile) != have || ferror(_outfile))
			{
				_gpak_make_error(_pak, GPAK_ERROR_INFLATE_FAILED);
				goto end;
			}

		} while (strm.avail_out == 0);

	} while (ret != Z_STREAM_END);

end:
	free(_bufferIn);
	free(_bufferOut);
	(void)inflateEnd(&strm);

	return _crc32;
}

// TODO: add dictionary
uint32_t _gpak_compressor_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	LZ4F_errorCode_t ret = LZ4F_OK_NoError;
	size_t len;
	LZ4_writeFile_t* lz4fWrite;
	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);

	fseek(_infile, 0, SEEK_END);
	size_t _total_size = ftell(_infile);
	fseek(_infile, 0, SEEK_SET);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	LZ4F_preferences_t _preferences;
	_preferences.frameInfo.blockSizeID = LZ4F_max4MB;
	_preferences.frameInfo.blockMode = LZ4F_blockLinked;
	_preferences.frameInfo.contentChecksumFlag = 0;
	_preferences.frameInfo.frameType = LZ4F_frame;
	_preferences.frameInfo.contentSize = 0;
	_preferences.frameInfo.dictID = 0;
	_preferences.frameInfo.blockChecksumFlag = 0;
	_preferences.compressionLevel = _pak->header_.compression_level_;
	_preferences.autoFlush = 1;
	_preferences.favorDecSpeed = 1;

	ret = LZ4F_writeOpen(&lz4fWrite, _outfile, &_preferences);
	if (LZ4F_isError(ret))
		return _gpak_make_error(_pak, GPAK_ERROR_LZ4_WRITE_OPEN);

	size_t _total_readed = 0ull;
	while (1)
	{
		len = _freadb(_bufferIn, 1, _DEFAULT_BLOCK_SIZE, _infile);
		_total_readed += len;
		_crc32 = crc32(_crc32, _bufferIn, len);
		_gpak_pass_progress(_pak, _total_readed, _total_size, GPAK_STAGE_COMPRESSION);

		if (ferror(_infile))
		{
			_gpak_make_error(_pak, GPAK_ERROR_READ);
			goto end;
		}

		if (len == 0)
			break;

		ret = LZ4F_write(lz4fWrite, _bufferIn, len);
		if (LZ4F_isError(ret))
		{
			_gpak_make_error(_pak, GPAK_ERROR_LZ4_WRITE);
			goto end;
		}
	}

end:
	free(_bufferIn);
	if (LZ4F_isError(LZ4F_writeClose(lz4fWrite)))
		return _gpak_make_error(_pak, GPAK_ERROR_LZ4_WRITE_CLOSE);
	return _crc32;
}

uint32_t _gpak_decompressor_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	LZ4F_errorCode_t ret = LZ4F_OK_NoError;
	LZ4_readFile_t* lz4fRead;
	char* _bufferIn = (char*)malloc(_DEFAULT_BLOCK_SIZE);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	ret = LZ4F_readOpen(&lz4fRead, _infile);
	if (LZ4F_isError(ret))
		return _gpak_make_error(_pak, GPAK_ERROR_LZ4_READ_OPEN);

	size_t bytesReaded = 0;
	size_t nextBlockSize = _DEFAULT_BLOCK_SIZE;

	while (bytesReaded < _read_size)
	{
		size_t bytes_to_read = (_read_size - bytesReaded) < _DEFAULT_BLOCK_SIZE ? (_read_size - bytesReaded) : _DEFAULT_BLOCK_SIZE;

		ret = LZ4F_read(lz4fRead, _bufferIn, _DEFAULT_BLOCK_SIZE);
		if (LZ4F_isError(ret))
		{
			_gpak_make_error(_pak, GPAK_ERROR_LZ4_READ);
			goto out;
		}

		if (ret == 0)
			break;

		bytesReaded += ret;
		_gpak_pass_progress(_pak, bytesReaded, _read_size, GPAK_STAGE_DECOMPRESSION);

		_crc32 = crc32(_crc32, _bufferIn, ret);

		if (_fwriteb(_bufferIn, 1, ret, _outfile) != ret)
		{
			_gpak_make_error(_pak, GPAK_ERROR_WRITE);
			goto out;
		}
	}

out:
	free(_bufferIn);
	if (LZ4F_isError(LZ4F_readClose(lz4fRead)))
		return _gpak_make_error(_pak, GPAK_ERROR_LZ4_READ_CLOSE);
	
	return _crc32;
}


uint32_t _gpak_compressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	size_t const buffInSize = ZSTD_CStreamInSize();
	void* const buffIn = malloc(buffInSize);
	size_t const buffOutSize = ZSTD_CStreamOutSize();
	void* const buffOut = malloc(buffOutSize);

	fseek(_infile, 0, SEEK_END);
	size_t _total_size = ftell(_infile);
	fseek(_infile, 0, SEEK_SET);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	/* Create the context. */
	ZSTD_CCtx* const cctx = ZSTD_createCCtx();

	uint32_t thread_count = get_num_threads();

	/* Set parameters. */
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, _pak->header_.compression_level_);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_enableLongDistanceMatching, 1);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_windowLog, 27);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, thread_count);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_jobSize, buffInSize * thread_count);
	
	if (_pak->dictionary_ && _pak->header_.dictionary_size_ > 0)
		ZSTD_CCtx_loadDictionary(cctx, _pak->dictionary_, _pak->header_.dictionary_size_);

	size_t _total_readed = 0ull;
	size_t const toRead = buffInSize;
	for (;;)
	{
		size_t read = _freadb(buffIn, 1, toRead, _infile);
		_total_readed += read;
		_crc32 = crc32(_crc32, buffIn, read);
		_gpak_pass_progress(_pak, _total_readed, _total_size, GPAK_STAGE_COMPRESSION);

		int const lastChunk = (read < toRead);
		ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;

		ZSTD_inBuffer input = { buffIn, read, 0 };
		int finished;
		do
		{
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
			size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
			//CHECK_ZSTD(remaining);
			_fwriteb(buffOut, 1, output.pos, _outfile);
			finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
		} while (!finished);
		assert(input.pos == input.size && "Impossible: zstd only returns 0 when the input is completely consumed!");

		if (lastChunk)
			break;
	}

	ZSTD_freeCCtx(cctx);
	free(buffIn);
	free(buffOut);

	return _crc32;
}

uint32_t _gpak_decompressor_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	size_t const buffInSize = ZSTD_DStreamInSize();
	void* const buffIn = malloc(buffInSize);
	size_t const buffOutSize = ZSTD_DStreamOutSize();
	void* const buffOut = malloc(buffOutSize);

	uint32_t _crc32 = crc32(0L, Z_NULL, 0);

	ZSTD_DCtx* const dctx = ZSTD_createDCtx();
	assert(dctx != NULL && "ZSTD_createDCtx() failed!");

	if (_pak->dictionary_ && _pak->header_.dictionary_size_ > 0)
		ZSTD_DCtx_loadDictionary(dctx, _pak->dictionary_, _pak->header_.dictionary_size_);

	size_t bytesRead = 0;
	size_t lastRet = 0;
	int isEmpty = 1;

	while (bytesRead < _read_size)
	{
		size_t const toRead = (bytesRead + buffInSize <= _read_size) ? buffInSize : (_read_size - bytesRead);
		size_t read = _freadb(buffIn, 1, toRead, _infile);

		if (!read)
		{
			break;
		}

		bytesRead += read;

		_gpak_pass_progress(_pak, bytesRead, _read_size, GPAK_STAGE_DECOMPRESSION);

		isEmpty = 0;
		ZSTD_inBuffer input = { buffIn, read, 0 };
		while (input.pos < input.size)
		{
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
			size_t const ret = ZSTD_decompressStream(dctx, &output, &input);

			_crc32 = crc32(_crc32, buffOut, output.pos);

			_fwriteb(buffOut, 1, output.pos, _outfile);
			lastRet = ret;
		}
	}

	if (isEmpty)
	{
		_gpak_make_error(_pak, GPAK_ERROR_EMPTY_INPUT);
		goto clear;
	}

	if (lastRet != 0)
	{
		_gpak_make_error(_pak, GPAK_ERROR_EOF_BEFORE_EOS);
		goto clear;
	}

clear:
	ZSTD_freeDCtx(dctx);
	free(buffIn);
	free(buffOut);

	return _crc32;
}

int32_t _gpak_compressor_generate_dictionary(gpak_t* _pak)
{
	char* samples = NULL;
	size_t sample_sizes[_DICTIONARY_SAMPLE_COUNT];
	size_t samples_count = 0ull;
	size_t samples_capacity = 0ull;
	size_t current_offset = 0ull;

	filesystem_tree_iterator_t* iterator = filesystem_iterator_create(_pak->root_);
	filesystem_tree_node_t* next_directory = _pak->root_;
	do
	{
		if (samples_count >= _DICTIONARY_SAMPLE_COUNT - 1)
			break;

		filesystem_tree_file_t* next_file = NULL;
		while ((next_file = filesystem_iterator_next_file(iterator)))
		{
			FILE* _infile = fopen(next_file->path_, "rb");

			fseek(_infile, 0, SEEK_END);
			sample_sizes[samples_count] = ftell(_infile);
			fseek(_infile, 0, SEEK_SET);

			samples_capacity += sample_sizes[samples_count];
			samples = (char*)realloc(samples, samples_capacity);

			_freadb(samples + current_offset, 1ull, sample_sizes[samples_count], _infile);

			current_offset += sample_sizes[samples_count];

			fclose(_infile);

			if (samples_count >= _DICTIONARY_SAMPLE_COUNT - 1)
				break;

			++samples_count;
		}
	} while ((next_directory = filesystem_iterator_next_directory(iterator)));

	filesystem_iterator_free(iterator);

	size_t average_file_size = 0ull;

	for (size_t idx = 0ull; idx < samples_count; ++idx)
		average_file_size += sample_sizes[idx] / samples_count;

	size_t nearest_pow_of_2 = 1;
	while (nearest_pow_of_2 < average_file_size)
		nearest_pow_of_2 *= 2;

	_pak->header_.dictionary_size_ = nearest_pow_of_2;

	_pak->dictionary_ = (char*)malloc(_pak->header_.dictionary_size_);
	_pak->header_.dictionary_size_ = ZDICT_trainFromBuffer(_pak->dictionary_, _pak->header_.dictionary_size_, samples, sample_sizes, samples_count);
	_pak->dictionary_ = (char*)realloc(_pak->dictionary_, _pak->header_.dictionary_size_);

	fseek(_pak->stream_, sizeof(pak_header_t), SEEK_SET);

	_fwriteb(_pak->dictionary_, 1ull, _pak->header_.dictionary_size_, _pak->stream_);

	free(samples);

	return _gpak_make_error(_pak, GPAK_ERROR_OK);
}