#include "help.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#include <ffshit/system.h>

namespace Help {

bool input_yn(std::function<void()> prompt) {
    std::string yes_no;

    while (yes_no != "n" && yes_no != "y") {
        prompt();

        yes_no.clear();
        std::cin >> yes_no;

        System::to_lower(yes_no);
    }

    return yes_no == "y";
}

std::string get_datetime() {
#if defined(__APPLE__)
    auto now        = std::chrono::system_clock::now();
    auto timestamp  = std::chrono::system_clock::to_time_t(now);
#else
    auto now        = std::chrono::high_resolution_clock::now();
    auto timestamp  = std::chrono::high_resolution_clock::to_time_t(now);
#endif

    auto tm         = *std::localtime(&timestamp);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");

    return oss.str();
}

#if defined(_WIN64) && defined(_MSC_VER )
void set_utf8_locale() {
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

};
