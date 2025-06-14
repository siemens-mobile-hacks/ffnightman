#include "extractor.h"

#include <ffshit/system.h>
#include <ffshit/filesystem/platform/builder.h>

#include <iostream>
#include <spdlog/spdlog.h>

Extractor::Extractor(FULLFLASH::Blocks &blocks, FULLFLASH::Platform platform) : blocks(blocks) {
    filesystem = FULLFLASH::Filesystem::build(platform, blocks);

    if (filesystem) {
        filesystem->load();
    } else {
        spdlog::error("fs == nullptr o_O");
    }
}

void Extractor::extract(std::string path, bool overwrite) {
    spdlog::info("Extracting filesystem");

    if (System::is_file_exists(path)) {
        std::string yes_no;

        if (overwrite) {
            yes_no = "y";
        } else {
            while (yes_no != "n" && yes_no != "y") {
                spdlog::warn("'{}' is regular file. Delete? (y/n)", path);

                yes_no.clear();
                std::cin >> yes_no;

                System::to_lower(yes_no);
            }
        }

        if (yes_no == "y") {
            bool r = System::remove_directory(path);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}'", path);
            }
        } else {
            return;
        }

    }

    if (System::is_directory_exists(path)) {
        std::string yes_no;

        if (overwrite) {
            yes_no = "y";
        } else {
            while (yes_no != "n" && yes_no != "y") {
                spdlog::warn("Directory '{}' already exists. Delete? (y/n)", path);

                yes_no.clear();
                std::cin >> yes_no;
                std::transform(yes_no.begin(), yes_no.end(), yes_no.begin(), [](unsigned char c){ 
                    return std::tolower(c); 
                });
            }
        }

        if (yes_no == "y") {
            bool r = System::remove_directory(path);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}'", path);
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
        throw FULLFLASH::Exception("Couldn't create directory '{}'", path);
    }

    const auto &fs_map = filesystem->get_filesystem_map();

    for (const auto &fs : fs_map) {
        std::string fs_name = fs.first;
        auto root           = fs.second;

        spdlog::info("Extracting {}", fs_name);

        unpack(root, path + "/" + fs_name);
    };
}

void Extractor::unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::string path) {
    bool r = System::create_directory(path, 
                    std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                    std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                    std::filesystem::perms::others_read | std::filesystem::perms::others_exec);

    if (!r) {
        throw FULLFLASH::Exception("Couldn't create directory '{}'", path);
    }

    const auto &subdirs = dir->get_subdirs();
    const auto &files   = dir->get_files();

    for (const auto &file : files) {
        if (file->get_name().length() == 0) {
            continue;
        }

        std::string     file_path = path + "/" + file->get_name();
        std::ofstream   file_stream;

        spdlog::info("  Extracting {}", file_path);

        file_stream.open(file_path, std::ios_base::binary | std::ios_base::trunc);

        if (!file_stream.is_open()) {
            throw FULLFLASH::Exception("Couldn't create file '{}': {}", file_path, strerror(errno));
        }

        file_stream.write(file->get_data().get_data().get(), file->get_data().get_size());

        file_stream.close();
    }

    for (const auto &subdir : subdirs) {
        unpack(subdir, path + "/" + subdir->get_name());
    }

}
