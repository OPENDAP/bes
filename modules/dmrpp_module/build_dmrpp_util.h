//
// Created by James Gallagher on 4/19/22.
//

#ifndef BES_BUILD_DMRPP_UTIL_H
#define BES_BUILD_DMRPP_UTIL_H

#include <string>

namespace dmrpp {
class DMRpp;
}

namespace build_dmrpp_util {
void add_chunk_information(const std::string &h5_file_name, dmrpp::DMRpp *dmrpp);
extern bool verbose;
}

#endif //BES_BUILD_DMRPP_UTIL_H
