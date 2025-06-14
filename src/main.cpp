#include <ffshit/blocks.h>
#include <ffshit/ex.h>

#include <ffshit/system.h>
#include <ffshit/log/logger.h>
#include <ffshit/filesystem/ex.h>

#include <spdlog/spdlog.h>
#include "thirdparty/cxxopts.hpp"

#include "log/interface.h"
#include "extractor.h"

Log::Interface::Ptr log_inerface_ptr = Log::Interface::build();

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
        FULLFLASH::Platform     platform;
        std::string             imei;
        std::string             model;

        FULLFLASH::Blocks::Ptr  blocks;

        if (override_platform.length()) {
            spdlog::warn("Manualy selected platform: {}", override_platform);
            
            platform    = FULLFLASH::StringToPlatform.at(override_platform);
            blocks      = FULLFLASH::Blocks::build(ff_path, platform);
        } else {
            blocks = FULLFLASH::Blocks::build(ff_path);

            platform    = blocks->get_platform();
            imei        = blocks->get_imei();
            model       = blocks->get_model();

            spdlog::info("Platform: {}", FULLFLASH::PlatformToString.at(platform));
        }
    
        if (model.length()) {
            spdlog::info("Model:    {}", model);
        }

        if (imei.length()) {
            spdlog::info("IMEI:     {}", imei);
        }

        std::string data_path = fmt::format("./Data_{}_{}", model, imei);

        if (override_dst_path.length() != 0) {
            spdlog::warn("Destination path override '{}' -> '{}'", data_path, override_dst_path);

            data_path = override_dst_path;
        }

        if (is_debug) {
            blocks->print();
        }

        Extractor extractor(*blocks, platform);

        if (!is_scan_only) {
            extractor.extract(data_path, is_overwrite);
        }
        
        spdlog::info("Done");

    } catch (const FULLFLASH::Exception &e) {
        spdlog::error("{}", e.what());
    } catch (const FULLFLASH::Filesystem::Exception &e) {
        spdlog::error("{}", e.what());
    }

    return 0;
}
