#ifndef GPAK_LIB_H
#define GPAK_LIB_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	GPAK_API gpak_t* gpak_open(const char* _path, int _mode);
	GPAK_API int gpak_close(gpak_t* _pak);

	GPAK_API void gpak_set_user_data(gpak_t* _pak, void* _user_data);
	GPAK_API void gpak_set_error_handler(gpak_t* _pak, gpak_error_handler_t _error_handler);
	GPAK_API void gpak_set_process_handler(gpak_t* _pak, gpak_progress_handler_t _progress_handler);

	GPAK_API void gpak_set_compression_algorithm(gpak_t* _pak, int _algorithm);
	GPAK_API void gpak_set_compression_level(gpak_t* _pak, int _level);
	GPAK_API void gpak_set_encryption_mode(gpak_t* _pak, int _mode);
	GPAK_API void gpak_set_encryption_password(gpak_t* _pak, const char* _password);

	GPAK_API int gpak_add_directory(gpak_t* _pak, const char* _internal_path);
	GPAK_API int gpak_add_file(gpak_t* _pak, const char* _external_path, const char* _internal_path);

	GPAK_API struct filesystem_tree_node* gpak_get_root(gpak_t* _pak);
	GPAK_API struct filesystem_tree_node* gpak_find_directory(gpak_t* _pak, const char* _path);
	GPAK_API struct filesystem_tree_file* gpak_find_file(gpak_t* _pak, const char* _path);

	// File functions
	GPAK_API gpak_file_t* gpak_fopen(gpak_t* _pak, const char* _path);
	GPAK_API int gpak_fgetc(gpak_file_t* _file);
	GPAK_API char* gpak_fgets(gpak_file_t* _file, char* _buffer, int _max);
	GPAK_API int gpak_ungetc(gpak_file_t* _file, int _character);
	GPAK_API size_t gpak_fread(void* _buffer, size_t _elemSize, size_t _elemCount, gpak_file_t* _file);
	GPAK_API long gpak_ftell(gpak_file_t* _file);
	GPAK_API long gpak_fseek(gpak_file_t* _file, long _offset, int _origin);
	GPAK_API long gpak_feof(gpak_file_t* _file);
	GPAK_API void gpak_fclose(gpak_file_t* _file);

#ifdef __cplusplus
}
#endif

#endif // GPAK_LIB_H