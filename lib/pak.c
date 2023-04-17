#include "pak.h"

#include "filesystem_tree.h"

#include <stdlib.h>
#include <string.h>

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

	int _error = ferror(_pak->stream_);
	if (_res * _size != _size)
	{
		if (_error = 0)
			return GPAK_ERROR_OK;
		return GPAK_ERROR_READ;
	}
		

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

int _gpak_add_file_uncompressed(gpak_t* _pak, const char* _path)
{
	FILE* _file = fopen(_path, "rb");
	if (_file == NULL)
		return GPAK_ERROR_OPEN_FILE;

	// Add stream encryprion and stream compressing here
	char _buffer[131072];
	size_t _readed = 0ull;

	do
	{
		_readed = fread(_buffer, 1ull, sizeof(_buffer), _file);
		_pak_write(_pak, _buffer, 1ull, _readed);
	} while (_readed);

	_gpak_flush(_pak);

	size_t _size = ftell(_file);
	_update_entry_header(_pak, _size, _size);

	fclose(_file);

	return GPAK_ERROR_OK;
}

int _gpak_add_file_compressed_deflate(gpak_t* _pak, const char* _path)
{
	return GPAK_ERROR_OK;
}

int _gpak_add_file_compressed_lz4(gpak_t* _pak, const char* _path)
{
	return GPAK_ERROR_OK;
}

int _gpak_add_file_compressed_zstd(gpak_t* _pak, const char* _path)
{
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

			if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_DEFLATE)
				_gpak_add_file_compressed_deflate(_pak, next_file->path_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_LZ4)
				_gpak_add_file_compressed_lz4(_pak, next_file->path_);
			else if (_pak->header_.compression_ & GPAK_HEADER_COMPRESSION_ZST)
				_gpak_add_file_compressed_zstd(_pak, next_file->path_);
			else
				_gpak_add_file_uncompressed(_pak, next_file->path_);
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
		{
			_pak->last_error_ = _res;
			_gpak_seek(_pak, 0l, SEEK_END);
			return _pak->last_error_;
		}

		size_t _size = 0ull;
		if (_entry.uncompressed_size_ == _entry.compressed_size_)
			_size = _entry.uncompressed_size_;
		else
			_size = _entry.compressed_size_;

		filesystem_tree_add_file(_pak->root_, _entry.name, NULL, _gpak_tell(_pak), _size);

		_gpak_seek(_pak, _size, SEEK_CUR);

		if (feof(_pak->stream_))
			break;
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
		open_mode_ = "rb";
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

void gpak_set_compression_algorithm(gpak_t* _pak, gpak_header_compression_algorithm_t _algorithm)
{
	_pak->header_.compression_ = _algorithm;
}

void gpak_set_compression_level(gpak_t* _pak, int _level)
{
	_pak->header_.compression_level_ = _level;
}

void gpak_set_encryption_mode(gpak_t* _pak, gpak_header_compression_algorithm_t _mode)
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

//int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path)
//{
//	int res = _write_entry_header(_pak, _internal_path, gpak_ENTRY_FILE);
//	if (res != gpak_ERROR_OK)
//		return res;
//
//	FILE* _file = fopen(_external_path, "rb");
//	if (_file == NULL)
//		return gpak_ERROR_OPEN_FILE;
//
//	// Add stream encryprion and stream compressing here
//	char _buffer[131072];
//	size_t _readed = 0ull;
//	
//	do 
//	{
//		_readed = fread(_buffer, 1ull, sizeof(_buffer), _file);
//		_pak_write(_pak, _buffer, 1ull, _readed);
//	} while (_readed);
//
//	_gpak_flush(_pak);
//
//	size_t _size = ftell(_file);
//	_update_entry_header(_pak, _size, _size);
//
//	fclose(_file);
//
//	return gpak_ERROR_OK;
//}

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