/**
 * @file filesystem_tree.h
 * @author AdamFull
 * @date 23.04.2023
 * @brief The filesystem_tree.h header file contains methods and data structures
 *        for working with a filesystem tree used in the Gpak library for
 *        managing game archives.
 *
 * This header file defines the data structures and functions for representing
 * and manipulating a filesystem tree used to store files and directories
 * within a Gpak game archive. It provides an interface for iterating over the
 * directories and files stored in the archive, allowing for efficient and
 * organized access to game assets.
 *
 * The filesystem tree consists of nodes representing directories and files
 * within the game archive. Each node can have zero or more child nodes,
 * representing the contents of a directory. File nodes store the file metadata
 * and can be used to access the file data stored within the archive.
 *
 * To use the filesystem tree functions provided by this header, include it
 * alongside the main gpak.h header file and link against the Gpak library in
 * your project.
 */

#ifndef FILESYSTEM_TREE_H
#define FILESYSTEM_TREE_H

#ifdef __cplusplus
extern "C" {
#endif

	#include "gpak_export.h"
	#include "gpak_data.h"

	/**
	 * @brief Creates a new filesystem tree node.
	 *
	 * This function initializes and returns a new filesystem tree node,
	 * which is the root of the filesystem tree.
	 *
	 * @return A pointer to the created filesystem_tree_node_t.
	 */
	GPAK_API filesystem_tree_node_t* filesystem_tree_create();

	/**
	 * @brief Adds a directory to the filesystem tree.
	 *
	 * This function creates a new directory node under the given _root node.
	 *
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 * @param _path The path of the directory to add.
	 */
	GPAK_API void filesystem_tree_add_directory(filesystem_tree_node_t* _root, const char* _path);

	/**
	 * @brief Adds a file to the filesystem tree.
	 *
	 * This function creates a new file node under the given _root node.
	 *
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 * @param _path The path of the directory where the file will be added.
	 * @param _file_path The file path of the file to add.
	 * @param _entry The pak_entry_t associated with the file.
	 */
	GPAK_API void filesystem_tree_add_file(filesystem_tree_node_t* _root, const char* _path, const char* _file_path, pak_entry_t _entry);

	/**
	 * @brief Finds a directory in the filesystem tree.
	 *
	 * This function searches for a directory node with the specified path
	 * in the filesystem tree.
	 *
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 * @param _path The path of the directory to find.
	 * @return A pointer to the found directory node or NULL if not found.
	 */
	GPAK_API filesystem_tree_node_t* filesystem_tree_find_directory(filesystem_tree_node_t* _root, const char* _path);

	/**
	 * @brief Finds a file in the filesystem tree.
	 *
	 * This function searches for a file node with the specified path
	 * in the filesystem tree.
	 *
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 * @param _path The path of the file to find.
	 * @return A pointer to the found file node or NULL if not found.
	 */
	GPAK_API filesystem_tree_file_t* filesystem_tree_find_file(filesystem_tree_node_t* _root, const char* _path);

	/**
	 * @brief Gets the path of a directory node.
	 *
	 * This function retrieves the path of the specified directory node.
	 *
	 * @param _node A pointer to the filesystem_tree_node_t.
	 * @return A string containing the path of the directory.
	 */
	GPAK_API char* filesystem_tree_directory_path(filesystem_tree_node_t* _node);

	/**
	 * @brief Gets the path of a file node.
	 *
	 * This function retrieves the path of the specified file node.
	 *
	 * @param _node A pointer to the filesystem_tree_node_t.
	 * @param _file A pointer to the filesystem_tree_file_t.
	 * @return A string containing the path of the file.
	 */
	GPAK_API char* filesystem_tree_file_path(filesystem_tree_node_t* _node, filesystem_tree_file_t* _file);

	/**
	 * @brief Deletes the filesystem tree.
	 *
	 * This function recursively deletes
	 * the filesystem tree and frees all associated memory.
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 */
	GPAK_API void filesystem_tree_delete(filesystem_tree_node_t* _root);

	
	/**
	 * @brief Creates a new filesystem tree iterator.
	 *
	 * This function initializes and returns a new filesystem tree iterator
	 * for traversing the filesystem tree starting from the given _root node.
	 *
	 * @param _root A pointer to the root filesystem_tree_node_t.
	 * @return A pointer to the created filesystem_tree_iterator_t.
	 */
	GPAK_API filesystem_tree_iterator_t* filesystem_iterator_create(filesystem_tree_node_t* _root);

	/**
	 * @brief Retrieves the next directory in the filesystem tree.
	 *
	 * This function returns the next directory node in the filesystem tree
	 * using a depth-first search algorithm.
	 *
	 * @param _iterator A pointer to the filesystem_tree_iterator_t.
	 * @return A pointer to the next directory node or NULL if there are no more directories.
	 */
	GPAK_API filesystem_tree_node_t* filesystem_iterator_next_directory(filesystem_tree_iterator_t* _iterator);

	/**
	 * @brief Retrieves the next sibling directory in the filesystem tree.
	 *
	 * This function returns the next sibling directory node in the filesystem tree
	 * without traversing its children.
	 *
	 * @param _iterator A pointer to the filesystem_tree_iterator_t.
	 * @return A pointer to the next sibling directory node or NULL if there are no more sibling directories.
	 */
	GPAK_API filesystem_tree_node_t* filesystem_iterator_next_subling_directory(filesystem_tree_iterator_t* _iterator);

	/**
	 * @brief Retrieves the next file in the filesystem tree.
	 *
	 * This function returns the next file node in the filesystem tree
	 * using a depth-first search algorithm.
	 *
	 * @param _iterator A pointer to the filesystem_tree_iterator_t.
	 * @return A pointer to the next file node or NULL if there are no more files.
	 */
	GPAK_API filesystem_tree_file_t* filesystem_iterator_next_file(filesystem_tree_iterator_t* _iterator);

	/**
	 * @brief Frees the memory associated with the filesystem tree iterator.
	 *
	 * This function deallocates the memory used by the filesystem tree iterator.
	 *
	 * @param _iterator A pointer to the filesystem_tree_iterator_t.
	 */
	GPAK_API void filesystem_iterator_free(filesystem_tree_iterator_t* _iterator);

#ifdef __cplusplus
}
#endif

#endif // FILESYSTEM_TREE_H