#include <ffshit/ex.h>

#include <ffshit/version.h>
#include <ffshit/system.h>
#include <ffshit/log/logger.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>

#include <spdlog/spdlog.h>
#include "thirdparty/cxxopts.hpp"

#include "log/interface.h"
#include "extractor.h"
#include "help.h"

#if defined(_WIN64)
    #include <windows.h>
#endif

#ifndef DEF_VERSION_STRING
    #define DEF_VERSION_STRING "unknown"
#endif

Log::Interface::Ptr log_inerface_ptr = Log::Interface::build();

static void dump_partitions_info(const FULLFLASH::Partitions::Partitions &partitions) {
    const auto &p_map = partitions.get_partitions();

    for (const auto &pair : p_map) {
        const std::string & name    = pair.first;
        const auto &        list    = pair.second.get_blocks();

        spdlog::debug("{}:", name);

        for (const auto &block : list) {
            spdlog::debug("  {} Addr: 0x{:08X}, size: {}", name, block.get_addr(), block.get_size());
            spdlog::debug("  Header:");
            spdlog::debug("    Name:     {}", block.get_header().name);
            spdlog::debug("    Unknown1: {:08X}", block.get_header().unknown_1);
            spdlog::debug("    Unknown2: {:08X}", block.get_header().unknown_2);
            spdlog::debug("    Unknown3: {:08X}", block.get_header().unknown_3);
        }
    }
}

static void dump_partitions_short(const FULLFLASH::Partitions::Partitions &partitions) {
    const auto &p_map = partitions.get_partitions();
    std::string partitions_list;

    for (const auto &pair : p_map) {
        const std::string & name    = pair.first;

        partitions_list += fmt::format("{} ", name);
    }

    spdlog::info("Found parts.: [ {}]", partitions_list);
}

#if defined(_WIN64)
static void set_locale() {
    auto curr_locale = std::setlocale(LC_ALL, NULL);

    if (!curr_locale) {
        spdlog::warn("Couldn't get current locale");

        return;
    }

    if (std::string(curr_locale) == "C") {
        spdlog::warn("Current locale is C. Trying set to ru_RU.UTF-8");

        const char* const locale_mame = "ru_RI.UTF-8";
        if (!std::setlocale(LC_ALL, locale_mame)) {
            spdlog::warn("Couldn't set locale to ru_RU.UTF-8");

            return;
        }

        std::locale::global(std::locale(locale_mame));
        spdlog::warn("Locale set to ru_RU.UTF-8");
    }
}
#endif

static std::string build_app_description() {
    std::string app = "Siemens filesystem extractor";
    std::string app_version(DEF_VERSION_STRING);
    std::string libffshit_version(FULLFLASH::get_libffshit_version());

    return fmt::format("{}\n  Version:           {}\n  libffshit version: {}\n", app, app_version, libffshit_version);
}

int main(int argc, char *argv[]) {
    // spdlog::set_pattern("\033[30;1m[%H:%M:%S.%e]\033[0;39m %^[%=8l]%$ \033[1;37m%v\033[0;39m");
    spdlog::set_pattern("[%H:%M:%S.%e] %^[%=8l]%$ %v");

    FULLFLASH::Log::Logger::init(log_inerface_ptr);

    cxxopts::Options options(argv[0], build_app_description());

    std::string ff_path;
    std::string override_dst_path;
    std::string override_platform;

    bool        is_debug                    = false;
    bool        is_overwrite                = false;
    bool        is_filesystem_scan_only     = false;
    bool        is_old_search_algorithm     = false;
    bool        is_partitions_search_only   = false;
    uint32_t    search_start_adddress       = 0;

    try {
        std::string supported_platforms;

        for (const auto &platform : FULLFLASH::StringToPlatform) {
            supported_platforms += fmt::format("{} ", platform.first);
        }

        options.add_options()
            ("d,debug", "Enable debugging")
            ("p,path", "Destination path. Data_<Model>_<IMEI> by default", cxxopts::value<std::string>())
            ("m,platform", "Specify platform (disable autodetect).\n[ " + supported_platforms + "]" , cxxopts::value<std::string>())
            ("start-addr", "Partition search start address (hex)", cxxopts::value<std::string>())
            ("old", "Old search algorithm")
            ("ffpath", "fullflash path", cxxopts::value<std::string>())
            ("f,partitions", "partitions search for debugging purposes only")
            ("s,scan", "filesystem scanning for debugging purposes only")
            ("o,overwrite", "Always delete data directory if exists")
            ("h,help", "Help");

        options.parse_positional({"ffpath"});

        auto parsed = options.parse(argc, argv);

        if (parsed.count("h")) {
            fmt::print("{}", options.help());

            return EXIT_SUCCESS;
        }

        if (!parsed.count("ffpath")) {
            spdlog::error("Please specify fullflash path");

            return EXIT_FAILURE;
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

        ff_path = parsed["ffpath"].as<std::string>();

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

        if (parsed.count("start-addr")) {
            std::string hex_str = parsed["start-addr"].as<std::string>();

            search_start_adddress = std::stoi(hex_str, nullptr, 16);
        }
    } catch (const cxxopts::exceptions::exception &e) {
        spdlog::error("{}", e.what());

        return EXIT_FAILURE;
    } catch (const std::invalid_argument &e) {
        spdlog::error("Incorrect value");

        return EXIT_FAILURE;
    }

#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    set_locale();
#endif

    if (is_debug) {
        spdlog::set_level(spdlog::level::debug);
    }

    try {
        FULLFLASH::Platform                     platform;
        FULLFLASH::Partitions::Partitions::Ptr  partitions;

        std::string imei;
        std::string model;

        if (override_platform.length()) {
            platform    = FULLFLASH::StringToPlatform.at(override_platform);

            partitions = FULLFLASH::Partitions::Partitions::build(ff_path, platform, is_old_search_algorithm, search_start_adddress);
        } else {
            partitions = FULLFLASH::Partitions::Partitions::build(ff_path, is_old_search_algorithm, search_start_adddress);

            platform    = partitions->get_platform();
            imei        = partitions->get_imei();
            model       = partitions->get_model();

            spdlog::info("Platform: {}", FULLFLASH::PlatformToString.at(platform));
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

        std::filesystem::path data_path;

        data_path.append(fmt::format("Data_{}_{}", model, imei));

        if (override_dst_path.length() != 0) {
            spdlog::warn("Destination path override '{}' -> '{}'", data_path.string(), override_dst_path);

            data_path = override_dst_path;
        } else {
            spdlog::info("Destination path: {}", data_path.string());
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


        Extractor extractor(partitions, platform);

        if (!is_filesystem_scan_only) {
            extractor.extract(data_path, is_overwrite);
        }

        spdlog::info("Done");

    } catch (const FULLFLASH::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Partitions::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const std::filesystem::filesystem_error &e) {
        spdlog::error("Filesystem error: '{}' '{}' {}", e.path1().string(), e.path2().string(), e.what());
    }

    return 0;
}
