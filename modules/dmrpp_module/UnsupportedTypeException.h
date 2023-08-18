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
    UnsupportedTypeException() = delete;

public:
    explicit UnsupportedTypeException(std::string &msg): d_msg(msg){};
    UnsupportedTypeException(UnsupportedTypeException &) = default;
    ~UnsupportedTypeException() override = default;

    const char* what() const noexcept override { return d_msg.c_str(); };

};

} // namespace dmrpp

#endif //BES_UNSUPPORTEDTYPE_H
