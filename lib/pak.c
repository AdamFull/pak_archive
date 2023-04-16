#include "pak.h"

#include <stdlib.h>
#include <string.h>

// Helpers
pak_header_t _pak_make_header()
{
	pak_header_t _header;

	_header.format_[0] = "p";
	_header.format_[1] = "a";
	_header.format_[2] = "k";
	_header.compression_ = PAK_ARCHIVE_HEADER_COMPRESSION_NONE;
	_header.encryption_ = PAK_ARCHIVE_HEADER_ENCRYPTION_NONE;
	_header.compression_level_ = 0;
	_header.compressed_size_ = 0ull;
	_header.uncompressed_size_ = 0ull;
	_header.entry_count_ = 0ull;

	return _header;
}

int _pak_validate_header(pak_t* _pak)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (strcmp(_pak->header_.format_, "pak") != 0)
		return PAK_ARCHIVE_ERROR_INCORRECT_HEADER;

	return PAK_ARCHIVE_ERROR_OK;
}

int _pak_write_single(pak_t* _pak, const void* _data, size_t _size)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	if (_pak->mode_ & PAK_ARCHIVE_MODE_READ_ONLY)
		return PAK_ARCHIVE_ERROR_INCORRECT_MODE;

	size_t _res = fwrite(_data, _size, 1ull, _pak->stream_);

	if (_res * _size != _size)
		return PAK_ARCHIVE_ERROR_WRITE;

	int _fres = _pak_archive_flush(_pak);
	if (_fres < 0)
		return PAK_ARCHIVE_ERROR_WRITE;

	return PAK_ARCHIVE_ERROR_OK;
}

int _pak_read_single(pak_t* _pak, void* _data, size_t _size)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	//if (_pak->mode_ & PAK_ARCHIVE_MODE_CREATE)
	//	return PAK_ARCHIVE_ERROR_INCORRECT_MODE;

	size_t _res = fread(_data, _size, 1ull, _pak->stream_);

	if (_res * _size != _size)
		return PAK_ARCHIVE_ERROR_READ;

	return PAK_ARCHIVE_ERROR_OK;
}

size_t _pak_write(pak_t* _pak, const void* _data, size_t _size, size_t _count)
{
	if (_pak == NULL)
		return 0ull;

	if (_pak->stream_ == NULL)
		return 0ull;

	if (_pak->mode_ & PAK_ARCHIVE_MODE_READ_ONLY)
		return 0ull;

	size_t _res = fwrite(_data, _size, _count, _pak->stream_);

	int _fres = _pak_archive_flush(_pak);
	if (_fres < 0)
		return 0ull;

	return _res * _size;
}

size_t _pak_read(pak_t* _pak, void* _data, size_t _size, size_t _count)
{
	if (_pak == NULL)
		return 0ull;

	if (_pak->stream_ == NULL)
		return 0ull;

	if (_pak->mode_ & PAK_ARCHIVE_MODE_CREATE)
		return 0ull;

	return fread(_data, _size, _count, _pak->stream_) * _size;
}

int _write_entry_header(pak_t* _pak, const char* _path, int _type)
{
	pak_entry_t _entry;
	strcpy(_entry.name, _path);
	_entry.offset_ = _pak_archive_tell(_pak);
	_entry.compressed_size_ = 0ull;
	_entry.uncompressed_size_ = 0ull;
	_entry.type_ = _type;

	return _pak_write_single(_pak, &_entry, sizeof(pak_entry_t));
}

int _update_entry_header(pak_t* _pak, size_t _commressed_size, size_t _uncompressed_size)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	size_t _shift = 0ull;
	if (_uncompressed_size == _commressed_size)
		_shift = _uncompressed_size;
	else
		_shift = _commressed_size;

	size_t _entry_size = sizeof(pak_entry_t);
	size_t _cur_pos = _pak_archive_tell(_pak);
	long _header_start_pos = (long)(_shift + _entry_size);

	_pak_archive_seek(_pak, -_header_start_pos, SEEK_CUR);

	pak_entry_t _entry;
	int _res = _pak_read_single(_pak, &_entry, _entry_size);
	if (_res != PAK_ARCHIVE_ERROR_OK)
	{
		_pak_archive_seek(_pak, 0l, SEEK_END);
		return _res;
	}

	_entry.compressed_size_ = _commressed_size;
	_entry.uncompressed_size_ = _uncompressed_size;

	_pak_archive_seek(_pak, -_header_start_pos, SEEK_CUR);

	_res = _pak_write_single(_pak, &_entry, _entry_size);
	if (_res != PAK_ARCHIVE_ERROR_OK)
	{
		_pak_archive_seek(_pak, 0l, SEEK_END);
		return _res;
	}

	_pak_archive_seek(_pak, 0l, SEEK_END);

	if (_cur_pos != _pak_archive_tell(_pak))
		return PAK_ARCHIVE_ERROR_WRITE;

	return PAK_ARCHIVE_ERROR_OK;
}

int _pak_archive_seek(pak_t* _pak, long _offset, int _origin)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	return fseek(_pak->stream_, _offset, _origin);
}

long _pak_archive_tell(pak_t* _pak)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	return ftell(_pak->stream_);
}

int _pak_archive_flush(pak_t* _pak)
{
	if (_pak == NULL)
		return PAK_ARCHIVE_ERROR_ARCHIVE_IS_NULL;

	if (_pak->stream_ == NULL)
		return PAK_ARCHIVE_ERROR_STREAM_IS_NULL;

	return fflush(_pak->stream_);
}

int _pak_archive_add_file_uncompressed(pak_t* _pak, const char* _path)
{
	FILE* _file = fopen(_path, "rb");
	if (_file == NULL)
		return PAK_ARCHIVE_ERROR_OPEN_FILE;

	// Add stream encryprion and stream compressing here
	char _buffer[131072];
	size_t _readed = 0ull;

	do
	{
		_readed = fread(_buffer, 1ull, sizeof(_buffer), _file);
		_pak_write(_pak, _buffer, 1ull, _readed);
	} while (_readed);

	_pak_archive_flush(_pak);

	size_t _size = ftell(_file);
	_update_entry_header(_pak, _size, _size);

	fclose(_file);

	return PAK_ARCHIVE_ERROR_OK;
}


// Implementation
pak_t* pak_archive_open(const char* _path, int _mode)
{
	pak_t* pak;
	pak = (pak_t*)calloc(1, sizeof(pak_t));

	if (pak == NULL)
		return NULL;

	pak->mode_ = _mode;

	const char* open_mode_;
	if (_mode & PAK_ARCHIVE_MODE_CREATE)
		open_mode_ = "wb+";
	else if (_mode & PAK_ARCHIVE_MODE_READ_ONLY)
		open_mode_ = "rb";
	else
	{
		pak_archive_close(pak);
		return NULL;
	}
	
	// Trying to open pak file
	pak->stream_ = fopen(_path, open_mode_);

	if (pak->stream_ == NULL)
	{
		pak_archive_close(pak);
		return NULL;
	}

	if (_mode & PAK_ARCHIVE_MODE_CREATE)
	{
		pak->header_ = _pak_make_header();
		int res = _pak_write_single(pak, &pak->header_, sizeof(pak_header_t));
		if (res != PAK_ARCHIVE_ERROR_OK)
		{
			pak_archive_close(pak);
			return NULL;
		}
	}
	else if (_mode & PAK_ARCHIVE_MODE_READ_ONLY)
	{
		int res = _pak_read_single(pak, &pak->header_, sizeof(pak_header_t));

		if (_pak_validate_header(pak) != PAK_ARCHIVE_ERROR_OK || res != PAK_ARCHIVE_ERROR_OK)
		{
			pak_archive_close(pak);
			return NULL;
		}
	}

	return pak;
}

int pak_archive_close(pak_t* _pak)
{
	if (_pak != NULL)
	{
		if (_pak->stream_ != NULL)
		{
			// Recalculate header

			_pak_archive_flush(_pak);
			fclose(_pak->stream_);
		}
			
		free(_pak);
		return PAK_ARCHIVE_ERROR_OK;
	}
	
	return -1;
}

int pak_archive_add_directory(pak_t* _pak, const char* _internal_path)
{
	return _write_entry_header(_pak, _internal_path, PAK_ARCHIVE_ENTRY_DIRECTORY);
}

int pak_archive_add_file(pak_t* _pak, const char* _external_path, const char* _internal_path)
{
	int res = _write_entry_header(_pak, _internal_path, PAK_ARCHIVE_ENTRY_FILE);
	if (res != PAK_ARCHIVE_ERROR_OK)
		return res;

	FILE* _file = fopen(_external_path, "rb");
	if (_file == NULL)
		return PAK_ARCHIVE_ERROR_OPEN_FILE;

	// Add stream encryprion and stream compressing here
	char _buffer[131072];
	size_t _readed = 0ull;
	
	do 
	{
		_readed = fread(_buffer, 1ull, sizeof(_buffer), _file);
		_pak_write(_pak, _buffer, 1ull, _readed);
	} while (_readed);

	_pak_archive_flush(_pak);

	size_t _size = ftell(_file);
	_update_entry_header(_pak, _size, _size);

	fclose(_file);

	return PAK_ARCHIVE_ERROR_OK;
}