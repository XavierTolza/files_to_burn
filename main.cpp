#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <fstream>
#include <thread>
#include <mutex>

namespace fs = std::filesystem;
typedef std::string path_t;
typedef std::string md5_t;
typedef std::vector<path_t> files_t;
typedef std::set<path_t> files_set_t;

// global mutex for stdout
std::mutex stdout_mutex;

typedef struct
{
    path_t folderPath;
    path_t md5Path;
    path_t ignoreFile;
    uint8_t numThreads;
    bool hidden;
} Arguments;

typedef struct
{
    md5_t md5;
    path_t filePath;
} md5_tuple_t;

void print_help()
{
    std::cout << "Usage: find_files [options]\n"
                 "Options:\n"
                 "  -f, --folder <path_to_folder_containing_files_to_scan>  Path to folder containing files to scan\n"
                 "  -m, --md5 <path_with_wildcards_to_md5_files>            Path with wildcards to md5 files\n"
                 "  -i, --ignore <path_to_file_containing_folders_to_ignore> Path to file containing folders to ignore\n"
                 "  -t, --threads <number_of_threads>                       Number of threads to use\n"
                 "  -H, --hidden                                             Scan hidden files\n"
                 "  -h, --help                                               Show this help message and exit\n";
}

Arguments parse_args(int argc, char *argv[])
{
    Arguments args;
    // initialize num thread to system number of cores
    args.numThreads = std::thread::hardware_concurrency();

    // Parse arguments using getopt
    const option longOpts[] = {
        {"folder", required_argument, nullptr, 'f'},
        {"md5", required_argument, nullptr, 'm'},
        {"help", no_argument, nullptr, 'h'},
        {"ignore", required_argument, nullptr, 'i'},
        {"threads", required_argument, nullptr, 't'},
        {"hidden", no_argument, nullptr, 'H'},
        {nullptr, 0, nullptr, 0}};
    // short options
    const char *const shortOpts = "f:m:hi:H";

    // Process arguments
    int opt = 0;
    while ((opt = getopt_long(argc, argv, shortOpts, longOpts, nullptr)) != -1)
    {
        switch (opt)
        {
        case 'f':
            args.folderPath = fs::path(optarg);
            break;
        case 'm':
            args.md5Path = fs::path(optarg);
            break;
        case 'h':
            print_help();
            exit(0);
            break;
        case 'i':
            args.ignoreFile = fs::path(optarg);
            break;
        case 't':
            args.numThreads = std::stoi(optarg);
            break;
        case 'H':
            args.hidden = true;
            break;
        default:
            break;
        }
    }

    return args;
}

md5_t computeMD5(const path_t &filePath)
{
    // Compute the md5 of a file
    md5_t result;
    std::string command = "md5sum \"" + filePath + "\"";
    FILE *pipe = popen(command.c_str(), "r");
    if (pipe)
    {
        // load all the output of the pipe into a std::string
        char buffer[128];
        while (!feof(pipe))
        {
            if (fgets(buffer, 128, pipe) != nullptr)
            {
                result += buffer;
            }
        }
        pclose(pipe);
        // cut to keep only the md5
        result = result.substr(0, 32);
    }
    else
    {
        std::cout << "Unable to open pipe" << std::endl;
        exit(1);
    }
    return result;
}

void splitmd5(std::vector<md5_tuple_t> &data, std::set<md5_t> &md5, files_set_t &files)
{
    // Split the result of computeMD5 into two sets
    for (auto &m : data)
    {
        md5.insert(m.md5);
        files.insert(m.filePath);
        // also add file with appended .xz extension
        files.insert(m.filePath + ".xz");
        // and with xz extension removed if any
        if (m.filePath.substr(m.filePath.size() - 3) == ".xz")
        {
            files.insert(m.filePath.substr(0, m.filePath.size() - 3));
        }
    }
}

std::vector<md5_tuple_t> loadMD5File(const path_t &md5FilePath)
{
    // Load md5 file
    std::vector<md5_tuple_t> result;
    std::ifstream file(md5FilePath);
    std::string line;
    while (std::getline(file, line))
    {
        if (line.size() > 0)
        {
            md5_tuple_t m;
            m.md5 = line.substr(0, 32);
            m.filePath = line.substr(34);
            // remove trailing "./" if present
            if (m.filePath.substr(0, 2) == "./")
            {
                m.filePath = m.filePath.substr(2);
            }
            result.push_back(m);
        }
    }
    return result;
}

files_t findFiles(const path_t &folderPath, bool hidden)
{
    // Recursively search for files in the specified folder
    files_t result;
    for (const auto &entry : fs::recursive_directory_iterator(folderPath))
    {
        // if the file starts by "." then it is hidden
        bool is_hidden = entry.path().filename().string()[0] == '.';
        if (is_hidden && !hidden)
        {
            continue;
        }
        else if (entry.is_regular_file())
        {
            // make relative from folderPath
            auto relativePath = fs::relative(entry.path(), folderPath);
            result.push_back(relativePath);
        }
    }
    return result;
}

void checkFile(path_t &file, std::set<md5_t> &md5, files_set_t &burnt, files_set_t &ignored, path_t &rootFolder)
{
    // check if the file is ignored: if any of ignore string is the begenning of any file, ignore it
    for (auto &i : ignored)
    {
        if (file.substr(0, i.size()) == i)
        {
#ifdef DEBUG
            std::cout << "Ignored: " << file << std::endl;
#endif
            return;
        }
    }

    // check if the file is burnt
    if (burnt.find(file) != burnt.end())
    {
#ifdef DEBUG
        std::cout << "Burnt: " << file << std::endl;
#endif
        return;
    }
    // compute the md5 of the file
    auto abspath = fs::path(rootFolder) / file;
    auto md5File = computeMD5(abspath.string());

    // check if the md5 is already present
    if (md5.find(md5File) != md5.end())
    {
#ifdef DEBUG
        std::cout << "Ignored by md5: " << file << std::endl;
#endif
        return;
    }
    // the file is valid, print it
    // but before,acquire a lock to protect the output
    std::lock_guard<std::mutex> lock(stdout_mutex);
    std::cout << file << std::endl;
}

void check_files(files_t &files, std::set<md5_t> &md5, files_set_t &burnt, files_set_t &ignored, path_t &rootFolder, uint8_t numThreads)
{
    // check files by spreading the work on threads
    // define a thread vector
    std::vector<std::thread> threads;
    // lambda function calling checkFile
    auto threadFunction = [&files, &md5, &burnt, &ignored, &rootFolder](int start, int end, int stride)
    {
        // check each file in the range [start, end[ with a given stride
        for (int i = start; i < end; i += stride)
        {
            checkFile(files[i], md5, burnt, ignored, rootFolder);
        }
    };

    // start N threads they all start to i with a stride of N
    for (int i = 0; i < numThreads; i++)
    {
        threads.push_back(std::thread(threadFunction, i, files.size(), numThreads));
    }

    // join threads
    for (auto &t : threads)
    {
        t.join();
    }
}

int main(int argc, char *argv[])
{
    // parse args
    Arguments args = parse_args(argc, argv);

    // load md5 files
    std::vector<md5_tuple_t> burnt = loadMD5File(args.md5Path);

    // Decompose into a set of md5s and a set of files
    std::set<md5_t> burnt_md5;
    files_set_t burnt_files;
    splitmd5(burnt, burnt_md5, burnt_files);

    std::cerr << "Loaded " << burnt.size() << " md5s" << std::endl;

    // list files
    files_t files = findFiles(args.folderPath, args.hidden);

    std::cerr << "Found " << files.size() << " files" << std::endl;

    // load a file containing paths to folders to ignore
    files_set_t ignore;
    if (args.ignoreFile != "")
    {
        std::ifstream file(args.ignoreFile);
        std::string line;
        while (std::getline(file, line))
        {
            if (line.size() > 0)
            {
                ignore.insert(line);
            }
        }
    }

    std::cerr << "Loaded " << ignore.size() << " ignore rules" << std::endl;
    std::cerr << "Processing files" << std::endl;


    // process files
    check_files(files, burnt_md5, burnt_files, ignore, args.folderPath, args.numThreads);
    return 0;
}
