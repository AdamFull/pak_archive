#ifndef FILESYSTEM_TREE_H
#define FILESYSTEM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "pak_export.h"

#include <stdio.h>

struct filesystem_tree_file;
struct filesystem_tree_node;
struct filesystem_iterator_state;
struct filesystem_tree_iterator;

typedef struct filesystem_tree_file filesystem_tree_file_t;
typedef struct filesystem_tree_node filesystem_tree_node_t;
typedef struct filesystem_iterator_state filesystem_iterator_state_t;
typedef struct filesystem_tree_iterator filesystem_tree_iterator_t;

struct filesystem_tree_file 
{
    char* name;
    size_t size;
};

struct filesystem_tree_node 
{
    char* name;
    struct filesystem_tree_node* parent;
    struct filesystem_tree_node** children;
    size_t num_children;
    filesystem_tree_file_t** files;
    size_t num_files;
};

struct filesystem_iterator_state
{
    filesystem_tree_node_t* node;
    size_t child_index;
    size_t file_index;
};

struct filesystem_tree_iterator
{
    filesystem_iterator_state_t* stack;
    size_t stack_size;
    size_t stack_capacity;
};

// 
filesystem_tree_node_t* filesystem_tree_create();
void filesystem_tree_add_directory(filesystem_tree_node_t* root, const char* path);
void filesystem_tree_add_file(filesystem_tree_node_t* root, const char* path, size_t size);
filesystem_tree_node_t* filesystem_tree_find_directory(filesystem_tree_node_t* root, const char* path);
filesystem_tree_file_t* filesystem_tree_find_file(filesystem_tree_node_t* root, const char* path);
void filesystem_tree_delete(filesystem_tree_node_t* root);

// Recursive iterator
filesystem_tree_iterator_t* filesystem_iterator_create(filesystem_tree_node_t* root);
filesystem_tree_node_t* filesystem_iterator_next_directory(filesystem_tree_iterator_t* iterator);
filesystem_tree_node_t* filesystem_iterator_next_subling_directory(filesystem_tree_iterator_t* iterator);
filesystem_tree_file_t* filesystem_iterator_next_file(filesystem_tree_iterator_t* iterator);
void filesystem_iterator_free(filesystem_tree_iterator_t* iterator);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_TREE_H