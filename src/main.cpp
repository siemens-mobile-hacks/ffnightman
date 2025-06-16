#include <ffshit/blocks.h>
#include <ffshit/ex.h>

#include <ffshit/system.h>
#include <ffshit/log/logger.h>
#include <ffshit/filesystem/ex.h>

#include <spdlog/spdlog.h>
#include "thirdparty/cxxopts.hpp"

#include "log/interface.h"
#include "extractor.h"
#include "help.h"

#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>

Log::Interface::Ptr log_inerface_ptr = Log::Interface::build();

void dump_partitions_info(const FULLFLASH::Partitions::Partitions &partitions) {
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

int main(int argc, char *argv[]) {
    // spdlog::set_pattern("\033[30;1m[%H:%M:%S.%e]\033[0;39m %^[%=8l]%$ \033[1;37m%v\033[0;39m");
    spdlog::set_pattern("[%H:%M:%S.%e] %^[%=8l]%$ %v");

    FULLFLASH::Log::Logger::init(log_inerface_ptr);

    cxxopts::Options options(argv[0], "Siemens filesystem extractor");

    std::string ff_path;
    std::string override_dst_path;
    std::string override_platform;

    bool        is_debug        = false;
    bool        is_overwrite    = false;
    bool        is_scan_only    = false;

    try {
        std::string supported_platforms;

        for (const auto &platform : FULLFLASH::StringToPlatform) {
            supported_platforms += fmt::format("{} ", platform.first);
        }

        options.add_options()
            ("d,debug", "Enable debugging")
            ("p,path", "Destination path. Data_<Model>_<IMEI> by default", cxxopts::value<std::string>())
            ("m,platform", "Specify platform (disable autodetect).\n[ " + supported_platforms + "]" , cxxopts::value<std::string>())
            ("ffpath", "fullflash path", cxxopts::value<std::string>())
            ("s,scan", "fullflash scanning for debugging purposes only")
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
            is_scan_only = true;
        }
    } catch (const cxxopts::exceptions::exception &e) {
        spdlog::error("{}", e.what());
    }

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

            partitions = FULLFLASH::Partitions::Partitions::build(ff_path, platform);
        } else {
            partitions = FULLFLASH::Partitions::Partitions::build(ff_path);

            platform    = partitions->get_platform();
            imei        = partitions->get_imei();
            model       = partitions->get_model();

            spdlog::info("Platform: {}", FULLFLASH::PlatformToString.at(platform));
        }

        if (model.length()) {
            spdlog::info("Model:    {}", model);
        }

        if (imei.length()) {
            spdlog::info("IMEI:     {}", imei);
        }

        if (is_debug) {
            dump_partitions_info(*partitions);
        }

        std::filesystem::path data_path;

        data_path.append(fmt::format("Data_{}_{}", model, imei));

        if (override_dst_path.length() != 0) {
            spdlog::warn("Destination path override '{}' -> '{}'", data_path.string(), override_dst_path);

            data_path = override_dst_path;
        } else {
            spdlog::info("Destination path: {}", data_path.string());
        }

        if (platform == FULLFLASH::Platform::X85 && !is_scan_only) {
            bool is_continue = false;

            is_continue = Help::input_yn([]() {
                spdlog::warn("X85 platform unsupported yet. Continue (y/n)?");
            });

            if (!is_continue) {
                return EXIT_SUCCESS;
            }
        }

        Extractor extractor(partitions, platform);

        if (!is_scan_only) {
            extractor.extract(data_path, is_overwrite);
        }

        spdlog::info("Done");

    } catch (const FULLFLASH::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Partitions::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        spdlog::error("{}", e.what());
    }

    // try {
    //     FULLFLASH::Platform     platform;
    //     std::string             imei;
    //     std::string             model;

    //     FULLFLASH::Blocks::Ptr  blocks;

    //     if (override_platform.length()) {
    //         spdlog::warn("Manualy selected platform: {}", override_platform);
            
    //         platform    = FULLFLASH::StringToPlatform.at(override_platform);
    //         blocks      = FULLFLASH::Blocks::build(ff_path, platform);
    //     } else {
    //         blocks = FULLFLASH::Blocks::build(ff_path);

    //         platform    = blocks->get_platform();
    //         imei        = blocks->get_imei();
    //         model       = blocks->get_model();

    //         spdlog::info("Platform: {}", FULLFLASH::PlatformToString.at(platform));
    //     }
    
    //     if (model.length()) {
    //         spdlog::info("Model:    {}", model);
    //     }

    //     if (imei.length()) {
    //         spdlog::info("IMEI:     {}", imei);
    //     }

    //     std::filesystem::path data_path;// std::filesystem::current_path();

    //     data_path.append(fmt::format("Data_{}_{}", model, imei));

    //     if (override_dst_path.length() != 0) {
    //         spdlog::warn("Destination path override '{}' -> '{}'", data_path.string(), override_dst_path);

    //         data_path = override_dst_path;
    //     } else {
    //         spdlog::info("Destination path: {}", data_path.string());
    //     }

    //     if (platform == FULLFLASH::Platform::X85 && !is_scan_only) {
    //         bool is_continue = false;

    //         is_continue = Help::input_yn([]() {
    //             spdlog::warn("X85 platform unsupported yet. Continue (y/n)?");
    //         });

    //         if (!is_continue) {
    //             return EXIT_SUCCESS;
    //         }
    //     }

    //     if (is_debug) {
    //         blocks->print();
    //     }

    //     Extractor extractor(*blocks, platform);

    //     if (!is_scan_only) {
    //         extractor.extract(data_path, is_overwrite);
    //     }

    //     spdlog::info("Done");

    // } catch (const FULLFLASH::Exception &e) {
    //     spdlog::error("{}", e.what());
    // } catch (const FULLFLASH::Filesystem::Exception &e) {
    //     spdlog::error("{}", e.what());
    // }

    return 0;
}
