#include "gpak_helper.h"

int _gpak_make_error(gpak_t* _pak, int _error_code)
{
	_pak->last_error_ = _error_code;

	if (_pak->error_handler_ && _error_code != GPAK_ERROR_OK)
	{
		_pak->error_handler_(_pak->current_file_, _error_code, _pak->user_data_);
	}

	return _error_code;
}

void _gpak_pass_progress(gpak_t* _pak, size_t _done, size_t _total, int32_t _stage)
{
	if (_pak->progress_handler_)
		_pak->progress_handler_(_pak->current_file_, _done, _total, _stage, _pak->user_data_);
}

size_t _fwriteb(const void* _data, size_t _elemSize, size_t _elemCount, FILE* _file)
{
	return fwrite(_data, _elemSize, _elemCount, _file) * _elemSize;
}

size_t _freadb(void* _data, size_t _elemSize, size_t _elemCount, FILE* _file)
{
	return fread(_data, _elemSize, _elemCount, _file) * _elemSize;
}