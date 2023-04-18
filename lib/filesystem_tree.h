#ifndef FILESYSTEM_TREE_H
#define FILESYSTEM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"

	#include <stdio.h>

	struct filesystem_tree_file
	{
		char* name_;
		size_t offset_;
		size_t size_;
		size_t usize_;
		char* path_;
	};
	typedef struct filesystem_tree_file filesystem_tree_file_t;

	struct filesystem_tree_node
	{
		char* name_;
		struct filesystem_tree_node* parent_;
		struct filesystem_tree_node** children_;
		size_t num_children_;
		filesystem_tree_file_t** files_;
		size_t num_files_;
	};
	typedef struct filesystem_tree_node filesystem_tree_node_t;

	struct filesystem_iterator_state
	{
		filesystem_tree_node_t* node_;
		size_t child_index_;
		size_t file_index_;
	};
	typedef struct filesystem_iterator_state filesystem_iterator_state_t;

	struct filesystem_tree_iterator
	{
		filesystem_iterator_state_t* stack_;
		size_t stack_size_;
		size_t stack_capacity_;
	};
	typedef struct filesystem_tree_iterator filesystem_tree_iterator_t;

	// 
	GPAK_API filesystem_tree_node_t* filesystem_tree_create();
	GPAK_API void filesystem_tree_add_directory(filesystem_tree_node_t* _root, const char* _path);
	GPAK_API void filesystem_tree_add_file(filesystem_tree_node_t* _root, const char* _path, const char* _file_path, size_t _offset, size_t _size, size_t _usize);
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