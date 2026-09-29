#ifndef FFNIGHTMAN_WRITER_H
#define FFNIGHTMAN_WRITER_H

#include <ffshit/fullflash.h>
#include <ffshit/filesystem/platform/platform.h>
#include "cli/options.h"

#include <filesystem>

class Writer {
    public:
        Writer() = delete;
        Writer(FULLFLASH::FULLFLASH::Ptr fullflash, FULLFLASH::Filesystem::Base::Ptr filesystem);

        void                                    write(const std::vector<CLI::Write> &writes);
        void                                    save(const std::filesystem::path &path);

    private:
        FULLFLASH::FULLFLASH::Ptr               fullflash;
        FULLFLASH::Filesystem::Base::Ptr        filesystem;

        void                                    put(const std::string &argument);
};

#endif
