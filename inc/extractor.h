#ifndef FFNIGHTMAN_EXTRACTOR_H
#define FFNIGHTMAN_EXTRACTOR_H

#include <ffshit/filesystem/platform/platform.h>
#include "cli/options.h"

#include <filesystem>
#include <functional>

class Extractor {
    public:
        Extractor() = delete;
        Extractor(FULLFLASH::Partitions::Partitions::Ptr partitions, FULLFLASH::Platform platform, const CLI::Options &options);

        void                                    list(std::string regexp);
        void                                    extract(std::filesystem::path path, bool overwrite);

    private:
        FULLFLASH::Partitions::Partitions::Ptr  partitions;
        FULLFLASH::Filesystem::Base::Ptr        filesystem;

        void                                    list_path(FULLFLASH::Filesystem::Directory::Ptr dir, std::string full_path, bool is_regex, std::regex &r);
        void                                    unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::filesystem::path path);

        static std::string                      utf8_filename(std::string file_name);
        static bool                             utf8_filename_check(const std::string &file_name, size_t &invalid_pos, size_t &invalid_size);
        static void                             utf8_filename_fix(std::string &file_name, size_t invalid_pos, size_t invalid_size, std::function<void()> warn_printer);
        static void                             check_unacceptable_symols(std::string &file_name);

        void                                    set_time(const std::filesystem::path &path, const FULLFLASH::Filesystem::TimePoint &timestamp);
};

#endif

