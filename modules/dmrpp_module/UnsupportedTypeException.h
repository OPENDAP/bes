//
// Created by ndp on 8/18/23.
//


#ifndef BES_UNSUPPORTEDTYPE_H
#define BES_UNSUPPORTEDTYPE_H

#include "config.h"

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
    UnsupportedTypeException(std::string msg) : d_msg(std::move(msg)){};

    UnsupportedTypeException() = delete;
    UnsupportedTypeException(UnsupportedTypeException &e) { d_msg = e.d_msg; };
    ~UnsupportedTypeException() override = default;

    const char* what() const noexcept override { return d_msg.c_str(); };


};

} // namespace dmrpp

#endif //BES_UNSUPPORTEDTYPE_H
