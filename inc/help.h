#ifndef FFNIGHTMAN_HELP_H
#define FFNIGHTMAN_HELP_H

#include <string>
#include <functional>

namespace Help {

bool input_yn(std::function<void()> prompt);

};

#endif
