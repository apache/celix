#pragma once

#include <exception>

namespace celix::rsa {

    /**
     * @brief Celix exception for remote services.
     */
    class RemoteServicesException : public std::exception {
    public:
        explicit RemoteServicesException(std::string msg) : w{std::move(msg)} {}

        const char* what() const noexcept override {
            return w.c_str();
        }
    private:
        const std::string w;
    };

}