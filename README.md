# Files to Burn

The **Files to Burn** program is a Command Line Interface (CLI) tool written in C++ that helps you identify new files within a collection of photos/videos that haven't been archived onto DVD/Blu-ray yet. The program scans a folder recursively, compares files against a list of MD5 hashes of already burnt files, and outputs a list of new files that can be considered for archival.

## Table of Contents

- [Files to Burn](#files-to-burn)
  - [Table of Contents](#table-of-contents)
  - [Getting Started](#getting-started)
    - [Prerequisites](#prerequisites)
    - [Installation](#installation)
  - [Usage](#usage)
  - [Contributing](#contributing)
  - [License](#license)

## Getting Started

### Prerequisites

- CMake (version >= 3.12)
- A C++ compiler with C++11 support (e.g., GCC, Clang)
- OpenSSL library (for MD5 computation)

### Installation

1. Clone the repository:
   ```sh
   git clone https://github.com/your-username/files_to_burn.git
   cd files_to_burn
   ```

2. Build the project using CMake:
   ```sh
   mkdir build && cd build
   cmake ..
   make
   ```

3. Install the executable:
   ```sh
   sudo make install
   ```

## Usage

Once the program is installed, you can use it from the command line as follows:

```sh
files_to_burn <path_to_folder_containing_files_to_scan> --md5 <path_with_wildcards_to_md5_files>
```

- `<path_to_folder_containing_files_to_scan>`: The path to the folder containing the photos/videos you want to scan.
- `<path_with_wildcards_to_md5_files>`: The path with wildcards to a collection of MD5 files containing MD5 hashes of already burnt files.

The program will scan the specified folder, compute MD5 hashes for new files, and compare them against the provided MD5 files to identify new files that haven't been archived.

## Contributing

Contributions are welcome! If you find any issues or want to add new features, feel free to submit a pull request. Please make sure to follow the [Contributing Guidelines](CONTRIBUTING.md).

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.