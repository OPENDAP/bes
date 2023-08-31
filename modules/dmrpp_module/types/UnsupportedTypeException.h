//
// Created by ndp on 8/18/23.
//


#ifndef BES_UNSUPPORTEDTYPEEXCEPTION_H
#define BES_UNSUPPORTEDTYPEEXCEPTION_H

#include "config.h"

#include <exception>
#include <string>
#include <utility>

namespace dmrpp {

/**
 * @brief Thrown to prevent the production of dmr++ files that contain unsupported data types.
 */
class UnsupportedTypeException : public std::exception  {

private:
    std::string d_msg;

public:
    explicit UnsupportedTypeException(std::string msg) : d_msg(std::move(msg)){};
    UnsupportedTypeException() = delete;
    UnsupportedTypeException(const UnsupportedTypeException &e) noexcept = default;
    UnsupportedTypeException(UnsupportedTypeException &&e) noexcept = default;
    ~UnsupportedTypeException() override = default;

    UnsupportedTypeException& operator=(UnsupportedTypeException &&e) noexcept = default;
    UnsupportedTypeException& operator=(const UnsupportedTypeException &e) = default;

    const char* what() const noexcept override { return d_msg.c_str(); };
};

} // namespace dmrpp

#endif //BES_UNSUPPORTEDTYPEEXCEPTION_H
