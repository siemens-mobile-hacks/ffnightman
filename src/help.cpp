#include "help.h"

#include <spdlog/spdlog.h>

#include <iostream>
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

};
