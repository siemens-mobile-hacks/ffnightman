#include <ffshit/ex.h>
#include <ffshit/system.h>
#include <ffshit/filesystem/ex.h>
#include <ffshit/fullflash.h>
#include <ffshit/partition/partitions.h>
#include <ffshit/partition/ex.h>

#include "log/log.h"
#include "cli/options.h"
#include "extractor.h"
#include "help.h"
#include <csignal>

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

            spdlog::debug("  {:8s} Addr: 0x{:08X}, size: {}, Unk1: {:04X}, Unk2: {:04X}, Unk3: {:08X}, Unk4: {:04X}",
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

    spdlog::info("Found {} FFS partitions", p_map.size());

    for (const auto &pair : p_map) {
        spdlog::info("  {:8s} {}", pair.first, pair.second.get_blocks().size());
    }
}

static bool setup_destination_path(std::filesystem::path path, bool overwrite) {
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
            return false;
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
            return false;
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

    return true;
}

int main(int argc, char *argv[]) {
    try {
        Log::init();
        Log::setup();
    } catch (const spdlog::spdlog_ex &e) {
        fmt::print("Log init error: {}\n", e.what());

        return EXIT_FAILURE;
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

    if (options.verbose_headers || options.verbose_data) {
        spdlog::set_level(spdlog::level::debug);
    }

#if defined(_WIN64)
    SetConsoleOutputCP(CP_UTF8);
#endif

#if defined(_WIN64) && defined(_MSC_VER)
    Help::set_utf8_locale();
#endif

    std::filesystem::path data_path = fmt::format("{}_data", options.ff_path.filename().string());

    if (options.override_dst_path.length() != 0) {
        data_path = options.override_dst_path;
    }

    try {
        if (options.is_log_to_file) {
            if (!setup_destination_path(data_path, options.is_overwrite)) {
                return EXIT_SUCCESS;
            }

            Log::setup(data_path);

            if (options.verbose_headers || options.verbose_data) {
                spdlog::set_level(spdlog::level::debug);
            }
        }

        FULLFLASH::Platform::Type               platform;
        FULLFLASH::FULLFLASH::Ptr               fullflash;
        FULLFLASH::Partitions::Partitions::Ptr  partitions;

        std::string imei;
        std::string model;

        if (options.override_platform.length()) {
            platform    = FULLFLASH::Platform::StringToType.at(options.override_platform);
    
            fullflash = FULLFLASH::FULLFLASH::build(options.ff_path, platform);
        } else {
            fullflash = FULLFLASH::FULLFLASH::build(options.ff_path);
        }

        const auto &detector = fullflash->get_detector();

        model       = detector.get_model();
        imei        = detector.get_imei();
        platform    = detector.get_platform();

        if (platform == FULLFLASH::Platform::Type::UNK) {
            throw FULLFLASH::Exception("Unknown platform");
        }

        spdlog::info("Platform:    {}", FULLFLASH::Platform::TypeToString.at(platform));

        if (model.length()) {
            spdlog::info("Model:       {}", model);
        }

        if (imei.length()) {
            spdlog::info("IMEI:        {}", imei);
        }

        fullflash->load_partitions(options.is_old_search_algorithm, options.search_start_adddress);

        partitions = fullflash->get_partitions();

        if (options.verbose_headers) {
            dump_partitions_info(*partitions);
        } else {
            dump_partitions_short(*partitions);
        }

        if (options.is_partitions_search_only) {
            return EXIT_SUCCESS;
        }

        if (options.override_fs_platform.length()) {
            platform = FULLFLASH::Platform::StringToType.at(options.override_fs_platform);

            spdlog::warn("FS platform: {}", FULLFLASH::Platform::TypeToString.at(platform));
        } else {
            if (partitions->get_fs_platform() != detector.get_platform()) {
                platform = partitions->get_fs_platform();

                spdlog::warn("FS platform: {}", FULLFLASH::Platform::TypeToString.at(platform));
            } else {
                spdlog::info("FS platform: {}", FULLFLASH::Platform::TypeToString.at(platform));
            }
        }

        Extractor extractor(partitions, platform, options);

        if (!options.is_filesystem_scan_only) {
            if (!options.is_list_only) {
                spdlog::info("Destination path: {}", data_path.string());
            }

            if (!options.is_log_to_file && !options.is_list_only) {
                if (!setup_destination_path(data_path, options.is_overwrite)) {
                    return EXIT_SUCCESS;
                }
            }

            if (options.is_list_only) {
                extractor.list(options.ls_regex);
            } else {
                extractor.extract(data_path, options.regexp, options.is_overwrite);
            }
        }

        spdlog::info("Done");
    } catch (const spdlog::spdlog_ex &e) {
        fmt::print("Log init error: {}\n", e.what());

        return EXIT_FAILURE;
    } catch (const FULLFLASH::BaseException &e) {
        spdlog::error("{}", e.what());

        return EXIT_FAILURE;
    } catch (const Patterns::Exception &e) {
        spdlog::error("{}", e.what());

        return EXIT_FAILURE;
    } catch (const std::filesystem::filesystem_error &e) {
        spdlog::error("Filesystem error: '{}' '{}' {}", e.path1().string(), e.path2().string(), e.what());

        return EXIT_FAILURE;
    } catch (const std::regex_error &e) {
        spdlog::error("Regexp error: {}", e.what());

		 return EXIT_FAILURE;
    } catch (const std::exception &e) {
        spdlog::error("{}", e.what());

        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
