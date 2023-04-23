# GPAK - Game Pack Archive Library
GPAK is a C library for creating, reading, and manipulating game archive files. It provides an API for working with game assets, making it easy to manage large collections of files within a game project.

[![CMake Build and Test](https://github.com/adamfull/pak_archive/actions/workflows/build_test.yml/badge.svg)](https://github.com/adamfull/pak_archive/actions/workflows/build_test.yml)

## Features
- Supports multiple compression algorithms (None, Deflate, LZ4, ZSTD)
- Easy-to-use API for creating, opening, and modifying archives
- Stream-based file access for efficient memory usage
- CLI tool for packing and unpacking archives

## Building the Project
The project uses CMake as the build system. To build the library and all subprojects, follow these steps:

1. Navigate to the project root directory.
2. Create a build directory: mkdir build && cd build
3. Run CMake to generate the build files: cmake ..
4. Build the project: cmake --build .

## Example Usage
### Creating an archive and adding files
```c
#include "gpak.h"

int main() {
gpak_t *pak = gpak_create("example.gpak", GPAK_MODE_CREATE);
gpak_set_compression_algorithm(pak, GPAK_HEADER_COMPRESSION_ZST);
gpak_set_compression_level(pak, 22);

gpak_add_file(pak, "external/path/to/file/file1.txt", "internal/path/to/file/file1.txt");
gpak_add_file(pak, "external/path/to/file/file2.txt", "internal/path/to/file/file2.txt");
gpak_close(pak);
return 0;
}
```

### Opening an archive and reading a file
```c
#include "gpak.h"

int main() {
gpak_t *pak = gpak_open("example.gpak", GPAK_MODE_READ_ONLY);
gpak_file_t *file = gpak_fopen(pak, "file1.txt");
if (file) {
char buffer[256];
while (!gpak_feof(file)) {
gpak_fgets(file, buffer, sizeof(buffer));
printf("%s", buffer);
}
gpak_fclose(file);
}
gpak_close(pak);
return 0;
}
```
## Documentation
You can find documentation here https://adamfull.github.io/pak_archive/

## Packer Tool
A command-line interface (CLI) tool for packing and unpacking GPAK archives is included in the packer_tool directory. After building the project, the tool can be found in the build directory as packer_tool.

### Usage
- To pack an archive: ./packer_tool -pack -src input_directory -dst output.gpak -alg zst -lvl 22
- To unpack an archive: ./packer_tool -unpack -src output.gpak -dst output_directory

##### Options
- -pack: Run packer_tool in packing mode to create a new archive.
- -unpack: Run packer_tool in unpacking mode to extract the contents of an existing archive.
- -src: Path to the source file or directory, depending on the mode.
- -dst: Path to the destination file or directory, depending on the mode.
- -alg: Choose the compression algorithm: deflate, lz4, or zst.
- -lvl: Choose the compression level. For deflate, the maximum level is 9. For lz4, the maximum level is 12. For zst, the maximum level is 22.

## License
This project is licensed under the MIT License.
