#ifdef __cplusplus
extern "C" {
#include "gpak.h"
#include "filesystem_tree.h"
}
#endif 

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>
#include <string>

namespace fs = std::filesystem;

const fs::path _tests_entry{ "test_data" };
const fs::path _tests_out_entry{ "test_out" };
constexpr const size_t test_folder_count{ 5ull };
constexpr const size_t test_files_per_folder_count{ 10ull };
constexpr const size_t test_max_file_size{ 5ull * 1024ull * 1024ull };

size_t test_gpak_error_count{ 0ull };

void generate_random_file(const fs::path& path, size_t size) 
{
    std::ofstream file(path, std::ios::binary);
    if (!file)
        return;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int32_t> dist(0, 255);

    for (size_t i = 0; i < size; ++i)
        file.put((char)dist(gen));
    file.close();
}

void generate_test_file_tree(const fs::path _root, size_t _num_folders, size_t _num_files_per_folder, size_t _max_file_size)
{
    if (!fs::exists(_root))
        fs::create_directories(_root);
    else
    {
        fs::remove_all(_root);
        fs::create_directories(_root);
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> size_dist(0, _max_file_size);

    for (size_t i = 0; i < _num_folders; ++i)
    {
        fs::path folder = _root / ("folder" + std::to_string(i));
        fs::create_directory(folder);

        for (size_t j = 0; j < _num_files_per_folder; ++j)
        {
            fs::path file = folder / ("file" + std::to_string(j) + ".dat");
            size_t file_size = size_dist(gen);
            generate_random_file(file, file_size);
        }
    }
}

std::size_t number_of_files_in_directory(std::filesystem::path path)
{
    using std::filesystem::recursive_directory_iterator;
    using fp = bool (*)(const std::filesystem::path&);
    return std::count_if(recursive_directory_iterator(path), recursive_directory_iterator{}, (fp)std::filesystem::is_regular_file);
}


void error_handler(const char* filepath, int errcode, void* user_data)
{
    ++test_gpak_error_count;
}

void gpak_test_add_files(gpak_t* _pak, const fs::path& _fspath)
{
    for (auto& entry : fs::recursive_directory_iterator(_fspath))
    {
        auto _path = fs::relative(entry.path(), _fspath);
        auto _srfullpath = entry.path().string();
        auto _srpath = _path.string();
        std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

        if (entry.is_regular_file())
            gpak_add_file(_pak, _srfullpath.c_str(), _srpath.c_str());
    }
}

void gpak_test_extract_files(gpak_t* _pak, const fs::path& _fspath)
{
    auto pak_root = gpak_get_root(_pak);

    filesystem_tree_iterator_t* iterator = filesystem_iterator_create(pak_root);
    filesystem_tree_node_t* next_directory = pak_root;
    do
    {
        filesystem_tree_file_t* next_file = NULL;
        while ((next_file = filesystem_iterator_next_file(iterator)))
        {
            std::filesystem::path _first_entry(_fspath);
            char* internal_filepath = filesystem_tree_file_path(next_directory, next_file);
            _first_entry = std::filesystem::weakly_canonical(_first_entry / internal_filepath);

            if (!std::filesystem::exists(_first_entry.parent_path()))
                std::filesystem::create_directories(_first_entry.parent_path());

            auto* infile = gpak_fopen(_pak, internal_filepath);
            if (!infile)
            {
                ++test_gpak_error_count;
                continue;
            }

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

TEST(filesystem_tree_test, filesystem_tree_add_files) 
{
    size_t filescount{ 0ull };

    auto* _root = filesystem_tree_create();

    // Add entries to filesystem_tree
    for (auto& entry : std::filesystem::recursive_directory_iterator(_tests_entry))
    {
        auto _path = std::filesystem::relative(entry.path(), _tests_entry);
        auto _srfullpath = entry.path().string();
        auto _srpath = _path.string();
        std::replace(_srpath.begin(), _srpath.end(), '\\', '/');

        if (entry.is_regular_file())
        {
            pak_entry_t _entry;
            _entry.compressed_size_ = 0u;
            _entry.uncompressed_size_ = 0u;
            _entry.crc32_ = 0u;
            _entry.offset_ = 0ull;

            filesystem_tree_add_file(_root, _srpath.c_str(), NULL, _entry);
        }
    }

    // Iterate via filesystem_tree
    filesystem_tree_iterator_t* iterator = filesystem_iterator_create(_root);
    filesystem_tree_node_t* next_directory = _root;
    do
    {
        filesystem_tree_file_t* next_file = NULL;
        while ((next_file = filesystem_iterator_next_file(iterator)))
            ++filescount;
    } while ((next_directory = filesystem_iterator_next_directory(iterator)));

    filesystem_iterator_free(iterator);

    filesystem_tree_delete(_root);

    EXPECT_EQ(test_folder_count * test_files_per_folder_count, filescount);
}

TEST(gpak_test, gpak_compress_deflate)
{
    auto _archive_path = _tests_out_entry / "deflate.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_CREATE);

    gpak_set_error_handler(_pak, &error_handler);
    gpak_set_compression_algorithm(_pak, GPAK_HEADER_COMPRESSION_DEFLATE);
    gpak_set_compression_level(_pak, GPAK_COMPRESSION_DEFLATE_BEST);

    gpak_test_add_files(_pak, _tests_entry);

    gpak_close(_pak);

    EXPECT_EQ(test_gpak_error_count, 0ull);
}

TEST(gpak_test, gpak_decompress_inflate)
{
    auto _out_path = _tests_out_entry / "deflate";
    auto _archive_path = _tests_out_entry / "deflate.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_READ_ONLY);
    if (!_pak)
        ++test_gpak_error_count;

    gpak_set_error_handler(_pak, &error_handler);
    gpak_test_extract_files(_pak, _out_path);

    gpak_close(_pak);

    auto files_unpacked = number_of_files_in_directory(_out_path);

    EXPECT_TRUE(files_unpacked == test_folder_count * test_files_per_folder_count && test_gpak_error_count == 0ull);
}


TEST(gpak_test, gpak_compress_lz4)
{
    auto _archive_path = _tests_out_entry / "lz4.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_CREATE);

    gpak_set_error_handler(_pak, &error_handler);
    gpak_set_compression_algorithm(_pak, GPAK_HEADER_COMPRESSION_LZ4);
    gpak_set_compression_level(_pak, GPAK_COMPRESSION_LZ4_BEST);

    gpak_test_add_files(_pak, _tests_entry);

    gpak_close(_pak);

    EXPECT_EQ(test_gpak_error_count, 0ull);
}

TEST(gpak_test, gpak_decompress_lz4)
{
    auto _out_path = _tests_out_entry / "lz4";
    auto _archive_path = _tests_out_entry / "lz4.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_READ_ONLY);
    if (!_pak)
        ++test_gpak_error_count;

    gpak_set_error_handler(_pak, &error_handler);
    gpak_test_extract_files(_pak, _out_path);

    gpak_close(_pak);

    auto files_unpacked = number_of_files_in_directory(_out_path);

    EXPECT_TRUE(files_unpacked == test_folder_count * test_files_per_folder_count && test_gpak_error_count == 0ull);
}


TEST(gpak_test, gpak_compress_zstd)
{
    auto _archive_path = _tests_out_entry / "zstd.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_CREATE);

    gpak_set_error_handler(_pak, &error_handler);
    gpak_set_compression_algorithm(_pak, GPAK_HEADER_COMPRESSION_ZST);
    gpak_set_compression_level(_pak, GPAK_COMPRESSION_ZST_BEST);

    gpak_test_add_files(_pak, _tests_entry);

    gpak_close(_pak);

    EXPECT_EQ(test_gpak_error_count, 0ull);
}

TEST(gpak_test, gpak_decompress_zstd)
{
    auto _out_path = _tests_out_entry / "zstd";
    auto _archive_path = _tests_out_entry / "zstd.gpak";
    test_gpak_error_count = 0ull;

    auto* _pak = gpak_open(_archive_path.string().c_str(), GPAK_MODE_READ_ONLY);
    if (!_pak)
        ++test_gpak_error_count;

    gpak_set_error_handler(_pak, &error_handler);
    gpak_test_extract_files(_pak, _out_path);

    gpak_close(_pak);

    auto files_unpacked = number_of_files_in_directory(_out_path);

    EXPECT_TRUE(files_unpacked == test_folder_count * test_files_per_folder_count && test_gpak_error_count == 0ull);
}

int main(int argc, char** argv) 
{
    // Prepare test data
    generate_test_file_tree(_tests_entry, test_folder_count, test_files_per_folder_count, test_max_file_size);

    if (!fs::exists(_tests_out_entry))
        fs::create_directories(_tests_out_entry);

    ::testing::InitGoogleTest(&argc, argv);
    auto tests_result = RUN_ALL_TESTS();

    if (fs::exists(_tests_entry))
        fs::remove_all(_tests_entry);

    if (fs::exists(_tests_out_entry))
        fs::remove_all(_tests_out_entry);

    return tests_result;
}