#pragma once

#include <exception>

namespace celix {

    /**
     * @brief Celix runtime Exception
     */
    class Exception : public std::exception {
    public:
        explicit Exception(std::string msg) : w{std::move(msg)} {}

        Exception(const Exception&) = default;
        Exception(Exception&&) = default;
        Exception& operator=(const Exception&) = default;
        Exception& operator=(Exception&&) = default;

        [[nodiscard]] const char* what() const noexcept override {
            return w.c_str();
        }
    private:
        std::string w;
    };

}
