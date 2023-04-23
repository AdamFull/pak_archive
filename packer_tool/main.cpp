#ifdef __cplusplus
extern "C" {
#include "gpak.h"
#include "filesystem_tree.h"
}
#endif 

#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <optional>
#include <string>
#include <format>

#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define PACKER_VERSION "v1.0"

size_t current_file_count{ 0ull };
size_t total_file_count{ 0ull };

class InputParser {
public:
	InputParser(int argc, char** argv) : tokens(argv + 1, argv + argc) {}
	InputParser(int argc, const char** argv) : tokens(argv + 1, argv + argc) {}

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

std::size_t number_of_files_in_directory(std::filesystem::path path)
{
	using std::filesystem::recursive_directory_iterator;
	using fp = bool (*)(const std::filesystem::path&);
	return std::count_if(recursive_directory_iterator(path), recursive_directory_iterator{}, (fp)std::filesystem::is_regular_file);
}

void pad_info_strings(std::string& info1, std::string& info2) {
	if (info1.length() > info2.length())
		info2 += std::string(info1.length() - info2.length(), ' ');
	else if (info2.length() > info1.length())
		info1 += std::string(info2.length() - info1.length(), ' ');
}

std::string show_progress_bar(int progress, int total, const std::string& info = "", int bar_width = 50, char fill = '#', char empty = '-') 
{
	int filled = static_cast<int>(bar_width * static_cast<float>(progress) / total);
	int unfilled = bar_width - filled;

	std::string filled_part(filled, fill);
	std::string unfilled_part(unfilled, empty);

	return std::format("{}[{}{}] {:.2f}%", info, filled_part, unfilled_part, (static_cast<float>(progress) / total) * 100.0f);
}

void on_exit()
{
	_CrtDumpMemoryLeaks();
}

void error_handler(const char* filepath, int errcode, void* user_data)
{
	std::cerr << "Error in file " << filepath << " with code: " << errcode << std::endl;
}

void progress_handler(const char* filepath, size_t done, size_t total, int32_t mode, void* user_data)
{
	auto work_mode = std::string((mode == 0 ? "Compressing" : "Decompressing"));
	auto first_info = std::string("Whole progress:");
	auto second_info = std::string("Progress:");
	auto status_file_str = std::format("{} file: ", work_mode);

	pad_info_strings(status_file_str, first_info);
	pad_info_strings(status_file_str, second_info);

	std::cout << std::format("\033[sGPAK Tool {}. \n", PACKER_VERSION);

	std::cout << std::format("\033[2K{}\n", show_progress_bar(current_file_count, total_file_count, first_info));

	std::cout << std::format("\033[2K{}{}\n", status_file_str, filepath);

	std::cout << std::format("\033[2K{}", show_progress_bar(done, total, second_info));

	std::cout << "\033[u";
	std::cout.flush();

	if(total == done)
		++current_file_count;
}

int main(int argc, char** argv)
{
	atexit(on_exit);

	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	gpak_t* _pak{ nullptr };
	//InputParser args(argc, argv);

	const char* debug[] = { "packer_tool.exe", "-pack", "-src", "tmp/common", "-dst", "common.gpak", "-alg", "zst", "-lvl", "22" };
	InputParser args(10, debug);

	//const char* debug[] = { "packer_tool.exe", "-unpack", "-src", "common.gpak", "-dst", "unpakced/common" };
	//InputParser args(6, debug);

	FTaskParams params;
	

	//if (args.exists("-h") || args.exists("-help") || argc < 2)
	//{
	//	std::cout
	//		<< "[-pack] - run application in packing mode.\n"
	//		<< "[-unpack] - run application in unpacking mode.\n"
	//		<< "[-src] - In the packing mode, you need to pass the path to the folder that you want to pack.\n"
	//		<< "In the unpacking mode, you need to specify the path to the archive packed with the same packer.\n"
	//		<< "[-dst] - In the packing mode, you need to pass the path to the archive packed by the same packer.\n"
	//		<< "In the unpacking mode, you need to specify the path to the folder into which you want to unpack.\n"
	//		<< "[-alg] - Choice of compression algorithm. It can be deflate, lz4 or zst.\n"
	//		<< "[-lvl] - For deflate and lz4 algorithms the maximum compression is 9, for zst the maximum compression is 22.\n";
	//	return 0;
	//}

	if (args.exists("-src"))
		params.srSource = args.get("-src").value();

	if (args.exists("-dst"))
		params.srDestination = args.get("-dst").value();

	// Algo selection
	if (args.exists("-alg"))
	{
		auto alg = args.get("-alg").value();
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
	if (args.exists("-lvl"))
		params.compression_level = std::stoi(args.get("-lvl").value());

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
		if (params.mode == GPAK_MODE_CREATE)
		{
			gpak_set_compression_algorithm(_pak, params.compression_mode);
			gpak_set_compression_level(_pak, params.compression_level);
		}

		std::filesystem::path _first_entry{ params.srSource };

		total_file_count = number_of_files_in_directory(_first_entry);

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

		total_file_count = _pak->header_.entry_count_;

		filesystem_tree_iterator_t* iterator = filesystem_iterator_create(pak_root);
		filesystem_tree_node_t* next_directory = pak_root;
		do
		{
			filesystem_tree_file_t* next_file = NULL;
			while ((next_file = filesystem_iterator_next_file(iterator)))
			{
				std::filesystem::path _first_entry(params.srDestination);
				char* internal_filepath = filesystem_tree_file_path(next_directory, next_file);
				_first_entry = std::filesystem::weakly_canonical(_first_entry / internal_filepath);

				if (!std::filesystem::exists(_first_entry.parent_path()))
					std::filesystem::create_directories(_first_entry.parent_path());

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