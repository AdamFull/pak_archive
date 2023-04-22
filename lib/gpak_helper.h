#ifndef GPAK_HELPER_H
#define GPAK_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "gpak_export.h"
#include "gpak_data.h"

	GPAK_API int _gpak_make_error(gpak_t* _pak, int _error_code);
	GPAK_API void _gpak_pass_progress(gpak_t* _pak, size_t _done, size_t _total, int32_t _stage);

	GPAK_API size_t _fwriteb(const void* _data, size_t _elemSize, size_t _elemCount, FILE* _file);
	GPAK_API size_t _freadb(void* _data, size_t _elemSize, size_t _elemCount, FILE* _file);

#ifdef __cplusplus
}
#endif

#endif // GPAK_HELPER_H