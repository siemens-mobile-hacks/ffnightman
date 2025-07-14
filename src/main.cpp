#include <ffshit/ex.h>
#include <ffshit/version.h>
#include <ffshit/system.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>

#include "thirdparty/cxxopts.hpp"

#include "log/log.h"
#include "extractor.h"
#include "help.h"

#include <iomanip>

#if defined(_WIN64)
    #include <windows.h>
#endif

#ifndef DEF_VERSION_STRING
    #define DEF_VERSION_STRING "unknown"
#endif

static void dump_partitions_info(const FULLFLASH::Partitions::Partitions &partitions) {
    const auto &p_map = partitions.get_partitions();

    for (const auto &pair : p_map) {
        const std::string & name    = pair.first;
        const auto &        list    = pair.second.get_blocks();

        spdlog::debug("{} {} Blocks:", name, list.size());

        for (const auto &block : list) {
            const auto &header = block.get_header();

            spdlog::debug("  {:8s} Addr: 0x{:08X}, size: {}, Unk1: {:08X}, Unk2: {:08X}, Unk3: {:08X}, Unk4: {:08X}", 
                header.name,
                block.get_addr(),
                block.get_size(),
                header.unknown_1,
                header.unknown_2,
                header.unknown_3,
                header.unknown_4);
        }
    }
}

static void dump_partitions_short(const FULLFLASH::Partitions::Partitions &partitions) {
    const auto &p_map = partitions.get_partitions();
    std::string partitions_list;

    spdlog::info("Found {} partitions", p_map.size());

    for (const auto &pair : p_map) {
        spdlog::info("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }
}

static std::string build_app_description() {
    std::string app = "Siemens filesystem extractor";
    std::string app_version(DEF_VERSION_STRING);
    std::string libffshit_version(FULLFLASH::get_libffshit_version());

    return fmt::format("{}\n  Version:           {}\n  libffshit version: {}\n", app, app_version, libffshit_version);
}

static void setup_destination_path(std::filesystem::path path, bool overwrite) {
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
            std::error_code error_code;

            bool r = System::remove_directory(path, error_code);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}': {}", path.string(), error_code.message());
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
            std::error_code error_code;

            bool r = System::remove_directory(path, error_code);

            if (!r) {
                throw FULLFLASH::Exception("Couldn't delete directory '{}': {}", path.string(), error_code.message());
            }
        } else {
            return;
        }
    }

    std::error_code error_code;

    bool r = System::create_directory(path, 
                        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                        std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                        std::filesystem::perms::others_read | std::filesystem::perms::others_exec, error_code);

    if (!r) {
        throw FULLFLASH::Exception("Couldn't create directory '{}': {}", path.string(), error_code.message());
    }
}

int main(int argc, char *argv[]) {
    Log::init();
    Log::setup();

    cxxopts::Options options(argv[0], build_app_description());

    std::filesystem::path   ff_path;
    std::string             override_dst_path;
    std::string             override_platform;

    bool        is_debug                    = false;
    bool        is_overwrite                = false;
    bool        is_filesystem_scan_only     = false;
    bool        is_old_search_algorithm     = false;
    bool        is_partitions_search_only   = false;
    bool        is_skip_broken              = false;
    bool        is_skip_dup                 = false;
    bool        is_dump_data                = false;
    bool        is_log_to_file              = false;

    uint32_t    search_start_adddress       = 0;

    try {
        std::string supported_platforms;

        for (const auto &platform : FULLFLASH::StringToPlatform) {
            supported_platforms += fmt::format("{} ", platform.first);
        }

        options.add_options()
            ("d,debug", "Enable debugging")
            ("p,path", "Destination path. './<FF_file_name>_data' by default", cxxopts::value<std::string>())
            ("m,platform", "Specify platform (disable autodetect).\n[ " + supported_platforms + "]" , cxxopts::value<std::string>())
            ("l,log", "Save log to file './<FF_file_name>/extracting.log'")
            ("dump", "Dump data to debug output")
            ("start-addr", "Partition search start address (hex)", cxxopts::value<std::string>())
            ("old", "Old search algorithm")
            ("ffpath", "fullflash path", cxxopts::value<std::string>())
            ("f,partitions", "partitions search for debugging purposes only")
            ("s,scan", "filesystem scanning for debugging purposes only")
            ("o,overwrite", "Always delete data directory if exists")
            ("skip", "Skip broken file/directory")
            ("skip-dup", "Skip duplicate id")
            ("proto", "For fullflash from prototypes. Enable all skip")
            ("h,help", "Help");

        options.parse_positional({"ffpath"});

        auto parsed = options.parse(argc, argv);

        if (parsed.count("h")) {
            fmt::print("{}", options.help());

            return EXIT_SUCCESS;
        }

        if (!parsed.count("ffpath")) {
            spdlog::error("Please specify fullflash path");

            fmt::print("\n{}", options.help());

            return EXIT_FAILURE;
        }

        ff_path = parsed["ffpath"].as<std::string>();

        if (parsed.count("l")) {
            is_log_to_file = true;
        }

        if (parsed.count("d")) {
            is_debug = true;
        }

        if (parsed.count("m")) {
            std::string platform_raw = parsed["m"].as<std::string>();

            System::to_upper(platform_raw);

            if (FULLFLASH::StringToPlatform.count(platform_raw) == 0) {
                throw cxxopts::exceptions::exception(fmt::format("Unknown platform {}. See help", platform_raw));
            }

            override_platform = platform_raw;
        }

        if (parsed.count("p")) {
            override_dst_path = parsed["p"].as<std::string>();
        }

        if (parsed.count("o")) {
            is_overwrite = true;
        }

        if (parsed.count("s")) {
            is_filesystem_scan_only = true;
        }

        if (parsed.count("old")) {
            is_old_search_algorithm = true;
        }

        if (parsed.count("f")) {
            is_partitions_search_only = true;
        }

        if (parsed.count("skip")) {
            is_skip_broken = true;
        }

        if (parsed.count("skip-dup")) {
            is_skip_dup = true;
        }

        if (parsed.count("proto")) {
            is_skip_broken  = true;
            is_skip_dup     = true;
        }

        if (parsed.count("start-addr")) {
            std::string hex_str = parsed["start-addr"].as<std::string>();

            search_start_adddress = std::stoi(hex_str, nullptr, 16);
        }

        if (parsed.count("dump")) {
            is_dump_data = true;
        }
    } catch (const cxxopts::exceptions::exception &e) {
        spdlog::error("{}", e.what());

        return EXIT_FAILURE;
    } catch (const std::invalid_argument &e) {
        spdlog::error("Incorrect value");

        return EXIT_FAILURE;
    }

#if defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
#endif

#if defined(_WIN64) && defined(_MSC_VER)
    Help::set_utf8_locale();
#endif

    if (is_debug) {
        spdlog::set_level(spdlog::level::debug);
    }

    std::filesystem::path data_path = fmt::format("{}_data", ff_path.filename().string());

    if (override_dst_path.length() != 0) {
        data_path = override_dst_path;
    }

    try {
        setup_destination_path(data_path, is_overwrite);
        
        if (is_log_to_file) {
            Log::setup(data_path);
        }

        FULLFLASH::Platform                     platform;
        FULLFLASH::Partitions::Partitions::Ptr  partitions;

        std::string imei;
        std::string model;

        if (override_platform.length()) {
            platform    = FULLFLASH::StringToPlatform.at(override_platform);

            partitions = FULLFLASH::Partitions::Partitions::build(ff_path, platform, is_old_search_algorithm, search_start_adddress);

            model       = partitions->get_model();
        } else {
            partitions = FULLFLASH::Partitions::Partitions::build(ff_path, is_old_search_algorithm, search_start_adddress);

            platform    = partitions->get_platform();
            imei        = partitions->get_imei();
            model       = partitions->get_model();

            spdlog::info("Platform:     {}", FULLFLASH::PlatformToString.at(platform));
        }

        if (model.length()) {
            spdlog::info("Model:        {}", model);
        }

        if (imei.length()) {
            spdlog::info("IMEI:         {}", imei);
        }

        if (is_debug) {
            dump_partitions_info(*partitions);
        } else {
            dump_partitions_short(*partitions);
        }

        if (is_partitions_search_only) {
            return EXIT_SUCCESS;
        }

        if (platform == FULLFLASH::Platform::SGOLD2_ELKA && !is_filesystem_scan_only) {
            bool is_continue = false;

            is_continue = Help::input_yn([]() {
                spdlog::warn("SGOLD2_ELKA platform unsupported yet. Continue (y/n)?");
            });

            if (!is_continue) {
                return EXIT_SUCCESS;
            }
        }

        Extractor extractor(partitions, platform, is_skip_broken, is_skip_dup, is_dump_data);

        if (!is_filesystem_scan_only) {
            spdlog::info("Destination path: {}", data_path.string());

            extractor.extract(data_path, is_overwrite);
        }

        spdlog::info("Done");

    } catch (const FULLFLASH::BaseException &e) {
        spdlog::error("{}", e.what());
    } catch (const Patterns::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const std::filesystem::filesystem_error &e) {
        spdlog::error("Filesystem error: '{}' '{}' {}", e.path1().string(), e.path2().string(), e.what());
    }


    return 0;
}
