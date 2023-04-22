#include "gpak.h"

#include "filesystem_tree.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#ifdef _WIN32
#include "libfmemopen.h"
#endif

#include "gpak_compressors.h"
#include "gpak_helper.h"

// Openssl
#include <openssl/aes.h>
#include <openssl/rand.h>

//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//---------------------------------HELPERS--------------------------------
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_
//-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-_

pak_header_t _pak_make_header()
{
	pak_header_t _header;

	strcpy(_header.format_, "gpak\0");
	_header.compression_ = GPAK_HEADER_COMPRESSION_NONE;
	_header.encryption_ = GPAK_HEADER_ENCRYPTION_NONE;
	_header.compression_level_ = 0;
	_header.entry_count_ = 0u;
	_header.dictionary_size_ = 0u;

	return _header;
}

int _pak_validate_header(gpak_t* _pak)
{
	if (strcmp(_pak->header_.format_, "gpak") != 0)
		return _gpak_make_error(_pak, GPAK_ERROR_INCORRECT_HEADER);

	return _gpak_make_error(_pak, GPAK_ERROR_OK);
}

long _write_entry_header(gpak_t* _pak, const char* _path, pak_entry_t* _entry)
{
	long bytes_writen = 0u;

	// Write filename
	short name_size = strlen(_path);
	bytes_writen += _fwriteb(&name_size, sizeof(short), 1ull, _pak->stream_);
	bytes_writen += _fwriteb(_path, 1ull, name_size, _pak->stream_);
	bytes_writen += _fwriteb(_entry, sizeof(pak_entry_t), 1ull, _pak->stream_);

	return bytes_writen;
}

long _read_entry_header(gpak_t* _pak, char* _path, pak_entry_t* _entry)
{
	long bytes_readed = 0u;

	short name_size;
	bytes_readed += _freadb(&name_size, sizeof(short), 1ull, _pak->stream_);
	bytes_readed += _freadb(_path, 1ull, name_size, _pak->stream_);
	bytes_readed += _freadb(_entry, sizeof(pak_entry_t), 1ull, _pak->stream_);
	_path[name_size] = '\0';

	return bytes_readed;
}

int _update_entry_header(gpak_t* _pak, size_t _commressed_size, size_t _uncompressed_size, long _entry_size, uint32_t _crc32)
{
	pak_entry_t _entry;

	if (!_pak->stream_ )
		return _gpak_make_error(_pak, GPAK_ERROR_STREAM_IS_NULL);

	size_t _shift = 0ull;
	if (_uncompressed_size == _commressed_size)
		_shift = _uncompressed_size;
	else
		_shift = _commressed_size;

	long _cur_pos = ftell(_pak->stream_);
	fseek(_pak->stream_, -((long)_shift), SEEK_CUR);
	long _offset = ftell(_pak->stream_);
	fseek(_pak->stream_, -_entry_size, SEEK_CUR);

	char _entry_path[256];
	_read_entry_header(_pak, _entry_path, &_entry);

	_entry.compressed_size_ = _commressed_size;
	_entry.uncompressed_size_ = _uncompressed_size;
	_entry.crc32_ = _crc32;
	_entry.offset_ = _offset;

	fseek(_pak->stream_, -_entry_size, SEEK_CUR);

	_write_entry_header(_pak, _entry_path, &_entry);

	fseek(_pak->stream_, 0l, SEEK_END);

	if (_cur_pos != ftell(_pak->stream_))
		return _gpak_make_error(_pak, GPAK_ERROR_WRITE);

	return GPAK_ERROR_OK;
}

int _update_pak_header(gpak_t* _pak)
{
	fseek(_pak->stream_, 0, SEEK_SET);

	size_t readed = _fwriteb(&_pak->header_, sizeof(pak_header_t), 1ull, _pak->stream_);
	if (readed != sizeof(pak_header_t))
	{
		fseek(_pak->stream_, 0l, SEEK_END);
		return _gpak_make_error(_pak, GPAK_ERROR_WRITE);
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
		_readed = _freadb(_buffer, 1ull, sizeof(_buffer), _file);

		if (_readed < sizeof(_buffer))
		{
			int padding = sizeof(_buffer) - _readed;
			memset(_buffer + _readed, padding, padding);
		}

		AES_cbc_encrypt(_buffer, _bufferout, sizeof(_buffer), &aesKey, iv, AES_ENCRYPT);

		_fwriteb(_bufferout, 1ull, _readed, _pak->stream_);
	} while (_readed);

	free(iv);

	return GPAK_ERROR_OK;
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
			_pak->current_file_ = filesystem_tree_file_path(next_directory, next_file);
			pak_entry_t _entry;
			long header_size = _write_entry_header(_pak, _pak->current_file_, &_entry);

			FILE* _infile = fopen(next_file->path_, "rb");
			FILE* _outfile = NULL;
			if (!_infile)
				return _gpak_make_error(_pak, GPAK_ERROR_OPEN_FILE);

			long cursor = ftell(_pak->stream_);

			uint32_t _crc32 = 0u;
			if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
				_crc32 = _gpak_compressor_deflate(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
				_crc32 = _gpak_compressor_lz4(_pak, _infile, _pak->stream_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
				_crc32 = _gpak_compressor_zstd(_pak, _infile, _pak->stream_);
			else
				_crc32 = _gpak_compressor_none(_pak, _infile, _pak->stream_);

			long position = ftell(_pak->stream_);
			size_t uncompressed_size = ftell(_infile);
			size_t compressed_size = position - cursor;

			fflush(_pak->stream_);

			_update_entry_header(_pak, compressed_size, uncompressed_size, header_size, _crc32);

			free(_pak->current_file_);
			fclose(_infile);
		}
	} while ((next_directory = filesystem_iterator_next_directory(iterator)));
	
	filesystem_iterator_free(iterator);

	return _gpak_make_error(_pak, GPAK_ERROR_OK);
}

int _gpak_parse_dictionary(gpak_t* _pak)
{
	if(_pak->header_.dictionary_size_ == 0u)
		return _gpak_make_error(_pak, GPAK_ERROR_OK);

	_pak->dictionary_ = (char*)malloc(_pak->header_.dictionary_size_ + 1);
	_pak->dictionary_[_pak->header_.dictionary_size_] = '\0';

	size_t readed = _freadb(_pak->dictionary_, 1ull, _pak->header_.dictionary_size_, _pak->stream_);

	if(readed != _pak->header_.dictionary_size_)
		return _gpak_make_error(_pak, GPAK_ERROR_INVALID_DICTIONARY);

	return _gpak_make_error(_pak, GPAK_ERROR_OK);
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

		filesystem_tree_add_file(_pak->root_, _filename, NULL, _entry);

		fseek(_pak->stream_, _entry.compressed_size_, SEEK_CUR);
	}

	return _gpak_make_error(_pak, GPAK_ERROR_OK);
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
	pak->current_file_ = NULL;
	pak->error_handler_ = NULL;
	pak->progress_handler_ = NULL;
	pak->user_data_ = NULL;
	pak->dictionary_ = NULL;

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
		size_t res = _fwriteb(&pak->header_, sizeof(pak_header_t), 1ull, pak->stream_);
		if (res != sizeof(pak_header_t))
		{
			gpak_close(pak);
			return NULL;
		}
	}
	else if (_mode & GPAK_MODE_READ_ONLY)
	{
		size_t res = _freadb(&pak->header_, sizeof(pak_header_t), 1ull, pak->stream_);
		if (_pak_validate_header(pak) != GPAK_ERROR_OK || res != sizeof(pak_header_t))
		{
			gpak_close(pak);
			return NULL;
		}

		_gpak_parse_dictionary(pak);
		_gpak_parse_file_tree(pak);
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

			if ((_pak->mode_ & GPAK_MODE_CREATE) || (_pak->mode_ & GPAK_MODE_UPDATE))
			{
				if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE);
				else if ((_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4) || (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST))
					_gpak_compressor_generate_dictionary(_pak);
				else;

				_gpak_archivate_file_tree(_pak);
			}

			filesystem_tree_delete(_pak->root_);

			_update_pak_header(_pak);
			fclose(_pak->stream_);
		}
		
		free(_pak->password_);
		free(_pak->dictionary_);
		free(_pak);
		return _gpak_make_error(_pak, GPAK_ERROR_OK);
	}
	
	return -1;
}

GPAK_API void gpak_set_user_data(gpak_t* _pak, void* _user_data)
{
	_pak->user_data_ = _user_data;
}

GPAK_API void gpak_set_error_handler(gpak_t* _pak, gpak_error_handler_t _error_handler)
{
	_pak->error_handler_ = _error_handler;
}

GPAK_API void gpak_set_process_handler(gpak_t* _pak, gpak_progress_handler_t _progress_handler)
{
	_pak->progress_handler_ = _progress_handler;
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
	return _gpak_make_error(_pak, GPAK_ERROR_OK);
}

int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path)
{
	pak_entry_t _entry;
	_entry.compressed_size_ = 0u;
	_entry.uncompressed_size_ = 0u;
	_entry.offset_ = 0u;
	_entry.crc32_ = 0;

	++_pak->header_.entry_count_;

	filesystem_tree_add_file(_pak->root_, _internal_path, _external_path, _entry);
	return _gpak_make_error(_pak, GPAK_ERROR_OK);
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
		_gpak_make_error(_pak, GPAK_ERROR_FILE_NOT_FOUND);
		return NULL;
	}

	_pak->current_file_ = filesystem_tree_file_path(_pak->root_, _file_info);

	uint32_t uncompressed_size = _file_info->entry_.uncompressed_size_;
	uint32_t compressed_size = _file_info->entry_.compressed_size_;

	gpak_file_t* mfile = (gpak_file_t*)malloc(sizeof(gpak_file_t));
	mfile->data_ = (char*)malloc(uncompressed_size + 1);
	mfile->data_[uncompressed_size] = '\0';

	mfile->stream_ = fmemopen(mfile->data_, uncompressed_size, "wb+");

	// Setting position to file start
	fseek(_pak->stream_, _file_info->entry_.offset_, SEEK_SET);

	if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
		mfile->crc32_ = _gpak_decompressor_inflate(_pak, _pak->stream_, mfile->stream_, compressed_size);
	else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
		mfile->crc32_ = _gpak_decompressor_lz4(_pak, _pak->stream_, mfile->stream_, compressed_size);
	else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
		mfile->crc32_ = _gpak_decompressor_zstd(_pak, _pak->stream_, mfile->stream_, compressed_size);
	else
		mfile->crc32_ = _gpak_decompressor_none(_pak, _pak->stream_, mfile->stream_, compressed_size);

	fseek(mfile->stream_, 0, SEEK_SET);

	// Check crc32
	if (mfile->crc32_ != _file_info->entry_.crc32_)
	{
		_gpak_make_error(_pak, GPAK_ERROR_FILE_CRC_NOT_MATCH);
		free(_pak->current_file_);
		gpak_fclose(mfile);
		return NULL;
	}

	free(_pak->current_file_);

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