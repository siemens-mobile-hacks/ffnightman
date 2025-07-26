#include "cli/options.h"

#include <ffshit/system.h>
#include <ffshit/version.h>
#include <ffshit/filesystem/platform/types.h>

#include <fmt/format.h>

#include "log/log.h"

namespace CLI {

static std::string build_app_description() {
    std::string app = "Siemens filesystem extractor";
    std::string app_version(DEF_VERSION_STRING);
    std::string libffshit_version(FULLFLASH::get_libffshit_version());

    return fmt::format("{}\n  Version:           {}\n  libffshit version: {}\n", app, app_version, libffshit_version);
}

int parse(int argc, char *argv[], Options &opts) {
    std::string supported_platforms;

    for (const auto &platform : FULLFLASH::StringToPlatform) {
        supported_platforms += fmt::format("{} ", platform.first);
    }

    cxxopts::Options options(argv[0], build_app_description());

    options.add_options("Extraction")
        ("p,path", "Destination path. './<FF_file_name>_data' by default", cxxopts::value<std::string>())
        ("o,overwrite", "Always delete data directory if exists")
        ("ffpath", "fullflash path", cxxopts::value<std::string>())
        ("skip", "Skip broken file/directory")
        ("skip-dup", "Skip duplicate id")
        ("skip-all", "Enable all skip\n")
        ("part", "Partition to extract (may be several)", cxxopts::value<std::vector<std::string>>())
        ("regexp", "Extract content whose path matches the regexp. C++ regex compatible", cxxopts::value<std::string>()->default_value(""));

    options.add_options("Partitions search")
        ("m,platform", "Specify platform (disable autodetect).\n[ " + supported_platforms + "]" , cxxopts::value<std::string>())
        ("start-addr", "Partition search start address (hex)", cxxopts::value<std::string>())
        ("old", "Old search algorithm")
        ("part-scan", "partitions search for debugging purposes only");

    options.add_options("Filesystem")
        ("f,fs-platform", "Specify filesystem type (for fullflash from prototype by example).\n[ " + supported_platforms + "]" , cxxopts::value<std::string>())
        ("fs-scan", "filesystem scanning for debugging purposes only");

    options.add_options("Listing")
        ("ls", "List content whose path matches the regexp. C++ regex compatible", cxxopts::value<std::string>()->default_value(""));

    options.add_options("Logging")
        ("l,log", "Save log to file '<dst_path>/extracting.log'")
        ("v,verbose", "Verbose level\nv   - Verbose processing\nvv  - Verbose headers\nvvv - Verbose data", cxxopts::value<bool>()->default_value("false"))
        ("d,debug", "Verbose level = vvv")
        ("h,help", "Help");

    options.parse_positional({"ffpath"});

    auto parsed = options.parse(argc, argv);

    if (parsed.count("h")) {
        fmt::print("{}", options.help());

        return EXIT_OK;
    }

    int verbose_level = parsed.count("v");

    if (!parsed.count("ffpath")) {
        spdlog::error("Please specify fullflash path");

        fmt::print("\n{}", options.help());

        return EXIT_FAIL;
    }

    opts.ff_path = parsed["ffpath"].as<std::string>();

    if (parsed.count("l")) {
        opts.is_log_to_file = true;
    }

    if (parsed.count("m")) {
        std::string platform_raw = parsed["m"].as<std::string>();

        System::to_upper(platform_raw);

        if (FULLFLASH::StringToPlatform.count(platform_raw) == 0) {
            throw cxxopts::exceptions::exception(fmt::format("Unknown platform {}. See help", platform_raw));
        }

        opts.override_platform = platform_raw;
    }

    if (parsed.count("fs-platform")) {
        std::string platform_raw = parsed["fs-platform"].as<std::string>();

        System::to_upper(platform_raw);

        if (FULLFLASH::StringToPlatform.count(platform_raw) == 0) {
            throw cxxopts::exceptions::exception(fmt::format("Unknown filesystem platform {}. See help", platform_raw));
        }

        opts.override_fs_platform = platform_raw;
    }

    if (parsed.count("p")) {
        opts.override_dst_path = parsed["p"].as<std::string>();
    }

    if (parsed.count("o")) {
        opts.is_overwrite = true;
    }

    if (parsed.count("fs-scan")) {
        opts.is_filesystem_scan_only = true;
    }

    if (parsed.count("old")) {
        opts.is_old_search_algorithm = true;
    }

    if (parsed.count("part-scan")) {
        opts.is_partitions_search_only = true;
    }

    if (parsed.count("skip")) {
        opts.is_skip_broken = true;
    }

    if (parsed.count("skip-dup")) {
        opts.is_skip_dup = true;
    }

    if (parsed.count("skip-all")) {
        opts.is_skip_broken  = true;
        opts.is_skip_dup     = true;
    }

    if (parsed.count("start-addr")) {
        std::string hex_str = parsed["start-addr"].as<std::string>();

        opts.search_start_adddress = std::stoi(hex_str, nullptr, 16);
    }

    if (parsed.count("part")) {
        opts.parts_to_extract = parsed["part"].as<std::vector<std::string>>();
    }

    if (parsed.count("ls")) {
        opts.is_list_only = true;

        opts.ls_regex = parsed["ls"].as<std::string>(); 
    }

    if (parsed.count("regexp")) {
        opts.regexp = parsed["regexp"].as<std::string>();
    }

    if (parsed.count("d")) {
        verbose_level = 3;
    }

    switch(verbose_level) {
        case 0: break;
        default:
        case 3: opts.verbose_data       = true;
        case 2: opts.verbose_headers    = true;
        case 1: opts.verbose_processing = true;
    }

    return CONTINUE;
}

};
