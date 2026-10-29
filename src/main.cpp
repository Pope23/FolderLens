#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>
#include <utility>

namespace fs = std::filesystem;

struct FileInfo {
    fs::path path;
    std::string extension;
    std::uintmax_t bytes = 0;
};

struct ExtensionStats {
    std::size_t count = 0;
    std::uintmax_t bytes = 0;
};

struct ScanResult {
    std::vector<FileInfo> files;
    std::map<std::string, ExtensionStats> extensions;
    std::size_t directories = 0;
    std::size_t errors = 0;
    std::uintmax_t total_bytes = 0;
};

struct Options {
    fs::path folder = fs::current_path();
    std::size_t top = 10;
    fs::path csv_path;
    bool export_csv = false;
};

static void print_help(const char* program) {
    std::cout << "FolderLens - read-only folder analyzer (C++17)\n\n"
        << "Usage:\n  " << program
        << " [FOLDER] [--top NUMBER] [--csv FILE]\n"
        << "  " << program << " --menu\n\n"
        << "Examples:\n  " << program << " demo_data --top 5\n"
        << "  " << program << " \"C:\\\\Users\\\\Public\\\\Documents\" --csv report.csv\n\n"
        << "Options:\n"
        << "  --top NUMBER    Number of largest files (default: 10)\n"
        << "  --csv FILE      Export all scanned files to a CSV report\n"
        << "  --menu          Start interactive mode\n"
        << "  --help          Show this help\n";
}

static bool parse_options(int argc, char* argv[], Options& options) {
    bool folder_set = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            print_help(argv[0]);
            std::exit(0);
        }
        if (argument == "--top") {
            if (++i >= argc) {
                std::cerr << "Error: --top requires a positive integer.\n";
                return false;
            }
            try {
                const std::string input = argv[i];
                std::size_t consumed = 0;
                const auto number = std::stoull(input, &consumed);
                if (consumed != input.size() || number == 0 || number > 1000) {
                    throw std::out_of_range("range");
                }
                options.top = static_cast<std::size_t>(number);
            }
            catch (const std::exception&) {
                std::cerr << "Error: --top must be between 1 and 1000.\n";
                return false;
            }
        }
        else if (argument == "--csv") {
            if (++i >= argc) {
                std::cerr << "Error: --csv requires an output filename.\n";
                return false;
            }
            options.csv_path = argv[i];
            options.export_csv = true;
        }
        else if (argument.rfind("--", 0) == 0) {
            std::cerr << "Error: unknown option: " << argument << "\n";
            return false;
        }
        else if (!folder_set) {
            options.folder = argument;
            folder_set = true;
        }
        else {
            std::cerr << "Error: provide only one folder path.\n";
            return false;
        }
    }
    return true;
}

static std::string extension_of(const fs::path& path) {
    std::string extension = path.extension().string();
    if (extension.empty()) {
        return "[no extension]";
    }
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension;
}

static std::string human_size(std::uintmax_t bytes) {
    constexpr const char* units[] = { "B", "KiB", "MiB", "GiB", "TiB", "PiB" };
    double size = static_cast<double>(bytes);
    std::size_t unit = 0;
    while (size >= 1024.0 && unit < 5) {
        size /= 1024.0;
        ++unit;
    }
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(unit == 0 ? 0 : 2)
        << size << " " << units[unit];
    return stream.str();
}

static ScanResult scan_folder(const fs::path& root) {
    ScanResult result;
    std::error_code error;
    fs::recursive_directory_iterator iterator(
        root, fs::directory_options::skip_permission_denied, error);
    const fs::recursive_directory_iterator end;
    if (error) {
        ++result.errors;
        return result;
    }

    while (iterator != end) {
        const fs::directory_entry& entry = *iterator;
        error.clear();
        if (entry.is_directory(error)) {
            ++result.directories;
        }
        else if (!error && entry.is_regular_file(error)) {
            const auto file_size = entry.file_size(error);
            if (!error) {
                FileInfo info{ entry.path(), extension_of(entry.path()), file_size };
                result.total_bytes += file_size;
                auto& group = result.extensions[info.extension];
                ++group.count;
                group.bytes += file_size;
                result.files.push_back(std::move(info));
            }
        }
        if (error) {
            ++result.errors;
        }
        error.clear();
        iterator.increment(error);
        if (error) {
            ++result.errors;
        }
    }
    return result;
}
static std::string csv_field(const std::string& value) {
    std::string safe = value;
    if (!safe.empty() && (safe.front() == '=' || safe.front() == '+' ||
        safe.front() == '-' || safe.front() == '@' ||
        safe.front() == '\t' || safe.front() == '\r')) {
        safe.insert(safe.begin(), '\'');
    }
    std::string quoted = "\"";
    for (char character : safe) {
        if (character == '"') {
            quoted.push_back('"');
        }
        quoted.push_back(character);
    }
    quoted.push_back('"');
    return quoted;
}

static bool write_csv(const fs::path& destination, const ScanResult& result) {
    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output) {
        std::cerr << "Error: cannot write CSV file: " << destination.string() << "\n";
        return false;
    }
    output << "path,extension,size_bytes\r\n";
    for (const FileInfo& file : result.files) {
        output << csv_field(file.path.string()) << ','
            << csv_field(file.extension) << ',' << file.bytes << "\r\n";
    }
    output.flush();
    if (!output) {
        std::cerr << "Error: failed to complete CSV export.\n";
        return false;
    }
    return true;
}

static void display_summary(const fs::path& root, const ScanResult& result) {
    std::cout << "\n============================================================\n"
        << "  FOLDERLENS | SMART FILE ANALYZER\n"
        << "============================================================\n"
        << "Folder       : " << root.string() << '\n'
        << "Files        : " << result.files.size() << '\n'
        << "Subfolders   : " << result.directories << '\n'
        << "Total size   : " << human_size(result.total_bytes) << '\n'
        << "Read errors  : " << result.errors << '\n';
}

static void display_extensions(const ScanResult& result) {
    std::vector<std::pair<std::string, ExtensionStats>> groups(
        result.extensions.begin(), result.extensions.end());
    std::sort(groups.begin(), groups.end(), [](const auto& a, const auto& b) {
        return a.second.bytes != b.second.bytes
            ? a.second.bytes > b.second.bytes : a.first < b.first;
        });
    std::cout << "\n--- FILE TYPES (by disk usage) ---\n";
    std::cout << std::left << std::setw(20) << "Extension"
        << std::right << std::setw(10) << "Files"
        << std::setw(16) << "Size" << '\n';
    std::cout << std::string(46, '-') << '\n';
    for (const auto& group : groups) {
        std::cout << std::left << std::setw(20) << group.first
            << std::right << std::setw(10) << group.second.count
            << std::setw(16) << human_size(group.second.bytes) << '\n';
    }
    if (groups.empty()) {
        std::cout << "(No regular files found.)\n";
    }
}

static void display_top_files(const fs::path& root, const ScanResult& result,
    std::size_t top) {
    std::vector<FileInfo> top_files = result.files;
    const std::size_t count = std::min(top, top_files.size());
    const auto compare = [](const FileInfo& a, const FileInfo& b) {
        return a.bytes != b.bytes ? a.bytes > b.bytes
            : a.path.string() < b.path.string();
        };
    if (count < top_files.size()) {
        std::partial_sort(top_files.begin(), top_files.begin() + count,
            top_files.end(), compare);
    }
    else {
        std::sort(top_files.begin(), top_files.end(), compare);
    }
    std::cout << "\n--- TOP " << count << " LARGEST FILES ---\n";
    for (std::size_t i = 0; i < count; ++i) {
        const FileInfo& file = top_files[i];
        std::error_code ec;
        fs::path shown = fs::relative(file.path, root, ec);
        if (ec) {
            shown = file.path;
        }
        std::cout << std::setw(3) << (i + 1) << "  "
            << std::right << std::setw(12) << human_size(file.bytes)
            << "   " << shown.string() << '\n';
    }
    if (count == 0) {
        std::cout << "(No regular files found.)\n";
    }
}

static void display_report(const fs::path& root, const ScanResult& result,
    std::size_t top) {
    display_summary(root, result);
    display_extensions(result);
    display_top_files(root, result, top);
    std::cout << "\nRead-only scan complete. No files were modified.\n";
}

static bool resolve_directory(const fs::path& input, fs::path& resolved) {
    std::error_code error;
    if (!fs::is_directory(input, error) || error) {
        std::cerr << "Error: folder does not exist or is inaccessible: "
            << input.string() << '\n';
        return false;
    }
    resolved = fs::absolute(input, error);
    if (error) {
        std::cerr << "Error: cannot resolve folder path.\n";
        return false;
    }
    return true;
}

static std::string strip_surrounding_quotes(std::string value) {
    const auto first = value.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    const auto last = value.find_last_not_of(" \t");
    value = value.substr(first, last - first + 1);
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

static bool parse_top_count(const std::string& input, std::size_t& count) {
    try {
        std::size_t consumed = 0;
        const auto number = std::stoull(input, &consumed);
        if (consumed != input.size() || number < 1 || number > 1000) {
            return false;
        }
        count = static_cast<std::size_t>(number);
        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

static int interactive_menu() {
    ScanResult current_result;
    fs::path current_root;
    bool has_scan = false;
    std::size_t top = 10;

    for (;;) {
        std::cout << "\n=======================================\n"
            << "          FOLDERLENS v1.1\n"
            << "          Smart File Analyzer\n"
            << "=======================================\n"
            << "Current folder: "
            << (has_scan ? current_root.string() : "(none selected)") << '\n'
            << "\n1. Analyze a folder\n"
            << "2. Show largest files\n"
            << "3. Statistics by extension\n"
            << "4. Export CSV report\n"
            << "5. Exit\n"
            << "\nChoose an option: " << std::flush;

        std::string selection;
        if (!std::getline(std::cin, selection)) {
            std::cout << "\nGoodbye!\n";
            return EXIT_SUCCESS;
        }

        if (selection == "5" || selection == "q" || selection == "Q") {
            std::cout << "Goodbye!\n";
            return EXIT_SUCCESS;
        }
        if (selection == "1") {
            std::cout << "Enter folder path: " << std::flush;
            std::string input;
            if (!std::getline(std::cin, input)) return EXIT_SUCCESS;
            input = strip_surrounding_quotes(input);
            if (input.empty()) {
                std::cout << "No folder selected.\n";
                continue;
            }
            fs::path resolved;
            if (!resolve_directory(fs::path(input), resolved)) {
                continue;
            }
            std::cout << "Scanning, please wait...\n";
            ScanResult next_result = scan_folder(resolved);
            current_result = std::move(next_result);
            current_root = std::move(resolved);
            has_scan = true;
            display_summary(current_root, current_result);
            std::cout << "Select 2 or 3 to explore the results.\n";
            continue;
        }
        if (selection == "2" || selection == "3" || selection == "4") {
            if (!has_scan) {
                std::cout << "Please choose 1 to analyze a folder first.\n";
                continue;
            }
            if (selection == "2") {
                std::cout << "How many files (1-1000, Enter for 10)? " << std::flush;
                std::string input;
                if (!std::getline(std::cin, input)) return EXIT_SUCCESS;
                if (!input.empty() && !parse_top_count(input, top)) {
                    std::cout << "Invalid number. Enter a value from 1 to 1000.\n";
                    continue;
                }
                if (input.empty()) top = 10;
                display_top_files(current_root, current_result, top);
            }
            else if (selection == "3") {
                display_extensions(current_result);
            }
            else {
                std::cout << "CSV filename (Enter for folderlens_report.csv): "
                    << std::flush;
                std::string output;
                if (!std::getline(std::cin, output)) return EXIT_SUCCESS;
                output = strip_surrounding_quotes(output);
                if (output.empty()) output = "folderlens_report.csv";
                if (write_csv(fs::path(output), current_result)) {
                    std::error_code ec;
                    const fs::path where = fs::absolute(fs::path(output), ec);
                    std::cout << "CSV exported: "
                        << (ec ? output : where.string()) << '\n';
                }
            }
            continue;
        }
        std::cout << "Invalid option. Choose 1, 2, 3, 4 or 5.\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--menu")) {
            return interactive_menu();
        }

        Options options;
        if (!parse_options(argc, argv, options)) {
            std::cerr << "Run with --help for usage.\n";
            return EXIT_FAILURE;
        }

        fs::path root;
        if (!resolve_directory(options.folder, root)) {
            return EXIT_FAILURE;
        }
        const ScanResult result = scan_folder(root);
        display_report(root, result, options.top);
        if (options.export_csv) {
            if (!write_csv(options.csv_path, result)) {
                return EXIT_FAILURE;
            }
            std::cout << "CSV exported: " << options.csv_path.string() << '\n';
        }
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception) {
        std::cerr << "Unexpected error: " << exception.what() << '\n';
        return EXIT_FAILURE;
    }
}
