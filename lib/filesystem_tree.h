#ifndef FILESYSTEM_TREE_H
#define FILESYSTEM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	// 
	GPAK_API filesystem_tree_node_t* filesystem_tree_create();
	GPAK_API void filesystem_tree_add_directory(filesystem_tree_node_t* _root, const char* _path);
	GPAK_API void filesystem_tree_add_file(filesystem_tree_node_t* _root, const char* _path, const char* _file_path, pak_entry_t _entry);
	GPAK_API filesystem_tree_node_t* filesystem_tree_find_directory(filesystem_tree_node_t* _root, const char* _path);
	GPAK_API filesystem_tree_file_t* filesystem_tree_find_file(filesystem_tree_node_t* _root, const char* _path);
	GPAK_API char* filesystem_tree_directory_path(filesystem_tree_node_t* _node);
	GPAK_API char* filesystem_tree_file_path(filesystem_tree_node_t* _node, filesystem_tree_file_t* _file);
	GPAK_API void filesystem_tree_delete(filesystem_tree_node_t* _root);

	// Recursive iterator
	GPAK_API filesystem_tree_iterator_t* filesystem_iterator_create(filesystem_tree_node_t* _root);
	GPAK_API filesystem_tree_node_t* filesystem_iterator_next_directory(filesystem_tree_iterator_t* _iterator);
	GPAK_API filesystem_tree_node_t* filesystem_iterator_next_subling_directory(filesystem_tree_iterator_t* _iterator);
	GPAK_API filesystem_tree_file_t* filesystem_iterator_next_file(filesystem_tree_iterator_t* _iterator);
	GPAK_API void filesystem_iterator_free(filesystem_tree_iterator_t* _iterator);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_TREE_H