#ifdef __cplusplus
extern "C" {
#include "pak.h"
#include "filesystem_tree.h"
}
#endif 

#include <filesystem>
#include <fstream>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

void on_exit()
{
	_CrtDumpMemoryLeaks();
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	atexit(on_exit);

	auto* pPak = gpak_open("test.pak", GPAK_MODE_CREATE);
	
	std::filesystem::path _first_entry{ "E:\\database\\oniilus" };
	for (auto& entry : std::filesystem::recursive_directory_iterator(_first_entry))
	{
		auto _path = std::filesystem::relative(entry.path(), _first_entry);
		auto _srfullpath = entry.path().string();
		auto _srpath = _path.string();
		std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

		if (entry.is_regular_file())
			gpak_add_file(pPak, _srfullpath.c_str(), _srpath.c_str());
	}

	gpak_close(pPak);


	pPak = gpak_open("test.pak", GPAK_MODE_READ_ONLY);
	
	auto pak_root = gpak_get_root(pPak);
	filesystem_tree_iterator_t* iterator = filesystem_iterator_create(pak_root);
	filesystem_tree_node_t* next_directory = pak_root;
	do
	{
		filesystem_tree_file_t* next_file = NULL;
		while ((next_file = filesystem_iterator_next_file(iterator)))
		{
			_first_entry = "E:\\database\\test";
			char* internal_filepath = filesystem_tree_file_path(next_directory, next_file);
			_first_entry = std::filesystem::weakly_canonical(_first_entry / internal_filepath);

			if (!std::filesystem::exists(_first_entry.parent_path()))
				std::filesystem::create_directories(_first_entry);

			auto* infile = gpak_fopen(pPak, internal_filepath);

			std::ofstream outfile(_first_entry);

			//char buffer[2048];
			//size_t readed{ 0ull }; size_t whole_readed{ 0ull };
			//do {
			//	readed = gpak_fread(buffer, 1ull, 2048ull, infile);
			//	outfile.write(buffer, readed);
			//	whole_readed += readed;
			//} while (readed != 0ull);

			outfile.write(infile->data_, infile->end_ - infile->begin_);

			outfile.close();
			gpak_fclose(infile);
			free(internal_filepath);
			// get file pointer
			// write to file
		}
	} while ((next_directory = filesystem_iterator_next_directory(iterator)));

	filesystem_iterator_free(iterator);
	
	gpak_close(pPak);

	return 0;
}