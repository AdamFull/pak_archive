#include "filesystem_tree.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

filesystem_tree_node_t* _create_node(const char* _name, filesystem_tree_node_t* _parent)
{
    filesystem_tree_node_t* node = (filesystem_tree_node_t*)malloc(sizeof(filesystem_tree_node_t));
    node->name_ = strdup(_name);
    node->parent_ = _parent;
    node->children_ = NULL;
    node->num_children_ = 0ull;
    node->files_ = NULL;
    node->num_files_ = 0ull;
    return node;
}

filesystem_tree_node_t* _add_directory(filesystem_tree_node_t* _root, const char* _dir_name) 
{
    filesystem_tree_node_t* new_node = _create_node(_dir_name, _root);
    _root->children_ = (filesystem_tree_node_t**)realloc(_root->children_, sizeof(filesystem_tree_node_t*) * (_root->num_children_ + 1));
    _root->children_[_root->num_children_++] = new_node;
    return new_node;
}

filesystem_tree_file_t* _create_file(const char* _name, const char* _file_path, size_t _offset, size_t _size)
{
    filesystem_tree_file_t* file = (filesystem_tree_file_t*)malloc(sizeof(filesystem_tree_file_t));
    file->name_ = strdup(_name);
    file->offset_ = _offset;
    file->size_ = _size;
    file->path_ = strdup(_file_path);

    return file;
}

filesystem_tree_file_t* _add_file(filesystem_tree_node_t* _directory, const char* _file_name, const char* _file_path, size_t _offset, size_t _size)
{
    filesystem_tree_file_t* new_file = _create_file(_file_name, _file_path, _offset, _size);
    _directory->files_ = (filesystem_tree_file_t**)realloc(_directory->files_, sizeof(filesystem_tree_file_t*) * (_directory->num_files_ + 1));
    _directory->files_[_directory->num_files_++] = new_file;
    return new_file;
}

filesystem_tree_node_t* _find_directory(filesystem_tree_node_t* _root, const char* _path) 
{
    if (!_root || !_path || !*_path)
        return NULL;

    if (strcmp(_root->name_, _path) == 0)
        return _root;

    for (size_t i = 0; i < _root->num_children_; i++) 
    {
        filesystem_tree_node_t* found = _find_directory(_root->children_[i], _path);
        if (found)
            return found;
    }

    return NULL;
}

filesystem_tree_file_t* _find_file(filesystem_tree_node_t* _directory, const char* _file_name) 
{
    if (!_directory || !_file_name || !*_file_name)
        return NULL;

    for (size_t i = 0; i < _directory->num_files_; i++) 
    {
        if (strcmp(_directory->files_[i]->name_, _file_name) == 0)
            return _directory->files_[i];
    }

    return NULL;
}

void _free_file(filesystem_tree_file_t* _file)
{
    if (_file) 
    {
        free(_file->path_);
        free(_file->name_);
        free(_file);
    }
}

void _free_node(filesystem_tree_node_t* _node)
{
    if (_node) 
    {
        for (size_t i = 0; i < _node->num_children_; i++)
            _free_node(_node->children_[i]);

        for (size_t i = 0; i < _node->num_files_; i++)
            _free_file(_node->files_[i]);

        free(_node->name_);
        free(_node->children_);
        free(_node->files_);
        free(_node);
    }
}


filesystem_tree_node_t* filesystem_tree_create()
{
    return _create_node("", NULL);
}

void filesystem_tree_add_directory(filesystem_tree_node_t* _root, const char* _path) 
{
    if (!_root || !_path || !*_path)
        return;

    char* dir_path = strdup(_path);
    char* dir_name = strtok(dir_path, "/");

    filesystem_tree_node_t* current = _root;
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

void filesystem_tree_add_file(filesystem_tree_node_t* _root, const char* _path, const char* _file_path, size_t _offset, size_t _size)
{
    if (!_root || !_path || !*_path)
        return;

    char* dir_path = strdup(_path);
    char* dir_name = strtok(dir_path, "/");
    char* file_name = NULL;

    filesystem_tree_node_t* current = _root;
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
        _add_file(current, dir_name, _file_path, _offset, _size);

    free(dir_path);
}

filesystem_tree_node_t* filesystem_tree_find_directory(filesystem_tree_node_t* _root, const char* _path) 
{
    if (!_root || !_path || !*_path)
        return NULL;

    char* dir_path = strdup(_path);
    char* dir_name = strtok(dir_path, "/");

    filesystem_tree_node_t* current = _root;
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

filesystem_tree_file_t* filesystem_tree_find_file(filesystem_tree_node_t* _root, const char* _path) {
    if (!_root || !_path || !*_path)
        return NULL;

    char* dir_path = strdup(_path);
    char* dir_name = strtok(dir_path, "/");
    char* file_name = NULL;

    filesystem_tree_node_t* current = _root;
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
            break;
    }

    filesystem_tree_file_t* file = NULL;
    if (dir_name && !file_name)
        file = _find_file(current, dir_name);

    free(dir_path);
    return file;
}

char* filesystem_tree_directory_path(filesystem_tree_node_t* _node)
{
    if (!_node || !_node->parent_)
        return strdup("");

    char* parent_path = filesystem_tree_directory_path(_node->parent_);
    size_t path_length = strlen(parent_path) + strlen(_node->name_) + 2;
    char* path = (char*)malloc(path_length * sizeof(char));

    snprintf(path, path_length, "%s%s/", parent_path, _node->name_);

    free(parent_path);
    return path;
}

char* filesystem_tree_file_path(filesystem_tree_node_t* _node, filesystem_tree_file_t* _file)
{
    if (!_node || !_file)
        return NULL;

    char* dir_path = filesystem_tree_directory_path(_node);
    size_t path_length = strlen(dir_path) + strlen(_file->name_) + 1;
    char* file_path = (char*)malloc(path_length * sizeof(char));

    snprintf(file_path, path_length, "%s%s", dir_path, _file->name_);

    free(dir_path);
    return file_path;
}

void filesystem_tree_delete(filesystem_tree_node_t* _root) 
{
    if (_root)
        _free_node(_root);
}


static void _iterator_stack_push(filesystem_tree_iterator_t* _iterator, filesystem_iterator_state_t _state)
{
    if (_iterator->stack_size_ >= _iterator->stack_capacity_)
    {
        _iterator->stack_capacity_ *= 2;
        _iterator->stack_ = (filesystem_iterator_state_t*)realloc(_iterator->stack_, sizeof(filesystem_iterator_state_t) * _iterator->stack_capacity_);
    }

    _iterator->stack_[_iterator->stack_size_++] = _state;
}

static filesystem_iterator_state_t _iterator_stack_pop(filesystem_tree_iterator_t* _iterator)
{
    if (_iterator->stack_size_ > 0)
        return _iterator->stack_[--_iterator->stack_size_];

    filesystem_iterator_state_t empty_state = { .node_ = NULL, .child_index_ = 0ull, .file_index_ = 0ull };
    return empty_state;
}

filesystem_tree_iterator_t* filesystem_iterator_create(filesystem_tree_node_t* _root)
{
    if (!_root)
        return NULL;

    filesystem_tree_iterator_t* iterator = (filesystem_tree_iterator_t*)malloc(sizeof(filesystem_tree_iterator_t));
    iterator->stack_capacity_ = 16ull;
    iterator->stack_size_ = 0ull;
    iterator->stack_ = (filesystem_iterator_state_t*)malloc(sizeof(filesystem_iterator_state_t) * iterator->stack_capacity_);

    filesystem_iterator_state_t initial_state = { .node_ = _root, .child_index_ = 0ull, .file_index_ = 0ull };
    iterator->stack_[iterator->stack_size_++] = initial_state;

    return iterator;
}

filesystem_tree_node_t* filesystem_iterator_next_directory(filesystem_tree_iterator_t* _iterator)
{
    if (_iterator->stack_size_ == 0)
        return NULL;

    filesystem_iterator_state_t current_state = _iterator->stack_[_iterator->stack_size_ - 1];

    if (current_state.child_index_ < current_state.node_->num_children_)
    {
        filesystem_tree_node_t* next_directory = current_state.node_->children_[current_state.child_index_];
        filesystem_iterator_state_t child_state = { .node_ = next_directory, .child_index_ = 0, .file_index_ = 0 };

        // Update the current state's child_index
        current_state.child_index_++;
        _iterator->stack_[_iterator->stack_size_ - 1] = current_state;

        // Push the child_state onto the stack
        _iterator_stack_push(_iterator, child_state);

        return next_directory;
    }
    else

    // If there are no more children, remove the current state from the stack
    _iterator_stack_pop(_iterator);

    // Call the function recursively to find the next directory
    return filesystem_iterator_next_directory(_iterator);
}

filesystem_tree_node_t* filesystem_iterator_next_subling_directory(filesystem_tree_iterator_t* _iterator)
{
    if (_iterator->stack_size_ == 0)
        return NULL;

    filesystem_iterator_state_t current_state = _iterator->stack_[_iterator->stack_size_ - 1];

    if (current_state.child_index_ < current_state.node_->num_children_)
    {
        filesystem_tree_node_t* next_directory = current_state.node_->children_[current_state.child_index_];

        // Update the current state's child_index
        current_state.child_index_++;
        _iterator->stack_[_iterator->stack_size_ - 1] = current_state;

        return next_directory;
    }

    return NULL;
}

filesystem_tree_file_t* filesystem_iterator_next_file(filesystem_tree_iterator_t* _iterator)
{
    if (_iterator->stack_size_ == 0)
        return NULL;

    filesystem_iterator_state_t current_state = _iterator->stack_[_iterator->stack_size_ - 1];

    if (current_state.file_index_ < current_state.node_->num_files_)
    {
        filesystem_tree_file_t* next_file = current_state.node_->files_[current_state.file_index_];
        current_state.file_index_++;
        _iterator->stack_[_iterator->stack_size_ - 1] = current_state;
        return next_file;
    }

    return NULL;
}

void filesystem_iterator_free(filesystem_tree_iterator_t* _iterator)
{
    if (_iterator)
    {
        free(_iterator->stack_);
        free(_iterator);
    }
}