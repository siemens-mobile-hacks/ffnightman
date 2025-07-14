#include <ffshit/ex.h>
#include <ffshit/system.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>

#include "log/log.h"
#include "cli/options.h"
#include "extractor.h"
#include "help.h"

#include <iomanip>

#if defined(_WIN64)
    #include <windows.h>
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
    try {
        Log::init();
        Log::setup();
    } catch (const spdlog::spdlog_ex &e) {
        fmt::print("Log init error: {}", e.what());

        return -1;
    }

    CLI::Options options;

    try {
        int r = CLI::parse(argc, argv, options);

        if (r != CLI::CONTINUE) {
            return r;
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

    if (options.is_debug) {
        spdlog::set_level(spdlog::level::debug);
    }

    std::filesystem::path data_path = fmt::format("{}_data", options.ff_path.filename().string());

    if (options.override_dst_path.length() != 0) {
        data_path = options.override_dst_path;
    }

    try {
        setup_destination_path(data_path, options.is_overwrite);
        
        if (options.is_log_to_file) {
            Log::setup(data_path);
        }

        FULLFLASH::Platform                     platform;
        FULLFLASH::Partitions::Partitions::Ptr  partitions;

        std::string imei;
        std::string model;

        if (options.override_platform.length()) {
            platform    = FULLFLASH::StringToPlatform.at(options.override_platform);

            partitions = FULLFLASH::Partitions::Partitions::build(options.ff_path, platform, options.is_old_search_algorithm, options.search_start_adddress);

            model       = partitions->get_model();
        } else {
            partitions = FULLFLASH::Partitions::Partitions::build(options.ff_path, options.is_old_search_algorithm, options.search_start_adddress);

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

        if (options.is_debug) {
            dump_partitions_info(*partitions);
        } else {
            dump_partitions_short(*partitions);
        }

        if (options.is_partitions_search_only) {
            return EXIT_SUCCESS;
        }

        if (platform == FULLFLASH::Platform::SGOLD2_ELKA && !options.is_filesystem_scan_only) {
            bool is_continue = false;

            is_continue = Help::input_yn([]() {
                spdlog::warn("SGOLD2_ELKA platform unsupported yet. Continue (y/n)?");
            });

            if (!is_continue) {
                return EXIT_SUCCESS;
            }
        }

        Extractor extractor(partitions, platform, options.is_skip_broken, options.is_skip_dup, options.is_dump_data);

        if (!options.is_filesystem_scan_only) {
            spdlog::info("Destination path: {}", data_path.string());

            extractor.extract(data_path, options.is_overwrite);
        }

        spdlog::info("Done");
    } catch (const spdlog::spdlog_ex &e) {
        fmt::print("Log init error: {}", e.what());

        return -1;
    } catch (const FULLFLASH::BaseException &e) {
        spdlog::error("{}", e.what());

        return -2;
    } catch (const Patterns::Exception &e) {
        spdlog::error("{}", e.what());

        return -3;
    } catch (const std::filesystem::filesystem_error &e) {
        spdlog::error("Filesystem error: '{}' '{}' {}", e.path1().string(), e.path2().string(), e.what());

        return -4;
    } catch (const std::exception &e) {
        spdlog::error("{}", e.what());

        return -5;
    }

    return 0;
}
