#include "gpak.h"

#include "filesystem_tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include "libfmemopen.h"
#endif

// Openssl
#include <openssl/aes.h>
#include <openssl/rand.h>

// Zlib
#include <zlib.h>

// LZ4
#include <lz4.h>
#include <lz4file.h>

// Z-standard
#include <zstd.h>

#define _DEFAULT_BLOCK_SIZE 128 * 1024

int _pak_make_error(gpak_t* _pak, int _error_code)
{
	_pak->last_error_ = _error_code;

	if (_pak->error_handler_ && _error_code != GPAK_ERROR_OK)
	{
		(*_pak->error_handler_)(_error_code, "");
	}
	
	return _error_code;
}

//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//---------------------------------HELPERS--------------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
size_t fwriteb(const void* _data, size_t _elemSize, size_t _elemCount, FILE* _file)
{
	return fwrite(_data, _elemSize, _elemCount, _file) * _elemSize;
}

size_t freadb(void* _data, size_t _elemSize, size_t _elemCount, FILE* _file)
{
	return fread(_data, _elemSize, _elemCount, _file) * _elemSize;
}

pak_header_t _pak_make_header()
{
	pak_header_t _header;

	strcpy(_header.format_, "gpak\0");
	_header.compression_ = GPAK_HEADER_COMPRESSION_NONE;
	_header.encryption_ = GPAK_HEADER_ENCRYPTION_NONE;
	_header.compression_level_ = 0;
	_header.entry_count_ = 0ull;

	return _header;
}

int _pak_validate_header(gpak_t* _pak)
{
	if (strcmp(_pak->header_.format_, "gpak") != 0)
		return _pak_make_error(_pak, GPAK_ERROR_INCORRECT_HEADER);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

long _write_entry_header(gpak_t* _pak, const char* _path, pak_entry_t* _entry)
{
	long bytes_writen = 0u;

	// Write filename
	short name_size = strlen(_path);
	bytes_writen += fwriteb(&name_size, sizeof(short), 1ull, _pak->stream_);
	bytes_writen += fwriteb(_path, 1ull, name_size, _pak->stream_);
	bytes_writen += fwriteb(_entry, sizeof(pak_entry_t), 1ull, _pak->stream_);

	return bytes_writen;
}

long _read_entry_header(gpak_t* _pak, char* _path, pak_entry_t* _entry)
{
	long bytes_readed = 0u;

	short name_size;
	bytes_readed += freadb(&name_size, sizeof(short), 1ull, _pak->stream_);
	bytes_readed += freadb(_path, 1ull, name_size, _pak->stream_);
	bytes_readed += freadb(_entry, sizeof(pak_entry_t), 1ull, _pak->stream_);
	_path[name_size] = '\0';

	return bytes_readed;
}

int _update_entry_header(gpak_t* _pak, size_t _commressed_size, size_t _uncompressed_size, long _entry_size)
{
	if (!_pak->stream_ )
		return _pak_make_error(_pak, GPAK_ERROR_STREAM_IS_NULL);

	size_t _shift = 0ull;
	if (_uncompressed_size == _commressed_size)
		_shift = _uncompressed_size;
	else
		_shift = _commressed_size;

	long _cur_pos = ftell(_pak->stream_);
	long _header_start_pos = (long)_shift + _entry_size;

	fseek(_pak->stream_, -_header_start_pos, SEEK_CUR);

	char _entry_path[256];
	pak_entry_t _entry;
	_read_entry_header(_pak, _entry_path, &_entry);

	_entry.compressed_size_ = _commressed_size;
	_entry.uncompressed_size_ = _uncompressed_size;

	fseek(_pak->stream_, -_entry_size, SEEK_CUR);

	_write_entry_header(_pak, _entry_path, &_entry);

	fseek(_pak->stream_, 0l, SEEK_END);

	if (_cur_pos != ftell(_pak->stream_))
		return _pak_make_error(_pak, GPAK_ERROR_WRITE);

	return GPAK_ERROR_OK;
}

int _update_pak_header(gpak_t* _pak)
{
	fseek(_pak->stream_, 0, SEEK_SET);

	size_t readed = fwriteb(&_pak->header_, sizeof(pak_header_t), 1ull, _pak->stream_);
	if (readed != sizeof(pak_header_t))
	{
		fseek(_pak->stream_, 0l, SEEK_END);
		return _pak_make_error(_pak, GPAK_ERROR_WRITE);
	}

	return GPAK_ERROR_OK;
}


//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-------------------------------ENCRYPTION-------------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
int _gpak_encrypt_stream_aes(gpak_t* _pak, FILE* _file)
{
	size_t _key_len = strlen(_pak->password_);
	char* iv = (char*)malloc(_key_len);
	RAND_bytes(iv, _key_len);

	AES_KEY aesKey;
	AES_set_encrypt_key(_pak->password_, _key_len * 8, &aesKey);

	char _buffer[_DEFAULT_BLOCK_SIZE];
	char _bufferout[_DEFAULT_BLOCK_SIZE];
	size_t _readed = 0ull;

	do
	{
		_readed = freadb(_buffer, 1ull, sizeof(_buffer), _file);

		if (_readed < sizeof(_buffer))
		{
			int padding = sizeof(_buffer) - _readed;
			memset(_buffer + _readed, padding, padding);
		}

		AES_cbc_encrypt(_buffer, _bufferout, sizeof(_buffer), &aesKey, iv, AES_ENCRYPT);

		fwriteb(_bufferout, 1ull, _readed, _pak->stream_);
	} while (_readed);

	free(iv);

	return GPAK_ERROR_OK;
}

//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-------------------------------COMPRESSION------------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
int _gpak_compress_stream_none(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	char _buffer[_DEFAULT_BLOCK_SIZE];
	size_t _readed = 0ull;

	do
	{
		_readed = freadb(_buffer, 1ull, sizeof(_buffer), _infile);
		fwriteb(_buffer, 1ull, _readed, _outfile);
	} while (_readed);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_compress_stream_deflate(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	char _bufferIn[_DEFAULT_BLOCK_SIZE];
	char _bufferOut[_DEFAULT_BLOCK_SIZE];

	int ret = deflateInit(&strm, _pak->header_.compression_level_);
	if(ret != Z_OK)
		return _pak_make_error(_pak, GPAK_ERROR_DEFLATE_INIT);

	int flush;
	unsigned have;
	do 
	{
		strm.avail_in = freadb(_bufferIn, 1ull, _DEFAULT_BLOCK_SIZE, _infile);
		if (ferror(_infile)) 
		{
			ret = _pak_make_error(_pak, GPAK_ERROR_READ);
			(void)deflateEnd(&strm);
			return ret;
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
			if (fwriteb(_bufferOut, 1ull, have, _outfile) != have || ferror(_outfile)) 
			{
				ret = _pak_make_error(_pak, GPAK_ERROR_WRITE);
				(void)deflateEnd(&strm);
				return ret;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);

	} while (flush != Z_FINISH);

	deflateEnd(&strm);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_compress_stream_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	LZ4F_errorCode_t ret = LZ4F_OK_NoError;
	size_t len;
	LZ4_writeFile_t* lz4fWrite;
	char buffer[_DEFAULT_BLOCK_SIZE];

	LZ4F_preferences_t _preferences;
	_preferences.frameInfo.blockSizeID = LZ4F_max4MB;
	_preferences.frameInfo.blockMode = LZ4F_blockLinked;
	_preferences.frameInfo.contentChecksumFlag = 0;
	_preferences.frameInfo.frameType = LZ4F_frame;
	_preferences.frameInfo.contentSize = 0;
	_preferences.frameInfo.dictID = 1;
	_preferences.frameInfo.blockChecksumFlag = 0;
	_preferences.compressionLevel = _pak->header_.compression_level_;
	_preferences.autoFlush = 1;
	_preferences.favorDecSpeed = 1;

	ret = LZ4F_writeOpen(&lz4fWrite, _outfile, &_preferences);
	if (LZ4F_isError(ret)) 
		return _pak_make_error(_pak, GPAK_ERROR_LZ4_WRITE_OPEN);

	while (1) 
	{
		len = freadb(buffer, 1, _DEFAULT_BLOCK_SIZE, _infile);

		if (ferror(_infile)) 
		{
			_pak_make_error(_pak, GPAK_ERROR_READ);
			goto out;
		}

		if (len == 0)
			break;

		ret = LZ4F_write(lz4fWrite, buffer, len);
		if (LZ4F_isError(ret)) 
		{
			_pak_make_error(_pak, GPAK_ERROR_LZ4_WRITE);
			goto out;
		}
	}

out:
	if (LZ4F_isError(LZ4F_writeClose(lz4fWrite))) 
		return _pak_make_error(_pak, GPAK_ERROR_LZ4_WRITE_CLOSE);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_compress_stream_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	size_t const buffInSize = ZSTD_CStreamInSize();
	void* const buffIn = malloc(buffInSize);
	size_t const buffOutSize = ZSTD_CStreamOutSize();
	void* const buffOut = malloc(buffOutSize);

	/* Create the context. */
	ZSTD_CCtx* const cctx = ZSTD_createCCtx();

	/* Set parameters. */
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_compressionLevel, _pak->header_.compression_level_);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_checksumFlag, 1);
	ZSTD_CCtx_setParameter(cctx, ZSTD_c_nbWorkers, 1);

	size_t const toRead = buffInSize;
	for (;;) 
	{
		size_t read = freadb(buffIn, 1, toRead, _infile);
		int const lastChunk = (read < toRead);
		ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;

		ZSTD_inBuffer input = { buffIn, read, 0 };
		int finished;
		do 
		{
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
			size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
			//CHECK_ZSTD(remaining);
			fwriteb(buffOut, 1, output.pos, _outfile);
			finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
		} while (!finished);
		assert(input.pos == input.size && "Impossible: zstd only returns 0 when the input is completely consumed!");

		if (lastChunk)
			break;
	}

	ZSTD_freeCCtx(cctx);
	free(buffIn);
	free(buffOut);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}


int _gpak_decompress_stream_none(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	char _buffer[_DEFAULT_BLOCK_SIZE];
	size_t bytesReaded = 0ull;
	size_t nextBlockSize = sizeof(_buffer);
	do
	{
		size_t _readed = freadb(_buffer, 1ull, nextBlockSize, _infile);
		fwriteb(_buffer, 1ull, _readed, _outfile);
		bytesReaded += _readed;

		if (_read_size - bytesReaded < nextBlockSize)
			nextBlockSize = _read_size - bytesReaded;
	} while (bytesReaded != _read_size);

	if (bytesReaded != _read_size)
		return _pak_make_error(_pak, GPAK_ERROR_READ);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_decompress_stream_inflate(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	z_stream strm;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;

	char _bufferIn[_DEFAULT_BLOCK_SIZE];
	char _bufferOut[_DEFAULT_BLOCK_SIZE];

	int ret = inflateInit(&strm);
	if (ret != Z_OK)
		return _pak_make_error(_pak, GPAK_ERROR_INFLATE_INIT);

	unsigned have;
	size_t total_read = 0ull;
	do 
	{
		size_t bytes_to_read = _read_size - total_read < _DEFAULT_BLOCK_SIZE ? _read_size - total_read : _DEFAULT_BLOCK_SIZE;
		strm.avail_in = freadb(_bufferIn, 1, bytes_to_read, _infile);
		total_read += strm.avail_in;
		if (ferror(_infile))
		{
			(void)inflateEnd(&strm);
			return _pak_make_error(_pak, GPAK_ERROR_READ);
		}

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
				(void)inflateEnd(&strm);
				return ret;
			}

			have = _DEFAULT_BLOCK_SIZE - strm.avail_out;

			if (fwriteb(_bufferOut, 1, have, _outfile) != have || ferror(_outfile)) {

				(void)inflateEnd(&strm);
				return _pak_make_error(_pak, GPAK_ERROR_INFLATE_FAILED);
			}

		} while (strm.avail_out == 0);

	} while (ret != Z_STREAM_END);

	(void)inflateEnd(&strm);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_decompress_stream_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	LZ4F_errorCode_t ret = LZ4F_OK_NoError;
	LZ4_readFile_t* lz4fRead;
	char buffer[_DEFAULT_BLOCK_SIZE];

	ret = LZ4F_readOpen(&lz4fRead, _infile);
	if (LZ4F_isError(ret)) 
		return _pak_make_error(_pak, GPAK_ERROR_LZ4_READ_OPEN);

	size_t bytesReaded = 0;
	size_t nextBlockSize = _DEFAULT_BLOCK_SIZE;

	while (bytesReaded < _read_size)
	{
		size_t bytes_to_read = (_read_size - bytesReaded) < _DEFAULT_BLOCK_SIZE ? (_read_size - bytesReaded) : _DEFAULT_BLOCK_SIZE;

		ret = LZ4F_read(lz4fRead, buffer, _DEFAULT_BLOCK_SIZE);
		if (LZ4F_isError(ret)) 
		{
			_pak_make_error(_pak, GPAK_ERROR_LZ4_READ);
			goto out;
		}

		if (ret == 0)
			break;

		bytesReaded += ret;

		if (fwriteb(buffer, 1, ret, _outfile) != ret) 
		{
			_pak_make_error(_pak, GPAK_ERROR_WRITE);
			goto out;
		}
	}

out:
	if (LZ4F_isError(LZ4F_readClose(lz4fRead))) 
		return _pak_make_error(_pak, GPAK_ERROR_LZ4_READ_CLOSE);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_decompress_stream_zstd(gpak_t* _pak, FILE* _infile, FILE* _outfile, size_t _read_size)
{
	size_t const buffInSize = ZSTD_DStreamInSize();
	void* const buffIn = malloc(buffInSize);
	size_t const buffOutSize = ZSTD_DStreamOutSize();
	void* const buffOut = malloc(buffOutSize);

	ZSTD_DCtx* const dctx = ZSTD_createDCtx();
	assert(dctx != NULL && "ZSTD_createDCtx() failed!");

	size_t bytesRead = 0;
	size_t lastRet = 0;
	int isEmpty = 1;

	while (bytesRead < _read_size)
	{
		size_t const toRead = (bytesRead + buffInSize <= _read_size) ? buffInSize : (_read_size - bytesRead);
		size_t read = freadb(buffIn, 1, toRead, _infile);

		if (!read)
		{
			break;
		}

		bytesRead += read;
		isEmpty = 0;
		ZSTD_inBuffer input = { buffIn, read, 0 };
		while (input.pos < input.size)
		{
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
			size_t const ret = ZSTD_decompressStream(dctx, &output, &input);

			fwriteb(buffOut, 1, output.pos, _outfile);
			lastRet = ret;
		}
	}

	if (isEmpty)
	{
		_pak_make_error(_pak, GPAK_ERROR_EMPTY_INPUT);
		goto clear;
	}

	if (lastRet != 0)
	{
		_pak_make_error(_pak, GPAK_ERROR_EOF_BEFORE_EOS);
		goto clear;
	}

clear:
	ZSTD_freeDCtx(dctx);
	free(buffIn);
	free(buffOut);

	return _pak->last_error_;
}


//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//--------------------------------FILE TREE-------------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
int _gpak_archivate_file_tree(gpak_t* _pak)
{
	filesystem_tree_iterator_t* iterator = filesystem_iterator_create(_pak->root_);
	filesystem_tree_node_t* next_directory = _pak->root_;
	do 
	{
		filesystem_tree_file_t* next_file = NULL;
		while ((next_file = filesystem_iterator_next_file(iterator)))
		{
			// Write file header first
			char* internal_filepath = filesystem_tree_file_path(next_directory, next_file);
			pak_entry_t _entry;
			long header_size = _write_entry_header(_pak, internal_filepath, &_entry);
			++_pak->header_.entry_count_;
			free(internal_filepath);

			FILE* _infile = fopen(next_file->path_, "rb");
			FILE* _outfile = NULL;
			if (!_infile)
				return _pak_make_error(_pak, GPAK_ERROR_OPEN_FILE);

			//bool _encrypt = (_pak->header_.encryption_ & GPAK_HEADER_ENCRYPTION_AES128) || (_pak->header_.encryption_ & GPAK_HEADER_ENCRYPTION_AES256);
			//
			//if(_encrypt)
			//{
			//	int _namesize = sprintf(NULL, "%s%s", next_file->name_, ".tmp");
			//	//char* _tempfilename = 
			//}

			long cursor = ftell(_pak->stream_);

			if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
				_gpak_compress_stream_deflate(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
				_gpak_compress_stream_lz4(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
				_gpak_compress_stream_zstd(_pak, _infile, _pak->stream_);
			else
				_gpak_compress_stream_none(_pak, _infile, _pak->stream_);

			long position = ftell(_pak->stream_);
			size_t uncompressed_size = ftell(_infile);
			size_t compressed_size = position - cursor;

			fflush(_pak->stream_);

			_update_entry_header(_pak, compressed_size, uncompressed_size, header_size);

			fclose(_infile);
		}
	} while ((next_directory = filesystem_iterator_next_directory(iterator)));
	
	filesystem_iterator_free(iterator);

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_parse_file_tree(gpak_t* _pak)
{
	while (1)
	{
		char _filename[256];
		pak_entry_t _entry;
		size_t readed = _read_entry_header(_pak, _filename, &_entry);
		if (readed == 0)
			break;	// EOF

		filesystem_tree_add_file(_pak->root_, _filename, NULL, ftell(_pak->stream_), _entry.compressed_size_, _entry.uncompressed_size_);

		fseek(_pak->stream_, _entry.compressed_size_, SEEK_CUR);
	}

	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-----------------------------IMPLEMENTATION-----------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
gpak_t* gpak_open(const char* _path, int _mode)
{
	gpak_t* pak;
	pak = (gpak_t*)calloc(1, sizeof(gpak_t));
	pak->mode_ = _mode;
	pak->error_handler_ = NULL;
	pak->progress_handler_ = NULL;

	const char* open_mode_;
	if (_mode & GPAK_MODE_CREATE)
		open_mode_ = "wb+";
	else if (_mode & GPAK_MODE_READ_ONLY)
		open_mode_ = "rb+";
	else
	{
		gpak_close(pak);
		return NULL;
	}
	
	// Trying to open pak file
	pak->stream_ = fopen(_path, open_mode_);

	if (pak->stream_ == NULL)
	{
		gpak_close(pak);
		return NULL;
	}

	pak->root_ = filesystem_tree_create();

	if (_mode & GPAK_MODE_CREATE)
	{
		pak->header_ = _pak_make_header();
		size_t res = fwriteb(&pak->header_, sizeof(pak_header_t), 1ull, pak->stream_);
		if (res != sizeof(pak_header_t))
		{
			gpak_close(pak);
			return NULL;
		}
	}
	else if (_mode & GPAK_MODE_READ_ONLY)
	{
		size_t res = freadb(&pak->header_, sizeof(pak_header_t), 1ull, pak->stream_);
		if (_pak_validate_header(pak) != GPAK_ERROR_OK || res != sizeof(pak_header_t))
		{
			gpak_close(pak);
			return NULL;
		}

		_gpak_parse_file_tree(pak);
		// Parse files
	}

	return pak;
}

int gpak_close(gpak_t* _pak)
{
	if (_pak != NULL)
	{
		if (_pak->stream_ != NULL)
		{
			// Recalculate header

			if((_pak->mode_ & GPAK_MODE_CREATE) || (_pak->mode_ & GPAK_MODE_UPDATE))
				_gpak_archivate_file_tree(_pak);

			filesystem_tree_delete(_pak->root_);

			_update_pak_header(_pak);
			fclose(_pak->stream_);
		}
		
		free(_pak->password_);
		free(_pak);
		return _pak_make_error(_pak, GPAK_ERROR_OK);
	}
	
	return -1;
}

void gpak_set_compression_algorithm(gpak_t* _pak, int _algorithm)
{
	_pak->header_.compression_ = _algorithm;
}

void gpak_set_compression_level(gpak_t* _pak, int _level)
{
	_pak->header_.compression_level_ = _level;
}

void gpak_set_encryption_mode(gpak_t* _pak, int _mode)
{
	_pak->header_.encryption_ = _mode;
}

void gpak_set_encryption_password(gpak_t* _pak, const char* _password)
{
	_pak->password_ = strdup(_password);
}

int gpak_add_directory(gpak_t* _pak, const char* _internal_path)
{
	filesystem_tree_add_directory(_pak->root_, _internal_path);
	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path)
{
	filesystem_tree_add_file(_pak->root_, _internal_path, _external_path, 0ull, 0ull, 0ull);
	return _pak_make_error(_pak, GPAK_ERROR_OK);
}

struct filesystem_tree_node* gpak_get_root(gpak_t* _pak)
{
	return _pak->root_;
}

filesystem_tree_node_t* gpak_find_directory(gpak_t* _pak, const char* _path)
{
	if (_pak == NULL)
		return NULL;

	return filesystem_tree_find_directory(_pak->root_, _path);
}

filesystem_tree_file_t* gpak_find_file(gpak_t* _pak, const char* _path)
{
	if (_pak == NULL)
		return NULL;

	return filesystem_tree_find_file(_pak->root_, _path);
}

gpak_file_t* gpak_fopen(gpak_t* _pak, const char* _path)
{
	filesystem_tree_file_t* _file_info = filesystem_tree_find_file(_pak->root_, _path);
	if (!_file_info)
	{
		_pak_make_error(_pak, GPAK_ERROR_FILE_NOT_FOUND);
		return NULL;
	}

	gpak_file_t* mfile = (gpak_file_t*)malloc(sizeof(gpak_file_t));
	mfile->data_ = (char*)malloc(_file_info->usize_ + 1);
	mfile->data_[_file_info->usize_] = '\0';

	mfile->stream_ = fmemopen(mfile->data_, _file_info->usize_, "wb+");

	// Setting position to file start
	fseek(_pak->stream_, _file_info->offset_, SEEK_SET);

	int res;
	if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
		res = _gpak_decompress_stream_inflate(_pak, _pak->stream_, mfile->stream_, _file_info->size_);
	else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
		res = _gpak_decompress_stream_lz4(_pak, _pak->stream_, mfile->stream_, _file_info->size_);
	else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
		res = _gpak_decompress_stream_zstd(_pak, _pak->stream_, mfile->stream_, _file_info->size_);
	else
		res = _gpak_decompress_stream_none(_pak, _pak->stream_, mfile->stream_, _file_info->size_);

	fseek(mfile->stream_, 0, SEEK_SET);

	if (res != GPAK_ERROR_OK)
	{
		gpak_fclose(mfile);
		return NULL;
	}

	return mfile;
}

int gpak_fgetc(gpak_file_t* _file)
{
	return fgetc(_file->stream_);
}

char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max)
{
	return fgets(_buffer, _max, _file->stream_);
}

int gpak_ungetc(gpak_file_t* _file, int _character)
{
	return ungetc(_character, _file->stream_);
}

size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file)
{
	return fread(_buffer, _elemSize, _elemCount, _file->stream_);
}

long gpak_ftell(gpak_file_t* _file)
{
	return ftell(_file->stream_);
}

long gpak_fseek(gpak_file_t* _file, long _offset, int _origin)
{
	return fseek(_file->stream_, _offset, _origin);
}

long gpak_feof(gpak_file_t* _file)
{
	return feof(_file->stream_);
}

void gpak_fclose(gpak_file_t* _file)
{
	free(_file->data_);
	fclose(_file->stream_);
	free(_file);
}