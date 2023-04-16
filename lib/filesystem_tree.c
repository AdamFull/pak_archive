#include "filesystem_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

filesystem_tree_node_t* _create_node(const char* name, filesystem_tree_node_t* parent)
{
    filesystem_tree_node_t* node = (filesystem_tree_node_t*)malloc(sizeof(filesystem_tree_node_t));
    node->name = strdup(name);
    node->parent = parent;
    node->children = NULL;
    node->num_children = 0;
    node->files = NULL;
    node->num_files = 0;
    return node;
}

filesystem_tree_node_t* _add_directory(filesystem_tree_node_t* root, const char* dir_name) 
{
    filesystem_tree_node_t* new_node = _create_node(dir_name, root);
    root->children = (filesystem_tree_node_t**)realloc(root->children, sizeof(filesystem_tree_node_t*) * (root->num_children + 1));
    root->children[root->num_children++] = new_node;
    return new_node;
}

filesystem_tree_file_t* _create_file(const char* name, size_t size) 
{
    filesystem_tree_file_t* file = (filesystem_tree_file_t*)malloc(sizeof(filesystem_tree_file_t));
    file->name = strdup(name);
    file->size = size;
    return file;
}

filesystem_tree_file_t* _add_file(filesystem_tree_node_t* directory, const char* file_name, size_t size) 
{
    filesystem_tree_file_t* new_file = _create_file(file_name, size);
    directory->files = (filesystem_tree_file_t**)realloc(directory->files, sizeof(filesystem_tree_file_t*) * (directory->num_files + 1));
    directory->files[directory->num_files++] = new_file;
    return new_file;
}

filesystem_tree_node_t* _find_directory(filesystem_tree_node_t* root, const char* path) 
{
    if (!path || !*path)
        return NULL;

    if (strcmp(root->name, path) == 0)
        return root;

    for (size_t i = 0; i < root->num_children; i++) 
    {
        filesystem_tree_node_t* found = _find_directory(root->children[i], path);
        if (found)
            return found;
    }

    return NULL;
}

filesystem_tree_file_t* _find_file(filesystem_tree_node_t* directory, const char* file_name) 
{
    if (!directory || !file_name || !*file_name)
        return NULL;

    for (size_t i = 0; i < directory->num_files; i++) 
    {
        if (strcmp(directory->files[i]->name, file_name) == 0)
            return directory->files[i];
    }

    return NULL;
}

void _free_file(filesystem_tree_file_t* file)
{
    if (file) 
    {
        free(file->name);
        free(file);
    }
}

void _free_node(filesystem_tree_node_t* node)
{
    if (node) 
    {
        for (size_t i = 0; i < node->num_children; i++)
            _free_node(node->children[i]);

        for (size_t i = 0; i < node->num_files; i++)
            _free_file(node->files[i]);

        free(node->name);
        free(node->children);
        free(node->files);
        free(node);
    }
}


filesystem_tree_node_t* filesystem_tree_create()
{
    return _create_node("", NULL);
}

void filesystem_tree_add_directory(filesystem_tree_node_t* root, const char* path) 
{
    if (!path || !*path)
        return;

    char* dir_path = strdup(path);
    char* dir_name = strtok(dir_path, "/");

    filesystem_tree_node_t* current = root;
    while (dir_name) 
    {
        filesystem_tree_node_t* found = _find_directory(current, dir_name);
        if (!found)
            found = _add_directory(current, dir_name);
 
        current = found;
        dir_name = strtok(NULL, "/");
    }

    free(dir_path);
}

void filesystem_tree_add_file(filesystem_tree_node_t* root, const char* path, size_t size) 
{
    if (!path || !*path)
        return;

    char* dir_path = strdup(path);
    char* dir_name = strtok(dir_path, "/");
    char* file_name = NULL;

    filesystem_tree_node_t* current = root;
    while (dir_name) 
    {
        file_name = strtok(NULL, "/");
        if (file_name) 
        {
            filesystem_tree_node_t* found = _find_directory(current, dir_name);
            if (!found)
                found = _add_directory(current, dir_name);

            current = found;
            dir_name = file_name;
        }
        else
            break;
    }

    if (dir_name && !file_name)
        _add_file(current, dir_name, size);

    free(dir_path);
}

filesystem_tree_node_t* filesystem_tree_find_directory(filesystem_tree_node_t* root, const char* path) 
{
    if (!path || !*path)
        return NULL;

    char* dir_path = strdup(path);
    char* dir_name = strtok(dir_path, "/");

    filesystem_tree_node_t* current = root;
    while (dir_name) 
    {
        current = _find_directory(current, dir_name);
        if (!current)
            break;
        dir_name = strtok(NULL, "/");
    }

    free(dir_path);
    return current;
}

filesystem_tree_file_t* filesystem_tree_find_file(filesystem_tree_node_t* root, const char* path) {
    if (!path || !*path)
        return NULL;

    char* dir_path = strdup(path);
    char* dir_name = strtok(dir_path, "/");
    char* file_name = NULL;

    filesystem_tree_node_t* current = root;
    while (dir_name) 
    {
        file_name = strtok(NULL, "/");
        if (file_name) 
        {
            filesystem_tree_node_t* found = _find_directory(current, dir_name);
            if (!found) 
            {
                free(dir_path);
                return NULL;
            }

            current = found;
            dir_name = strtok(NULL, "/");
        }
        else
            file_name = dir_name;
    }

    filesystem_tree_file_t* file = NULL;
    if (file_name)
        file = _find_file(current, file_name);

    free(dir_path);
    return file;
}

void filesystem_tree_delete(filesystem_tree_node_t* root) 
{
    if (root)
        _free_node(root);
}


static void _iterator_stack_push(filesystem_tree_iterator_t* iterator, filesystem_iterator_state_t state)
{
    if (iterator->stack_size >= iterator->stack_capacity)
    {
        iterator->stack_capacity *= 2;
        iterator->stack = (filesystem_iterator_state_t*)realloc(iterator->stack, sizeof(filesystem_iterator_state_t) * iterator->stack_capacity);
    }

    iterator->stack[iterator->stack_size++] = state;
}

static filesystem_iterator_state_t _iterator_stack_pop(filesystem_tree_iterator_t* iterator)
{
    if (iterator->stack_size > 0)
        return iterator->stack[--iterator->stack_size];

    filesystem_iterator_state_t empty_state = { .node = NULL, .child_index = 0ull, .file_index = 0ull };
    return empty_state;
}

filesystem_tree_iterator_t* filesystem_iterator_create(filesystem_tree_node_t* root)
{
    if (!root)
        return NULL;

    filesystem_tree_iterator_t* iterator = (filesystem_tree_iterator_t*)malloc(sizeof(filesystem_tree_iterator_t));
    iterator->stack_capacity = 16ull;
    iterator->stack_size = 0ull;
    iterator->stack = (filesystem_iterator_state_t*)malloc(sizeof(filesystem_iterator_state_t) * iterator->stack_capacity);

    filesystem_iterator_state_t initial_state = { .node = root, .child_index = 0ull, .file_index = 0ull };
    iterator->stack[iterator->stack_size++] = initial_state;

    return iterator;
}

filesystem_tree_node_t* filesystem_iterator_next_directory(filesystem_tree_iterator_t* iterator)
{
    if (iterator->stack_size == 0)
        return NULL;

    filesystem_iterator_state_t current_state = iterator->stack[iterator->stack_size - 1];

    if (current_state.child_index < current_state.node->num_children)
    {
        filesystem_tree_node_t* next_directory = current_state.node->children[current_state.child_index];
        filesystem_iterator_state_t child_state = { .node = next_directory, .child_index = 0, .file_index = 0 };

        // Update the current state's child_index
        current_state.child_index++;
        iterator->stack[iterator->stack_size - 1] = current_state;

        // Push the child_state onto the stack
        _iterator_stack_push(iterator, child_state);

        return next_directory;
    }
    else

    // If there are no more children, remove the current state from the stack
    _iterator_stack_pop(iterator);

    // Call the function recursively to find the next directory
    return filesystem_iterator_next_directory(iterator);
}

filesystem_tree_node_t* filesystem_iterator_next_subling_directory(filesystem_tree_iterator_t* iterator)
{
    if (iterator->stack_size == 0)
        return NULL;

    filesystem_iterator_state_t current_state = iterator->stack[iterator->stack_size - 1];

    if (current_state.child_index < current_state.node->num_children)
    {
        filesystem_tree_node_t* next_directory = current_state.node->children[current_state.child_index];

        // Update the current state's child_index
        current_state.child_index++;
        iterator->stack[iterator->stack_size - 1] = current_state;

        return next_directory;
    }

    return NULL;
}

filesystem_tree_file_t* filesystem_iterator_next_file(filesystem_tree_iterator_t* iterator)
{
    if (iterator->stack_size == 0)
        return NULL;

    filesystem_iterator_state_t current_state = iterator->stack[iterator->stack_size - 1];

    if (current_state.file_index < current_state.node->num_files)
    {
        filesystem_tree_file_t* next_file = current_state.node->files[current_state.file_index];
        current_state.file_index++;
        iterator->stack[iterator->stack_size - 1] = current_state;
        return next_file;
    }

    return NULL;
}

void filesystem_iterator_free(filesystem_tree_iterator_t* iterator)
{
    if (iterator)
    {
        free(iterator->stack);
        free(iterator);
    }
}