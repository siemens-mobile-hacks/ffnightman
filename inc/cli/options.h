#ifndef FFNIGHTMAN_CLI_OPTIONS_H
#define FFNIGHTMAN_CLI_OPTIONS_H

#include <filesystem>
#include <string>
#include <cstdint>

#include "thirdparty/cxxopts.hpp"

namespace CLI {

struct Options {
    Options() = default;

    std::filesystem::path       ff_path;
    std::string                 override_dst_path;
    std::string                 override_platform;
    std::string                 override_fs_platform;

    std::vector<std::string>    parts_to_extract;

    bool                        verbose_processing          = false;
    bool                        verbose_headers             = false;
    bool                        verbose_data                = false;
    bool                        is_overwrite                = false;
    bool                        is_filesystem_scan_only     = false;
    bool                        is_old_search_algorithm     = false;
    bool                        is_partitions_search_only   = false;
    bool                        is_list_only                = false;
    std::string                 ls_regex;
    bool                        is_skip_broken              = false;
    bool                        is_skip_dup                 = false;
    bool                        is_dump_data                = false;
    bool                        is_log_to_file              = false;
    std::string                 regexp;
    uint32_t                    search_start_adddress       = 0;
};

constexpr const int EXIT_OK     = EXIT_SUCCESS;
constexpr const int EXIT_FAIL   = EXIT_FAILURE;
constexpr const int CONTINUE    = 100500;

int parse(int argc, char *argv[], Options &opts);

};

#endif
