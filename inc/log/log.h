#ifndef FFNIGHTMAN_LOG_LOG_H
#define FFNIGHTMAN_LOG_LOG_H

#include "log/interface.h"

#include <spdlog/spdlog.h>

#include <filesystem>

namespace Log {

void init();
void setup(std::filesystem::path dst_path = "");

};

#endif
