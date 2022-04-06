//
// Created by James Gallagher on 4/6/22.
//

#include "config.h"

#include <iostream>

#include "BESUtil.h"
#include "BESLog.h"

#include <libdap/DDS.h>
#include <libdap/DMR.h>

using namespace libdap;

namespace dap_utils {

static void log_request_and_memory_size_helper(long req_size) {
    long mem_size = BESUtil::get_current_memory_usage();    // size in KB or 0. jhrg 4/6/22
    if (mem_size) {
        INFO_LOG("request size: " << req_size << "KB|&|memory used by process: " << mem_size << "KB" << endl);
    }
    else {
        INFO_LOG("request size: " << req_size << "KB" << endl);
    }
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DDS to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dds Use this DDS to get the size of the request.
 */
void
log_request_and_memory_size(DDS *const *dds)
{
    long req_size = (long)(*dds)->get_request_size_kb(true);
    log_request_and_memory_size_helper(req_size);
}

/**
 * Log information about memory used by this request.
 *
 * Use the given DDS to log information about the request. As a bonus, log the
 * RSS for this process.
 *
 * @param dmr Use this DMR to get the size of the request.
 */
void
log_request_and_memory_size(/*const*/ DMR &dmr)
{
    // The request_size_kb() method is not marked const. Fix. jhrg 4/6/22
    long req_size = (long)dmr.request_size_kb(true);
    log_request_and_memory_size_helper(req_size);
}

}   // namespace dap_utils
