#ifndef FFNIGHTMAN_EXTRACTOR_H
#define FFNIGHTMAN_EXTRACTOR_H

#include <ffshit/filesystem/platform/platform.h>

class Extractor {
    public:
        Extractor() = delete;
        Extractor(FULLFLASH::Blocks &blocks, FULLFLASH::Platform platform);

        void                                extract(std::string path, bool overwrite);
        void                                unpack(FULLFLASH::Filesystem::Directory::Ptr dir, std::string path = "/");

    private:
        FULLFLASH::Blocks &                 blocks;
        FULLFLASH::Filesystem::Base::Ptr    filesystem;
};

#endif

