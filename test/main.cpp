#ifdef __cplusplus
extern "C" {
#include "gpak.h"
#include "filesystem_tree.h"
}
#endif 

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#include <algorithm>
#include <string>
#include <iostream>
#include <queue>

void charSort(std::string& str)
{
    const std::string vowelsList{ 'a', 'e', 'i', 'o', 'u', 'y', 'A', 'E', 'I', 'O','U','Y' };
    std::queue<char> vowels;
    std::queue<char> consonants;

    for (size_t i = 0; i < str.size(); ++i)
    {
        if (vowelsList.find(str[i]) != std::string::npos)
        {
            vowels.push(str[i]);
        }
        else
        {
            consonants.push(str[i]);
        }
    }
    for (size_t i = 0; i < str.size(); ++i)
    {
        if (vowels.empty())
        {
            str[i] = consonants.front();
            consonants.pop();

        }
        else if (consonants.empty())
        {
            str[i] = vowels.front();
            vowels.pop();
        }
        else if (!(i % 2))
        {
            str[i] = vowels.front();
            vowels.pop();
        }
        else
        {
            str[i] = consonants.front();
            consonants.pop();
        }
    }
}


int main() 
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

    std::string letters{ "smSMfmSiowlsJSLFkkjhsoyyySjjfoaojfjKSJFWUlnjas" };
    std::sort(letters.begin(), letters.end(), [](const char a, const char b) { return static_cast<int>(a) > static_cast<int>(b); });
    std::cout << letters;

    std::string letters_{ "smSMfmSiowlsJSLFkkjhsoyyySjjfoaojfjKSJFWUlnjas" };
    charSort(letters_);
    std::cout << letters_;


    //filesystem_tree_node_t* root = filesystem_tree_create();
    //
    //filesystem_tree_add_file(root, "my_directory/bebra/bobra/file.ddt", 14);
    //filesystem_tree_add_file(root, "my_directory/my_file.txt", 25673);
    //filesystem_tree_add_file(root, "my_directory/my_file2.txt", 3091023);
    //filesystem_tree_add_file(root, "my_directory/amogra/my_file2.txt", 22222);
    //filesystem_tree_add_file(root, "my_directory/pidoras/gay.txt", 22222);
    //filesystem_tree_add_file(root, "aboba/file1.bat", 200);
    //filesystem_tree_add_file(root, "aboba/file2.exe", 1000);
    //
    //auto iterator = filesystem_iterator_create(root);
    //filesystem_tree_node_t* next_directory = NULL;
    //while ((next_directory = filesystem_iterator_next_directory(iterator))) 
    //{
    //    printf("Directory: %s\n", next_directory->name);
    //    filesystem_tree_file_t* next_file = NULL;
    //    while ((next_file = filesystem_iterator_next_file(iterator)))
    //        printf("  File: %s, size: %zu\n", next_file->name, next_file->size);
    //}
    //
    //filesystem_iterator_free(iterator);
    //
    //auto found_directory = filesystem_tree_find_directory(root, "my_directory/");
    //if (found_directory) 
    //{
    //    iterator = filesystem_iterator_create(found_directory);
    //
    //    while ((next_directory = filesystem_iterator_next_subling_directory(iterator)))
    //    {
    //        printf("Directory: %s\n", next_directory->name);
    //        filesystem_tree_file_t* next_file = NULL;
    //        while ((next_file = filesystem_iterator_next_file(iterator)))
    //            printf("  File: %s, size: %zu\n", next_file->name, next_file->size);
    //    }
    //
    //    filesystem_iterator_free(iterator);
    //
    //    printf("Directory found: %s\n", found_directory->name);
    //
    //    for (size_t i = 0; i < found_directory->num_files; ++i)
    //    {
    //        auto pFile = found_directory->files[i];
    //        printf("File: %s, size: %zu\n", pFile->name, pFile->size);
    //    }
    //}
    //else
    //    printf("Directory not found.\n");
    //
    //filesystem_tree_file_t* found_file = filesystem_tree_find_file(root, "my_directory/bebra/bobra/file.ddt");
    //if (found_file) {
    //    printf("File found: %s, size: %zu\n", found_file->name, found_file->size);
    //}
    //else {
    //    printf("File not found.\n");
    //}
    //
    //filesystem_tree_delete(root);
    _CrtDumpMemoryLeaks();

    return 0;
}