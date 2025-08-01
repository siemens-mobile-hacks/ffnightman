#include "extractor.h"
#include "help.h"

#include <ffshit/system.h>
#include <ffshit/filesystem/platform/builder.h>
#include <ffshit/filesystem/ex.h>

#include "log/log.h"

#include <iostream>
#include <algorithm>
#include <regex>

// на BSD это конечно же никто не проверял

#if defined( __linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    #include <utime.h>
#elif defined(_WIN64)
    #include <windows.h>
#else
    #error Unsupported operating system
#endif

static size_t broken_name_counter = 0;

Extractor::Extractor(FULLFLASH::Partitions::Partitions::Ptr partitions, FULLFLASH::Platform::Type platform, const CLI::Options &options) :
    partitions(partitions) {
    
    if (!partitions) {
        throw FULLFLASH::Exception("partitions == nullptr o_O");
    }

    filesystem = FULLFLASH::Filesystem::build(platform, partitions);

    filesystem->log_verbose_processing(options.verbose_processing);
    filesystem->log_verbose_headers(options.verbose_headers);
    filesystem->log_verbose_data(options.verbose_data);

    if (filesystem) {
        filesystem->load(options.is_skip_broken, options.is_skip_dup, options.parts_to_extract);
    } else {
        throw FULLFLASH::Exception("fs == nullptr o_O");
    }
}
void Extractor::list(std::string regexp) {
    std::string list_msg = "Listing filesystem";

    std::regex  r;
    bool        is_regex = false;

    if (regexp.length()) {
        r = std::regex(regexp);

        is_regex = true;

        list_msg += fmt::format(". Regexp: '{}'", regexp);
    }

    spdlog::info("{}", list_msg);

    Log::flush_wait();

    const auto root = filesystem->get_root();

    for (const auto &disk_root : root->get_subdirs()) {
        const std::string &fs_name = disk_root->get_name();

        list_path(disk_root, fs_name, is_regex, r);
    }
}

static std::string format_attributes(const FULLFLASH::Filesystem::Attributes &attr) {
    std::string str_attrs;

    str_attrs += attr.is_readonly() ? "r" : "w";
    str_attrs += attr.is_hidden() ? "h" : "-";
    str_attrs += attr.is_system() ? "s" : "-";

    return str_attrs;
}

static std::string format_date_time(const FULLFLASH::Filesystem::TimePoint &time_point) {
    auto t = std::chrono::system_clock::to_time_t(time_point);

    std::stringstream ss;

    ss << std::put_time(std::localtime(&t), "%Y %b %d %H:%M");

    return ss.str();
}

void Extractor::list_path(FULLFLASH::Filesystem::Directory::Ptr dir, std::string full_path, bool is_regex, std::regex &regexp) {
    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::string file_name = file->get_name();

        std::string str_attributes  = format_attributes(file->get_attributes());
        std::string str_date        = format_date_time(file->get_timestamp());
        std::string str_path        = fmt::format("{}/{}", full_path, file_name);

        if (is_regex) {
            if (std::regex_search(str_path, regexp)) {
                fmt::print("{} {} {:3d} {}\n", str_attributes, str_date, 1, str_path);
            }
        } else {
            fmt::print("{} {} {:3d} {}\n", str_attributes, str_date, 1, str_path);
        }
    }

    for (const auto &subdir : subdirs) {
        std::string             file_name = subdir->get_name();

        std::string             str_attributes  = format_attributes(subdir->get_attributes());
        std::string             str_date        = format_date_time(subdir->get_timestamp());
        std::string             str_path        = fmt::format("{}/{}", full_path, file_name);
        
        size_t                  items = subdir->get_files().size() + subdir->get_subdirs().size();

        if (is_regex) {
            if (std::regex_search(str_path, regexp)) {
                fmt::print("{} {} {:3d} {}\n", str_attributes, str_date, 1, str_path);
            }
        } else {
            fmt::print("{} {} {:3d} {}\n", str_attributes, str_date, 1, str_path);
        }

        list_path(subdir, fmt::format("{}/{}", full_path, subdir->get_name()), is_regex, regexp);
    }
}

void Extractor::extract(std::filesystem::path path, std::string regexp, bool overwrite) {
    std::string extract_msg = "Extracting filesystem";

    std::regex  r;
    bool        is_regex = false;

    if (regexp.length()) {
        r = std::regex(regexp);

        is_regex = true;

        extract_msg += fmt::format(". Regexp: '{}'", regexp);
    }

    spdlog::info("{}", extract_msg);
 
    const auto root = filesystem->get_root();

    for (const auto &disk_root : root->get_subdirs()) {
        const std::string &fs_name = disk_root->get_name();

        spdlog::info("Extracting partition {}", fs_name);

        std::filesystem::path dir(path);
        dir.append(fs_name);

        size_t extracted_count = unpack(disk_root, fs_name, dir, is_regex, r);

        if (is_regex && extracted_count == 0) {
            std::error_code error_code;

            System::remove_directory(dir, error_code);
        }
    }
}

// https://gist.github.com/ichramm/3ffeaf7ba4f24853e9ecaf176da84566
bool Extractor::utf8_filename_check(const std::string &file_name, size_t &invalid_pos, size_t &invalid_size) {
    size_t n    = 0;
    size_t len  = file_name.size();

    for (size_t i = 0; i < len; ++i) {
        unsigned char c = static_cast<unsigned char>(file_name.at(i));

        //if (c==0x09 || c==0x0a || c==0x0d || (0x20 <= c && c <= 0x7e) ) n = 0; // is_printable_ascii
        if (0x00 <= c && c <= 0x7f) {
            n = 0; // 0bbbbbbb
        } else if ((c & 0xE0) == 0xC0) {
            n = 1; // 110bbbbb
        } else if   (c == 0xED && i < (len - 1) &&
                    (static_cast<unsigned char>(file_name.at(i + 1)) & 0xA0) == 0xA0) {
            invalid_pos     = i;
            invalid_size    = 255;

            return false; //U+d800 to U+dfff
        } else if ((c & 0xF0) == 0xE0) {
            n = 2; // 1110bbbb
        } else if ((c & 0xF8) == 0xF0) {
            n = 3; // 11110bbb
        //} else if (($c & 0xFC) == 0xF8) { n=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //} else if (($c & 0xFE) == 0xFC) { n=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        } else {
            invalid_pos     = i;
            invalid_size    = 255;

            return false;
        }

        for (size_t j = 0; j < n && i < len; ++j) { // n bytes matching 10bbbbbb follow ?
            if ((++i == len) || (( (static_cast<unsigned char>(file_name.at(i)) & 0xC0) != 0x80))) {
                invalid_pos    = i;
                invalid_size   = n;

                return false;
            }
        }
    }

    return true;
}

void Extractor::utf8_filename_fix(std::string &file_name, size_t invalid_pos, size_t invalid_size, std::function<void()> warn_printer) {
    if (warn_printer) {
        warn_printer();
    }

    if (invalid_size == 255) {
        std::string old_name = file_name;

        for (auto &c : file_name) {
            if (!isprint(static_cast<unsigned char>(c))) {
                c = 0x2D;
            }
        }
    } else {
        for (ssize_t i = invalid_pos; i > (static_cast<ssize_t>(invalid_pos) - static_cast<ssize_t>(invalid_size) - 1); --i) {
            file_name[i] = 0x2D;
        }
    }


    file_name = fmt::format("brk_{}_{}", broken_name_counter++, file_name);
}

std::string Extractor::utf8_filename(std::string file_name) {
    size_t invalid_pos  = 0;
    size_t invalid_size = 0;

    if (!utf8_filename_check(file_name, invalid_pos, invalid_size)) {
        std::string original_fname = file_name;

        spdlog::debug("  Invalid pos: {}, Invalid size: {}", invalid_pos, invalid_size);
        utf8_filename_fix(file_name, invalid_pos, invalid_size, [&]() {
            std::string out;

            for (const auto &c : file_name) {
                out += fmt::format("{:02X} ", static_cast<uint8_t>(c));
            }

            spdlog::warn("    Invalid file name: {}", file_name);
            spdlog::warn("    Hex:               {}", out);
            spdlog::warn("    Invalid character code replaced with 0x2D (-)");
            spdlog::warn("    brk_N_ prefix will be added");
        });

        spdlog::warn("  Filename fixed {} -> {}", original_fname, file_name);
    }

    return file_name;
}

void Extractor::check_unacceptable_symols(std::string &file_name) {
#if defined(_WIN64)
    const char forbidden_characters[] = { '*', '|', ':', '"', '<', '>', '?', '/', '\\', '\n', '\r'};
#else
    const char forbidden_characters[] = { '\\', '/', '\n', '\r'};
#endif

    constexpr size_t size_forbidden = sizeof(forbidden_characters);

    std::string original_name = file_name;

    bool is_fixed = false;

    for (char &c : file_name) {
        for (size_t i = 0; i < size_forbidden; ++i){
            const char &forbidden = forbidden_characters[i];

            if (c == forbidden) {
                is_fixed = true;

                c = '_';
            }
        }
    }

    if (is_fixed) {
        file_name = fmt::format("brk_{}_{}", broken_name_counter++, file_name);

        spdlog::warn("  Filename fixed '{} -> '{}", original_name, file_name);
    }
}


size_t Extractor::unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::string full_path, std::filesystem::path path, bool is_regex, std::regex &regexp) {
    if (System::is_directory_exists(path)) {
        static size_t dbl_counter = 0;

        spdlog::warn("Directory '{}' already exists. postfix will be added", path.string());

        std::string new_filename = fmt::format("{}_{}", path.filename().string(), dbl_counter++);

        path.replace_filename(new_filename);
    }

    std::error_code error_code;

    bool r = System::create_directory(path, 
                    std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                    std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_read | std::filesystem::perms::others_exec, error_code);

    if (!r) {
        throw FULLFLASH::Exception("Couldn't create directory '{}': {}", path.string(), error_code.message());
    }

    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    size_t files_count = 0;

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::filesystem::path   file_path(path);
        std::ofstream           file_stream;
        std::string             file_name = file->get_name();

        file_name = utf8_filename(file_name);
        check_unacceptable_symols(file_name);

        file_path.append(file_name);

        std::string str_path = fmt::format("{}/{}", full_path, file_name);

        if (is_regex && !std::regex_search(str_path, regexp)) {
            continue;
        }

        ++files_count;

        spdlog::info("  File      {}", file_path.string());

        file_stream.open(file_path, std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw FULLFLASH::Exception("Couldn't create file '{}': {}", file_path.string(), strerror(errno));
        }

        if (file->get_size() != 0) {
            file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());
        }

        file_stream.close();

        auto timestamp  = file->get_timestamp();
        set_time(file_path, timestamp);
    }

    for (const auto &subdir : subdirs) {
        std::filesystem::path dir(path);

        std::string subdir_name = subdir->get_name();

        subdir_name = utf8_filename(subdir_name);
        check_unacceptable_symols(subdir_name);

        dir.append(subdir_name);

        std::string str_path = fmt::format("{}/{}", full_path, subdir_name);

        spdlog::info("  Directory {}", dir.string());

        size_t extracted_count = unpack(subdir, str_path, dir, is_regex, regexp);

        files_count += extracted_count;

        if (is_regex && extracted_count == 0) {
            System::remove_directory(dir, error_code);
        }
    }

    auto timestamp = dir->get_timestamp();
    set_time(path, timestamp);

    return files_count;
}

void Extractor::set_time(const std::filesystem::path &path, const FULLFLASH::Filesystem::TimePoint &timestamp) {
    // o_O

#if defined( __linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    auto set = [&] () {
        struct utimbuf new_times;

        new_times.actime    = std::chrono::system_clock::to_time_t(timestamp);
        new_times.modtime   = std::chrono::system_clock::to_time_t(timestamp);

        if (utime(path.c_str(), &new_times) == -1) {
            throw FULLFLASH::Exception("Couldn't set file '{}' mod. date: {}", path.string(), strerror(errno));
        }
    };
#elif defined(_WIN64)
    auto set = [&] () {
        const ULONGLONG UNIX_EPOCH_AS_FILETIME = 116444736000000000ULL; 

        time_t      unix_timestamp  = std::chrono::system_clock::to_time_t(timestamp);
        ULONGLONG   nanakon89       = unix_timestamp * 10000000ULL + UNIX_EPOCH_AS_FILETIME;

        FILETIME    file_time;

        file_time.dwLowDateTime     = static_cast<DWORD>(nanakon89);
        file_time.dwHighDateTime    = static_cast<DWORD>(nanakon89 >> 32);

        // =================================

        HANDLE          h_file;
        std::wstring    ws_path     = path.wstring();
        LPCWSTR         win_path    = reinterpret_cast<LPCWSTR>(ws_path.c_str());
        DWORD           type;

        if (std::filesystem::is_directory(path)) {
            type = FILE_FLAG_BACKUP_SEMANTICS;
        } else {
            type = FILE_ATTRIBUTE_NORMAL;
        }

        h_file = CreateFileW(
            win_path,
            GENERIC_READ | GENERIC_WRITE, // Desired access (read and write)
            FILE_SHARE_READ | FILE_SHARE_WRITE,            // Share mode (allow others to read)
            NULL,                       // Security attributes (default)
            OPEN_EXISTING,              // Creation disposition (open if exists, create if not)
            type,                       // File attributes (normal file)
            NULL                        // Template file (none)
        );

        if (h_file == INVALID_HANDLE_VALUE) {
            throw FULLFLASH::Exception("Couldn't open file '{}' for set mod. date: {}", path.string(), GetLastError());
        }

        if (!SetFileTime(h_file, &file_time, &file_time, &file_time)) {
            CloseHandle(h_file);

            throw FULLFLASH::Exception("Couldn't set file '{}' mod. date: {}", path.string(), GetLastError());
        }

        CloseHandle(h_file);
    };
#endif

    set();
}
