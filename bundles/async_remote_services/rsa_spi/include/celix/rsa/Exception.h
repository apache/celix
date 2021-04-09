#pragma once

#include <exception>

namespace celix::rsa {

    /**
     * @brief Celix Remote Service Admin Exception
     */
    class Exception : public std::exception {
    public:
        explicit Exception(std::string msg) : w{std::move(msg)} {}

        const char* what() const noexcept override {
            return w.c_str();
        }
    private:
        const std::string w;
    };

}