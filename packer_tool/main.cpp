#ifdef __cplusplus
extern "C" {
#include "pak.h"
}
#endif 

#include <filesystem>

int main()
{
	auto* pPak = pak_archive_open("test.pak", PAK_ARCHIVE_MODE_CREATE);
	
	std::filesystem::path _first_entry{ "E:\\media" };
	for (auto& entry : std::filesystem::recursive_directory_iterator(_first_entry))
	{
		auto _path = std::filesystem::relative(entry.path(), _first_entry);
		auto _srfullpath = entry.path().string();
		auto _srpath = _path.string();
		std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

		if (entry.is_directory())
			pak_archive_add_directory(pPak, _srpath.c_str());
		else if (entry.is_regular_file())
			pak_archive_add_file(pPak, _srfullpath.c_str(), _srpath.c_str());
	}

	pak_archive_close(pPak);


	pPak = pak_archive_open("test.pak", PAK_ARCHIVE_MODE_READ_ONLY);


	pak_archive_close(pPak);


	return 0;
}