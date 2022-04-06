//
// Created by James Gallagher on 4/6/22.
//

#ifndef BES_DAPUTILS_H
#define BES_DAPUTILS_H

namespace libdap {
class DDS;
class DMR;
}

namespace dap_utils {

void log_request_and_memory_size(libdap::DDS *const *dds);

void log_request_and_memory_size(/*const*/ libdap::DMR &dmr);

}
#endif //BES_DAPUTILS_H
