#include "writer.h"

#include <ffshit/ex.h>

#include "log/log.h"

#include <fstream>

Writer::Writer(FULLFLASH::FULLFLASH::Ptr fullflash, FULLFLASH::Filesystem::Base::Ptr filesystem) :
    fullflash(fullflash),
    filesystem(filesystem) {

    if (!fullflash || !filesystem) {
        throw FULLFLASH::Exception("fullflash or filesystem == nullptr o_O");
    }
}

void Writer::write(const std::vector<CLI::Write> &writes) {
    for (const auto &write : writes) {
        switch (write.action) {
            case CLI::Write::Action::PUT: {
                put(write.argument);

                break;
            }

            case CLI::Write::Action::MKDIR: {
                spdlog::info("Creating directory {}", write.argument);

                filesystem->create_directory(write.argument, std::chrono::system_clock::now());

                break;
            }

            case CLI::Write::Action::RM: {
                spdlog::info("Removing {}", write.argument);

                filesystem->remove(write.argument);

                break;
            }
        }
    }
}

void Writer::save(const std::filesystem::path &path) {
    spdlog::info("Saving fullflash to {}", path.string());

    fullflash->save(path);
}

// C++17 has no conversion between the clocks, so this takes the offset between them now
static FULLFLASH::Filesystem::TimePoint modification_time(const std::filesystem::path &path) {
    auto file_time = std::filesystem::last_write_time(path);

    return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        file_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
}

void Writer::put(const std::string &argument) {
    // The last colon: a Windows path has one after the drive letter
    size_t separator = argument.rfind(':');

    if (separator == std::string::npos || separator == 0 || separator + 1 == argument.size()) {
        throw FULLFLASH::Exception("--put {}: expected <local path>:<path>", argument);
    }

    std::filesystem::path   local_path(argument.substr(0, separator));
    std::string             path = argument.substr(separator + 1);

    std::ifstream file(local_path, std::ios_base::binary | std::ios_base::in);

    if (!file.is_open()) {
        throw FULLFLASH::Exception("Couldn't open file '{}': {}", local_path.string(), strerror(errno));
    }

    size_t size = std::filesystem::file_size(local_path);

    spdlog::info("Writing {} -> {} ({} bytes)", local_path.string(), path, size);

    FULLFLASH::RawData data;

    if (size != 0) {
        data = FULLFLASH::RawData(file, 0, size);
    }

    filesystem->write_file(path, data, modification_time(local_path));
}
