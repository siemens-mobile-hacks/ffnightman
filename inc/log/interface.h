#ifndef FFNIGHTMAN_LOG_INTERFACE_H
#define FFNIGHTMAN_LOG_INTERFACE_H

#include <ffshit/log/interface.h>
#include <spdlog/spdlog.h>
#include <memory>

namespace Log {

class Interface : public FULLFLASH::Log::Interface {
    public:
        using Ptr = std::shared_ptr<Interface>;

        Interface() { }

        static Ptr build() {
            return std::make_shared<Interface>();
        }

        void on_info(std::string msg) override final {
            spdlog::info("{}", msg);
        }

        void on_warning(std::string msg) override final {
            spdlog::warn("{}", msg);
        }

        void on_error(std::string msg) override final {
            spdlog::error("{}", msg);
        }

        void on_debug(std::string msg) override final {
            spdlog::debug("{}", msg);
        }        


    private:
};

};

#endif
