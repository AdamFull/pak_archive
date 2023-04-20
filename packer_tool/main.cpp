#ifdef __cplusplus
extern "C" {
#include "gpak.h"
#include "filesystem_tree.h"
}
#endif 

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define USE_DEFLATE
//#define USE_LZ4
//#define USE_ZST

class InputParser {
public:
	InputParser(int argc, char** argv) : tokens(argv + 1, argv + argc) {}

	std::optional<std::string> get(const std::string& option) const {
		auto itr = std::find(tokens.begin(), tokens.end(), option);
		if (itr != tokens.end() && ++itr != tokens.end())
			return *itr;
		return std::nullopt;
	}

	bool exists(const std::string& option) const {
		return std::find(tokens.begin(), tokens.end(), option) != tokens.end();
	}

private:
	std::vector<std::string> tokens;
};

struct FTaskParams
{
	int mode;
	int compression_mode;
	char compression_level;
	std::string srSource;
	std::string srDestination;
	std::string srPassword;
};

void on_exit()
{
	_CrtDumpMemoryLeaks();
}

void error_handler(int errcode, void* user_data)
{
	std::cerr << "Error: " << errcode << std::endl;
}

void progress_handler(size_t done, size_t total, int32_t mode, void* user_data)
{
	std::cout << (mode == 0 ? "COMPRESSING" : "DECOMPRESSING") << ": " << done << "/" << total << std::endl;
}

void compress(const std::string& srSource, const std::string& srDestination)
{
	auto* _pak = gpak_open(srDestination.c_str(), GPAK_MODE_CREATE);
	gpak_set_error_handler(_pak, &error_handler);
	gpak_set_process_handler(_pak, &progress_handler);

	std::filesystem::path _first_entry{ srSource };
	for (auto& entry : std::filesystem::recursive_directory_iterator(_first_entry))
	{
		auto _path = std::filesystem::relative(entry.path(), _first_entry);
		auto _srfullpath = entry.path().string();
		auto _srpath = _path.string();
		std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

		if (entry.is_regular_file())
			gpak_add_file(_pak, _srfullpath.c_str(), _srpath.c_str());
	}

	gpak_close(_pak);
}

void decompress(const std::string& srSource, const std::string& srDestination)
{

}

int main(int argc, char** argv)
{
	atexit(on_exit);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	gpak_t* _pak{ nullptr };

	FTaskParams params;
	InputParser args(argc, argv);

	if (args.exists("-h") || args.exists("-help") || argc < 2)
	{
		std::cout
			<< "[-pack] - run in packer mode.\n"
			<< "[-unpack] - run in unpacker mode.\n"
			<< "[-i] - input file/files path.\n"
			<< "[-o] - output file/files path.\n"
			<< "[-p] - encryption password.";
		return 0;
	}

	if (args.exists("-src"))
		params.srSource = args.get("-src").value();

	if (args.exists("-dst"))
		params.srDestination = args.get("-dst").value();

	// Algo selection
	if (args.exists("-algorythm"))
	{
		auto alg = args.get("-algorythm").value();
		if (alg == "deflate")
			params.compression_mode = GPAK_HEADER_COMPRESSION_DEFLATE;
		else if(alg == "lz4")
			params.compression_mode = GPAK_HEADER_COMPRESSION_LZ4;
		else if (alg == "zst")
			params.compression_mode = GPAK_HEADER_COMPRESSION_ZST;
	}
	else
		params.compression_mode = GPAK_HEADER_COMPRESSION_NONE;

	// Compression
	if (args.exists("-compression"))
		params.compression_level = std::stoi(args.get("-compression").value());

	if (args.exists("-pack"))
	{
		params.mode = GPAK_MODE_CREATE;
		_pak = gpak_open(params.srDestination.c_str(), params.mode);
	}
	else if (args.exists("-unpack"))
	{
		params.mode = GPAK_MODE_READ_ONLY;
		_pak = gpak_open(params.srSource.c_str(), params.mode);
	}
	else if (args.exists("-update"))
	{
		params.mode = GPAK_MODE_UPDATE;
		_pak = gpak_open(params.srDestination.c_str(), params.mode);
	}
	else
		throw std::runtime_error("");

	gpak_set_error_handler(_pak, &error_handler);
	gpak_set_process_handler(_pak, &progress_handler);

	if (params.mode == GPAK_MODE_CREATE || params.mode == GPAK_MODE_UPDATE)
	{
		gpak_set_compression_algorithm(_pak, params.compression_mode);
		gpak_set_compression_level(_pak, params.compression_level);

		std::filesystem::path _first_entry{ params.srSource };
		for (auto& entry : std::filesystem::recursive_directory_iterator(_first_entry))
		{
			auto _path = std::filesystem::relative(entry.path(), _first_entry);
			auto _srfullpath = entry.path().string();
			auto _srpath = _path.string();
			std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

			if (entry.is_regular_file())
				gpak_add_file(_pak, _srfullpath.c_str(), _srpath.c_str());
		}
	}
	else if (params.mode == GPAK_MODE_READ_ONLY)
	{
		auto pak_root = gpak_get_root(_pak);
		filesystem_tree_iterator_t* iterator = filesystem_iterator_create(pak_root);
		filesystem_tree_node_t* next_directory = pak_root;
		do
		{
			filesystem_tree_file_t* next_file = NULL;
			while ((next_file = filesystem_iterator_next_file(iterator)))
			{
				std::filesystem::path _first_entry = params.srSource;
				char* internal_filepath = filesystem_tree_file_path(next_directory, next_file);
				_first_entry = std::filesystem::weakly_canonical(_first_entry / internal_filepath);

				if (!std::filesystem::exists(_first_entry.parent_path()))
					std::filesystem::create_directories(_first_entry);

				auto* infile = gpak_fopen(_pak, internal_filepath);

				std::ofstream outfile(_first_entry, std::ios_base::out | std::ios_base::binary);

				char buffer[2048];
				size_t readed{ 0ull }; size_t whole_readed{ 0ull };
				do {
					readed = gpak_fread(buffer, 1ull, 2048ull, infile);
					outfile.write(buffer, readed);
					whole_readed += readed;
				} while (readed != 0ull);

				outfile.close();
				gpak_fclose(infile);
				free(internal_filepath);
			}
		} while ((next_directory = filesystem_iterator_next_directory(iterator)));

		filesystem_iterator_free(iterator);
	}

	gpak_close(_pak);

	return 0;
}