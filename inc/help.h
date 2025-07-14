#ifndef FFNIGHTMAN_HELP_H
#define FFNIGHTMAN_HELP_H

#include <string>
#include <functional>

namespace Help {

bool        input_yn(std::function<void()> prompt);
std::string get_datetime();

#if defined(_WIN64) && defined(_MSC_VER )
void        set_utf8_locale();
#endif

};

#endif
