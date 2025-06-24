#ifndef FFNIGHTMAN_EXTRACTOR_H
#define FFNIGHTMAN_EXTRACTOR_H

#include <ffshit/filesystem/platform/platform.h>

#include <filesystem>

class Extractor {
    public:
        Extractor() = delete;
        Extractor(FULLFLASH::Partitions::Partitions::Ptr partitions, FULLFLASH::Platform platform);

        void                                    extract(std::filesystem::path path, bool overwrite);
        void                                    unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::filesystem::path path);

    private:
        FULLFLASH::Partitions::Partitions::Ptr partitions;
        FULLFLASH::Filesystem::Base::Ptr       filesystem;

        void                                    set_time(const std::filesystem::path &path, const FULLFLASH::Filesystem::TimePoint &timestamp);
};

#endif

