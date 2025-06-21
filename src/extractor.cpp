#include "extractor.h"
#include "help.h"

#include <ffshit/system.h>
#include <ffshit/filesystem/platform/builder.h>

#include <iostream>
#include <algorithm>
#include <spdlog/spdlog.h>

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

// Extractor::Extractor(FULLFLASH::Blocks &blocks, FULLFLASH::Platform platform) : blocks(blocks) {
//     filesystem = FULLFLASH::Filesystem::build(platform, blocks);

//     if (filesystem) {
//         filesystem->load();
//     } else {
//         throw FULLFLASH::Exception("fs == nullptr o_O");
//     }
// }

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

        file_stream.open(file_path.string(), std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw FULLFLASH::Exception("Couldn't create file '{}': {}", file_path.string(), strerror(errno));
        }

        file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());

        file_stream.close();
    }

    for (const auto &subdir : subdirs) {
        std::filesystem::path dir(path);

        dir.append(subdir->get_name());
        spdlog::info("  Directory {}", dir.string());

        unpack(subdir, dir);
    }

}
