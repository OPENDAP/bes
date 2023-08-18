//
// Created by ndp on 8/18/23.
//

#include "config.h"

#include <string>
#include <utility>

#ifndef BES_UNSUPPORTEDVARIABLELENGTHSTRINGARRAY_H
#define BES_UNSUPPORTEDVARIABLELENGTHSTRINGARRAY_H

namespace dmrpp {

/**
 * @brief Used to prevent the production of dmr++ files that contain arrays of variable length strings, which are not supported.
 */
class UnsupportedVariableLengthStringArrayException : public std::exception  {

private:
    const char *d_msg = ("UnsupportedVariableLengthStringArrayException: Your data granule contains an array of "
                         "variable length strings (AVLS). This data architecture is not currently supported by the "
                         "dmr++ machinery. One solution available to you is to rewrite the granule so that these "
                         "arrays are represented as arrays of fixed length strings (AFLS). While these may not be as "
                         "'elegant' as AVLS, the ragged ends of the AFLS compress well, so the storage penalty is "
                         "minimal.");

public:
    UnsupportedVariableLengthStringArrayException() = default;
    const char* what() const noexcept override { return d_msg; };

};

} // namespace dmrpp

#endif //BES_UNSUPPORTEDVARIABLELENGTHSTRINGARRAY_H
