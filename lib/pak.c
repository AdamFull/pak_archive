#include "pak.h"

#include "filesystem_tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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

#define _DEFAULT_BLOCK_SIZE 16 * 1024

// Helpers
pak_header_t _pak_make_header()
{
	pak_header_t _header;

	strcpy(_header.format_, "gpak");
	_header.compression_ = GPAK_HEADER_COMPRESSION_NONE;
	_header.encryption_ = GPAK_HEADER_ENCRYPTION_NONE;
	_header.compression_level_ = 0;
	_header.compressed_size_ = 0ull;
	_header.uncompressed_size_ = 0ull;
	_header.entry_count_ = 0ull;

	return _header;
}

int _pak_validate_header(gpak_t* _pak)
{
	if (strcmp(_pak->header_.format_, "gpak") != 0)
		return GPAK_ERROR_INCORRECT_HEADER;

	return GPAK_ERROR_OK;
}

int _pak_write_single(gpak_t* _pak, const void* _data, size_t _size)
{
	if (!_pak->stream_ )
		return GPAK_ERROR_STREAM_IS_NULL;

	if (_pak->mode_ & GPAK_MODE_READ_ONLY)
		return GPAK_ERROR_INCORRECT_MODE;

	size_t _res = fwrite(_data, _size, 1ull, _pak->stream_);

	if (_res * _size != _size)
		return GPAK_ERROR_WRITE;

	int _fres = _gpak_flush(_pak);
	if (_fres < 0)
		return GPAK_ERROR_WRITE;

	return GPAK_ERROR_OK;
}

int _pak_read_single(gpak_t* _pak, void* _data, size_t _size)
{
	if (!_pak->stream_ )
		return GPAK_ERROR_STREAM_IS_NULL;

	size_t _res = fread(_data, _size, 1ull, _pak->stream_);

	if (_res * _size != _size)
		return GPAK_ERROR_READ;

	return GPAK_ERROR_OK;
}

size_t _pak_write(gpak_t* _pak, const void* _data, size_t _size, size_t _count)
{
	if (!_pak->stream_)
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return 0ull;
	}

	if (_pak->mode_ & GPAK_MODE_READ_ONLY)
	{
		_pak->last_error_ = GPAK_ERROR_INCORRECT_MODE;
		return 0ull;
	}

	size_t _res = fwrite(_data, _size, _count, _pak->stream_);

	int _fres = _gpak_flush(_pak);
	if (_fres < 0)
		return 0ull;

	return _res * _size;
}

size_t _pak_read(gpak_t* _pak, void* _data, size_t _size, size_t _count)
{
	if (!_pak->stream_)
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return 0ull;
	}

	if (_pak->mode_ & GPAK_MODE_CREATE)
	{
		_pak->last_error_ = GPAK_ERROR_INCORRECT_MODE;
		return 0ull;
	}

	return fread(_data, _size, _count, _pak->stream_) * _size;
}

int _write_entry_header(gpak_t* _pak, const char* _path)
{
	pak_entry_t _entry;
	strcpy(_entry.name, _path);
	_entry.offset_ = _gpak_tell(_pak);
	_entry.compressed_size_ = 0ull;
	_entry.uncompressed_size_ = 0ull;

	_pak->header_.compressed_size_ += sizeof(pak_entry_t);
	_pak->header_.uncompressed_size_ += sizeof(pak_entry_t);
	++_pak->header_.entry_count_;

	return _pak_write_single(_pak, &_entry, sizeof(pak_entry_t));
}

int _update_entry_header(gpak_t* _pak, size_t _commressed_size, size_t _uncompressed_size)
{
	if (!_pak->stream_ )
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return _pak->last_error_;
	}

	size_t _shift = 0ull;
	if (_uncompressed_size == _commressed_size)
		_shift = _uncompressed_size;
	else
		_shift = _commressed_size;

	long _entry_size = sizeof(pak_entry_t);
	size_t _cur_pos = _gpak_tell(_pak);
	long _header_start_pos = (long)(_shift + _entry_size);

	_gpak_seek(_pak, -_header_start_pos, SEEK_CUR);

	pak_entry_t _entry;
	int _res = _pak_read_single(_pak, &_entry, _entry_size);
	if (_res != GPAK_ERROR_OK)
	{
		_pak->last_error_ = _res;
		_gpak_seek(_pak, 0l, SEEK_END);
		return _pak->last_error_;
	}

	_entry.compressed_size_ = _commressed_size;
	_entry.uncompressed_size_ = _uncompressed_size;

	_pak->header_.compressed_size_ += _commressed_size;
	_pak->header_.uncompressed_size_ += _uncompressed_size;

	_gpak_seek(_pak, -_entry_size, SEEK_CUR);

	_res = _pak_write_single(_pak, &_entry, _entry_size);
	if (_res != GPAK_ERROR_OK)
	{
		_pak->last_error_ = _res;
		_gpak_seek(_pak, 0l, SEEK_END);
		return _pak->last_error_;
	}

	_gpak_seek(_pak, 0l, SEEK_END);

	if (_cur_pos != _gpak_tell(_pak))
	{
		_pak->last_error_ = GPAK_ERROR_WRITE;
		return _pak->last_error_;
	}

	return GPAK_ERROR_OK;
}

int _update_pak_header(gpak_t* _pak)
{
	_gpak_seek(_pak, 0, SEEK_SET);

	int _res = _pak_write_single(_pak, &_pak->header_, sizeof(pak_header_t));
	if (_res != GPAK_ERROR_OK)
	{
		_pak->last_error_ = _res;
		_gpak_seek(_pak, 0l, SEEK_END);
		return _pak->last_error_;
	}

	// TODO: Add checksum

	return GPAK_ERROR_OK;
}

int _gpak_seek(gpak_t* _pak, long _offset, int _origin)
{
	if (!_pak->stream_ )
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return 0;
	}

	return fseek(_pak->stream_, _offset, _origin);
}

long _gpak_tell(gpak_t* _pak)
{
	if (!_pak->stream_ )
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return 0l;
	}

	return ftell(_pak->stream_);
}

int _gpak_flush(gpak_t* _pak)
{
	if (!_pak->stream_ )
	{
		_pak->last_error_ = GPAK_ERROR_STREAM_IS_NULL;
		return 0;
	}

	return fflush(_pak->stream_);
}

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
		_readed = fread(_buffer, 1ull, sizeof(_buffer), _file);

		if (_readed < sizeof(_buffer))
		{
			int padding = sizeof(_buffer) - _readed;
			memset(_buffer + _readed, padding, padding);
		}

		AES_cbc_encrypt(_buffer, _bufferout, sizeof(_buffer), &aesKey, iv, AES_ENCRYPT);

		_pak_write(_pak, _bufferout, 1ull, _readed);
	} while (_readed);

	free(iv);

	return GPAK_ERROR_OK;
}

int _gpak_compress_stream_none(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	char _buffer[_DEFAULT_BLOCK_SIZE];
	size_t _readed = 0ull;

	do
	{
		_readed = fread(_buffer, 1ull, sizeof(_buffer), _infile);
		fwrite(_buffer, 1ull, _readed, _outfile);
	} while (_readed);

	return GPAK_ERROR_OK;
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
	{
		_pak->last_error_ = GPAK_ERROR_DEFLATE_INIT;
		return _pak->last_error_;
	}

	int flush;
	unsigned have;
	do 
	{
		strm.avail_in = fread(_bufferIn, 1ull, _DEFAULT_BLOCK_SIZE, _infile);
		if (ferror(_infile)) 
		{
			_pak->last_error_ = GPAK_ERROR_READ;
			(void)deflateEnd(&strm);
			return _pak->last_error_;
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
			if (fwrite(_bufferOut, 1ull, have, _outfile) != have || ferror(_outfile)) 
			{
				_pak->last_error_ = GPAK_ERROR_WRITE;
				(void)deflateEnd(&strm);
				return _pak->last_error_;
			}
		} while (strm.avail_out == 0);
		assert(strm.avail_in == 0);

	} while (flush != Z_FINISH);

	return GPAK_ERROR_OK;
}

int _gpak_compress_stream_lz4(gpak_t* _pak, FILE* _infile, FILE* _outfile)
{
	assert(_infile != NULL); 
	assert(_outfile != NULL);

	LZ4F_errorCode_t ret = LZ4F_OK_NoError;
	size_t len;
	LZ4_writeFile_t* lz4fWrite;
	void* const buf = malloc(_DEFAULT_BLOCK_SIZE);

	ret = LZ4F_writeOpen(&lz4fWrite, _outfile, NULL);
	if (LZ4F_isError(ret)) 
	{
		_pak->last_error_ = GPAK_ERROR_LZ4_WRITE_OPEN;
		free(buf);
		return _pak->last_error_;
	}

	while (true) 
	{
		len = fread(buf, 1, _DEFAULT_BLOCK_SIZE, _infile);

		if (ferror(_infile)) 
		{
			_pak->last_error_ = GPAK_ERROR_READ;
			goto out;
		}

		/* nothing to read */
		if (len == 0)
			break;

		ret = LZ4F_write(lz4fWrite, buf, len);
		if (LZ4F_isError(ret)) 
		{
			_pak->last_error_ = GPAK_ERROR_LZ4_WRITE;
			goto out;
		}
	}

out:
	free(buf);
	if (LZ4F_isError(LZ4F_writeClose(lz4fWrite))) 
	{
		_pak->last_error_ = GPAK_ERROR_LZ4_WRITE_CLOSE;
		return _pak->last_error_;
	}

	return GPAK_ERROR_OK;
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
		size_t read = fread(buffIn, 1, toRead, _infile);
		int const lastChunk = (read < toRead);
		ZSTD_EndDirective const mode = lastChunk ? ZSTD_e_end : ZSTD_e_continue;

		ZSTD_inBuffer input = { buffIn, read, 0 };
		int finished;
		do 
		{
			ZSTD_outBuffer output = { buffOut, buffOutSize, 0 };
			size_t const remaining = ZSTD_compressStream2(cctx, &output, &input, mode);
			//CHECK_ZSTD(remaining);
			fwrite(buffOut, 1, output.pos, _outfile);
			finished = lastChunk ? (remaining == 0) : (input.pos == input.size);
		} while (!finished);
		assert(input.pos == input.size && "Impossible: zstd only returns 0 when the input is completely consumed!");

		if (lastChunk)
			break;
	}

	ZSTD_freeCCtx(cctx);
	free(buffIn);
	free(buffOut);

	return GPAK_ERROR_OK;
}

int _gpak_get_file_uncompressed(gpak_t* _pak, gpak_file_t* _file, size_t _size)
{
	char _buffer[_DEFAULT_BLOCK_SIZE];
	size_t bytesReaded = 0ull;
	size_t nextBlockSize = sizeof(_buffer);
	do
	{
		size_t _readed = fread(_buffer, 1ull, nextBlockSize, _pak->stream_);
		memcpy(_file->data_ + bytesReaded, _buffer, _readed);
		bytesReaded += _readed;

		if (_size - bytesReaded < nextBlockSize)
			nextBlockSize = _size - bytesReaded;
	} while (bytesReaded != _size);

	if (bytesReaded != _size)
	{
		_pak->last_error_ = GPAK_ERROR_READ;
		return _pak->last_error_;
	}

	return GPAK_ERROR_OK;
}

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
			int res = _write_entry_header(_pak, internal_filepath);
			free(internal_filepath);

			FILE* _infile = fopen(next_file->path_, "rb");
			FILE* _outfile = NULL;
			if (!_infile)
				return GPAK_ERROR_OPEN_FILE;

			//bool _encrypt = (_pak->header_.encryption_ & GPAK_HEADER_ENCRYPTION_AES128) || (_pak->header_.encryption_ & GPAK_HEADER_ENCRYPTION_AES256);
			//
			//if(_encrypt)
			//{
			//	int _namesize = sprintf(NULL, "%s%s", next_file->name_, ".tmp");
			//	//char* _tempfilename = 
			//}

			long cursor = _gpak_tell(_pak);

			if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
				_gpak_compress_stream_deflate(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
				_gpak_compress_stream_lz4(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
				_gpak_compress_stream_zstd(_pak, _infile, _pak->stream_);
			else
				_gpak_compress_stream_none(_pak, _infile, _pak->stream_);

			size_t uncompressed_size = ftell(_infile);
			size_t compressed_size = _gpak_tell(_pak) - cursor;

			_gpak_flush(_pak);

			_update_entry_header(_pak, compressed_size, uncompressed_size);

			fclose(_infile);
		}
	} while ((next_directory = filesystem_iterator_next_directory(iterator)));
	
	filesystem_iterator_free(iterator);

	return GPAK_ERROR_OK;
}

int _gpak_parse_file_tree(gpak_t* _pak)
{
	while (1)
	{
		pak_entry_t _entry;
		int _res = _pak_read_single(_pak, &_entry, sizeof(pak_entry_t));
		if (_res != GPAK_ERROR_OK)
			break;	// EOF

		size_t _size = 0ull;
		if (_entry.uncompressed_size_ == _entry.compressed_size_)
			_size = _entry.uncompressed_size_;
		else
			_size = _entry.compressed_size_;

		filesystem_tree_add_file(_pak->root_, _entry.name, NULL, _gpak_tell(_pak), _size);

		_gpak_seek(_pak, _size, SEEK_CUR);
	}

	return GPAK_ERROR_OK;
}


// Implementation
gpak_t* gpak_open(const char* _path, int _mode)
{
	gpak_t* pak;
	pak = (gpak_t*)calloc(1, sizeof(gpak_t));

	if (pak == NULL)
		return NULL;

	pak->mode_ = _mode;

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
		int res = _pak_write_single(pak, &pak->header_, sizeof(pak_header_t));
		if (res != GPAK_ERROR_OK)
		{
			gpak_close(pak);
			return NULL;
		}
	}
	else if (_mode & GPAK_MODE_READ_ONLY)
	{
		int res = _pak_read_single(pak, &pak->header_, sizeof(pak_header_t));
		if (_pak_validate_header(pak) != GPAK_ERROR_OK || res != GPAK_ERROR_OK)
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
		return GPAK_ERROR_OK;
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
	return GPAK_ERROR_OK;
}

int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path)
{
	filesystem_tree_add_file(_pak->root_, _internal_path, _external_path, 0ull, 0ull);
	return GPAK_ERROR_OK;
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
		_pak->last_error_ = GPAK_ERROR_FILE_NOT_FOUND;
		return NULL;
	}

	gpak_file_t* mfile = (gpak_file_t*)malloc(sizeof(gpak_file_t));
	mfile->data_ = (char*)malloc(_file_info->size_ + 1);
	mfile->data_[_file_info->size_] = '\0';

	// Setting position to file start
	_gpak_seek(_pak, _file_info->offset_, SEEK_SET);

	int res = _gpak_get_file_uncompressed(_pak, mfile, _file_info->size_);
	if (res != GPAK_ERROR_OK)
	{
		free(mfile->data_);
		free(mfile);
		return NULL;
	}

	mfile->begin_ = mfile->current_ = mfile->data_;
	mfile->end_ = mfile->data_ + _file_info->size_;
	mfile->ungetc_buffer_ = EOF;

	return mfile;
}

int gpak_fgetc(gpak_file_t* _file)
{
	if (_file->ungetc_buffer_ != EOF)
	{
		int result = _file->ungetc_buffer_;
		_file->ungetc_buffer_ = EOF;
		return result;
	}

	if (_file->current_ >= _file->end_)
		return EOF;

	return (int)(*_file->current_++);
}

char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max)
{
	if (_file->current_ >= _file->end_)
		return NULL;

	int i = 0;
	while (i < _max - 1)
	{
		auto c = gpak_fgetc(_file);
		if (c == EOF)
			break;

		_buffer[i++] = (char)c;
		if (c == '\n')
			break;
	}

	_buffer[i] = '\0';
	return _buffer;
}

int gpak_ungetc(gpak_file_t* _file, int _character)
{
	if (_file->ungetc_buffer_ != EOF || _file->current_ <= _file->end_)
		return EOF;

	_file->ungetc_buffer_ = _character;
	return _character;
}

size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file)
{
	size_t n = _elemSize * _elemCount;
	if (n > (size_t)(_file->end_ - _file->current_))
		n = _file->end_ - _file->current_;

	memcpy(_buffer, _file->current_, n);
	_file->current_ += n;
	return n / _elemSize;
}

long gpak_ftell(gpak_file_t* _file)
{
	return (long)(_file->current_ - _file->begin_);
}

long gpak_fseek(gpak_file_t* _file, long _offset, int _origin)
{
	const char* new_current;

	switch (_origin)
	{
	case SEEK_SET: new_current = _file->begin_ + _offset; break;
	case SEEK_CUR: new_current = _file->current_ + _offset; break;
	case SEEK_END: new_current = _file->end_ + _offset; break;
	default: return -1;
	}

	if (new_current < _file->begin_ || new_current > _file->end_)
		return -1;

	_file->current_ = new_current;
	return 0;
}

long gpak_feof(gpak_file_t* _file)
{
	return _file->current_ >= _file->end_;
}

void gpak_fclose(gpak_file_t* _file)
{
	free(_file->data_);
	free(_file);
}