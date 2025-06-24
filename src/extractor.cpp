#include "extractor.h"
#include "help.h"

#include <ffshit/system.h>
#include <ffshit/filesystem/platform/builder.h>

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

// на BSD это конечно же никто не проверял

#if defined( __linux__) || defined(__APPLE__) || defined(__FreeBSD__)
    #include <utime.h>
#elif defined(_WIN64)
    #include <windows.h>
#endif

Extractor::Extractor(FULLFLASH::Partitions::Partitions::Ptr partitions, FULLFLASH::Platform platform) :
    partitions(partitions) {
    
    if (!partitions) {
        throw FULLFLASH::Exception("partitions == nullptr o_O");

    }

    filesystem = FULLFLASH::Filesystem::build(platform, partitions);

    if (filesystem) {
        filesystem->load();
    } else {
        throw FULLFLASH::Exception("fs == nullptr o_O");
    }
}

void Extractor::extract(std::filesystem::path path, bool overwrite) {
    spdlog::info("Extracting filesystem");

    if (System::is_file_exists(path)) {
        bool is_delete = false;

        if (overwrite) {
            is_delete = true;
        } else {
            is_delete = Help::input_yn([&]() {
                spdlog::warn("'{}' is regular file. Delete? (y/n)", path.string());
            });
        }

        if (is_delete) {
            bool r = System::remove_directory(path);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}'", path.string());
            }
        } else {
            return;
        }
    }

    if (System::is_directory_exists(path)) {
        bool is_delete = false;

        if (overwrite) {
            is_delete = true;
        } else {
            is_delete = Help::input_yn([&]() {
                spdlog::warn("Directory '{}' already exists. Delete? (y/n)", path.string());
            });
        }


        if (is_delete) {
            bool r = System::remove_directory(path);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}'", path.string());
            }
        } else {
            return;
        }
    }

    bool r = System::create_directory(path, 
                        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                        std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                        std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

    if (!r) {
        throw FULLFLASH::Exception("Couldn't create directory '{}'", path.string());
    }

    const auto &fs_map = filesystem->get_filesystem_map();

    for (const auto &fs : fs_map) {
        std::string fs_name = fs.first;
        auto root           = fs.second;

        spdlog::info("Extracting partition {}", fs_name);

        std::filesystem::path dir(path);

        dir.append(fs_name);

        unpack(root, dir);
    };
}

void Extractor::unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::filesystem::path path) {
    bool r = System::create_directory(path, 
                    std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                    std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

    if (!r) {
        throw FULLFLASH::Exception("Couldn't create directory '{}'", path.string());
    }

    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::filesystem::path   file_path(path);
        std::ofstream           file_stream;

        file_path.append(file->get_name());

        spdlog::info("  File      {}", file_path.string());

        file_stream.open(file_path, std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw FULLFLASH::Exception("Couldn't create file '{}': {}", file_path.string(), strerror(errno));
        }

        file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());

        file_stream.close();

        auto timestamp  = file->get_timestamp();
        set_time(file_path, timestamp);
    }

    for (const auto &subdir : subdirs) {
        std::filesystem::path dir(path);

        dir.append(subdir->get_name());
        spdlog::info("  Directory {}", dir.string());

        unpack(subdir, dir);
    }

    auto timestamp = dir->get_timestamp();
    set_time(path, timestamp);
}

#ifdef __linux__ 
static void set_linux_file_timestamp(const std::filesystem::path &path, const FULLFLASH::Filesystem::TimePoint &timestamp) {
}
#endif

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
