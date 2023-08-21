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
    explicit UnsupportedTypeException(const std::string &msg) : d_msg(msg){};
    UnsupportedTypeException() = delete;
    UnsupportedTypeException(const UnsupportedTypeException &e) : d_msg(e.d_msg){ };
    ~UnsupportedTypeException() override = default;

    const char* what() const noexcept override { return d_msg.c_str(); };

};

} // namespace dmrpp

#endif //BES_UNSUPPORTEDTYPEEXCEPTION_H
