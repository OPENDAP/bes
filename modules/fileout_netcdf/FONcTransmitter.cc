// FONcTransmitter.cc

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <exception>
#include <sstream>      // std::stringstream
#include <libgen.h>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>
#include <ConstraintEvaluator.h>

#include <TheBESKeys.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include <BESDapResponseBuilder.h>

#include <BESError.h>
#include <BESDapError.h>
#include <BESForbiddenError.h>
#include <BESInternalFatalError.h>
#include <DapFunctionUtils.h>

#include "FONcBaseType.h"
#include "FONcRequestHandler.h"
#include "FONcTransmitter.h"
#include "FONcTransform.h"

using namespace ::libdap;
using namespace std;

// size of the buffer used to read from the temporary file built on disk and
// send data to the client over the network connection (socket/stream)
#define OUTPUT_FILE_BLOCK_SIZE 4096

/** @brief Construct the FONcTransmitter, adding it with name netcdf to be
 * able to transmit a data response
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as a netcdf file.
 *
 * The OPeNDAP data object is written to a netcdf file locally in a
 * temporary directory specified by the BES configuration parameter
 * FONc.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FONC_TEMP_DIR.
 */
FONcTransmitter::FONcTransmitter() :
    BESBasicTransmitter()
{
    add_method(DATA_SERVICE, FONcTransmitter::send_data);
}

/**
 * Hack to ensure the file descriptor for the temporary file is closed.
 */
struct wrap_temp_descriptor {
    int d_fd;
    wrap_temp_descriptor(int fd) : d_fd(fd) {}
    ~wrap_temp_descriptor() { close(d_fd); }
};

/**
 * Hack to ensure that the temporary file name used with mkstemp() will
 * be 'unlinked' no matter how we exit.
 */
struct wrap_temp_name {
    vector<char> d_name;
    wrap_temp_name(vector<char> &name) : d_name(name) {}
    ~wrap_temp_name() { unlink(&d_name[0]); }
};

/**
 * Process the "history" attribute.
 * We add:
 *  - Sub-setting information if any
 *  - SSFunction invocations
 *  - ResourceID? URL?
 *
 * @param dds The DDS to modify
 * @param ce The constraint expression that produced this new netCDF file.
 */
void updateHistoryAttribute(DDS *dds, const string ce)
{
    bool foundIt = false;
    string cf_history_entry = BESContextManager::TheManager()->get_context("cf_history_entry", foundIt);
    if (!foundIt) {
        // This code will be used only when the 'cf_histroy_context' is not set,
        // which should be never in an operating server. However, when we are
        // testing, often only the besstandalone code is running and the existing
        // baselines don't set the context, so we have this. It must do something
        // so the tests are not hopelessly obscure and filter out junk that varies
        // by host (e.g., the names of cached files that have been decompressed).
        // jhrg 6/3/16

        string request_url = dds->filename();
        // remove path info
        request_url = request_url.substr(request_url.find_last_of('/')+1);
        // remove 'uncompress' cache mangling
        request_url = request_url.substr(request_url.find_last_of('#')+1);
        request_url += "?" + ce;

        std::stringstream ss;

        time_t raw_now;
        struct tm * timeinfo;
        time(&raw_now); /* get current time; same as: timer = time(NULL)  */
        timeinfo = localtime(&raw_now);

        char time_str[100];
        // 2000-6-1 6:00:00
        strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", timeinfo);

        ss << time_str << " " << "Hyrax" << " " << request_url;
        cf_history_entry = ss.str();
    }

    BESDEBUG("fonc",
        "FONcTransmitter::updateHistoryAttribute() - Adding cf_history_entry context. '" << cf_history_entry << "'" << endl);

    vector<string> hist_entry_vec;
    hist_entry_vec.push_back(cf_history_entry);
    BESDEBUG("fonc",
        "FONcTransmitter::updateHistoryAttribute() - hist_entry_vec.size(): " << hist_entry_vec.size() << endl);

    // Add the new entry to the "history" attribute
    // Get the top level Attribute table.
    AttrTable &globals = dds->get_attr_table();

    // Since many files support "CF" conventions the history tag may already exist in the source data
    // and we should add an entry to it if possible.
    bool done = false; // Used to indicate that we located a toplevel ATtrTable whose name ends in "_GLOBAL" and that has an existing "history" attribute.
    unsigned int num_attrs = globals.get_size();
    if (num_attrs) {
        // Here we look for a top level AttrTable whose name ends with "_GLOBAL" which is where, by convention,
        // data ingest handlers place global level attributes found in the source dataset.
        AttrTable::Attr_iter i = globals.attr_begin();
        AttrTable::Attr_iter e = globals.attr_end();
        for (; i != e && !done; i++) {
            AttrType attrType = globals.get_attr_type(i);
            string attr_name = globals.get_name(i);
            // Test the entry...
            if (attrType == Attr_container && BESUtil::endsWith(attr_name, "_GLOBAL")) {
                // Look promising, but does it have an existing "history" Attribute?
                AttrTable *source_file_globals = globals.get_attr_table(i);
                AttrTable::Attr_iter history_attrItr = source_file_globals->simple_find("history");
                if (history_attrItr != source_file_globals->attr_end()) {
                    // Yup! Add our entry...
                    BESDEBUG("fonc",
                        "FONcTransmitter::updateHistoryAttribute() - Adding history entry to " << attr_name << endl);
                    source_file_globals->append_attr("history", "string", &hist_entry_vec);
                    done = true;
                }
            }
        }
    }

    if (!done) {
        // We never found an existing location to place the "history" entry, so we'll just stuff it into the top level AttrTable.
        BESDEBUG("fonc",
            "FONcTransmitter::updateHistoryAttribute() - Adding history entry to top level AttrTable" << endl);
        globals.append_attr("history", "string", &hist_entry_vec);

    }
}

/**
 * @brief The static method registered to transmit OPeNDAP data objects as
 * a netcdf file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a netcdf file, and
 * streams back that netcdf file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a netcdf file, or
 * streaming the netcdf file
 */
void FONcTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG("fonc", "FONcTransmitter::send_data() - BEGIN" << endl);

    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015
        BESDapResponseBuilder responseBuilder;
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16

        BESDEBUG("fonc", "FONcTransmitter::send_data() - Reading data into DataDDS" << endl);

        DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);

        // ResponseBuilder splits the CE, so use the DHI or make two calls and
        // glue the result together: responseBuilder.get_btp_func_ce() + " " + responseBuilder.get_ce()
        // jhrg 9/6/16
        updateHistoryAttribute(loaded_dds, dhi.data[POST_CONSTRAINT]);

        // TODO Make this code and the two struct classes that wrap the name a fd part of
        // a utility class or file. jhrg 9/7/16

        string temp_file_name = FONcRequestHandler::temp_dir + "/ncXXXXXX";
        vector<char> temp_file(temp_file_name.length() + 1);
        string::size_type len = temp_file_name.copy(&temp_file[0], temp_file_name.length());
        temp_file[len] = '\0';
        // cover the case where older versions of mkstemp() create the file using
        // a mode of 666.
        mode_t original_mode = umask(077);
        int fd = mkstemp(&temp_file[0]);
        umask(original_mode);

        // Hack: Wrap the name and file descriptors so that the descriptor is closed
        // and temp file in unlinked no matter hoe we exit. jhrg 9/7/16
        wrap_temp_name w_temp_file(temp_file);
        wrap_temp_descriptor w_fd(fd);

        if (fd == -1) throw BESInternalError("Failed to open the temporary file.", __FILE__, __LINE__);

        BESDEBUG("fonc", "FONcTransmitter::send_data - Building response file " << &temp_file[0] << endl);

        // Note that 'RETURN_CMD' is the same as the string that determines the file type:
        // netcdf 3 or netcdf 4. Hack. jhrg 9/7/16
        FONcTransform ft(loaded_dds, dhi, &temp_file[0], dhi.data[RETURN_CMD]);
        ft.transform();

        ostream &strm = dhi.get_output_stream();
        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        BESDEBUG("fonc", "FONcTransmitter::send_data - Transmitting temp file " << &temp_file[0] << endl);

        FONcTransmitter::write_temp_file_to_stream(fd, strm); //, loaded_dds->filename(), ncVersion);
    }
    catch (Error &e) {
        throw BESDapError("Failed to read data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (std::exception &e) {
        throw BESInternalError("Failed to read data: STL Error: " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to get read data: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG("fonc", "FONcTransmitter::send_data - done transmitting to netcdf" << endl);
}

/** @brief stream the temporary netcdf file back to the requester
 *
 * Streams the temporary netcdf file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void FONcTransmitter::write_temp_file_to_stream(int fd, ostream &strm) //, const string &filename, const string &ncVersion)
{
    char block[OUTPUT_FILE_BLOCK_SIZE];

    int nbytes = read(fd, block, sizeof block);
    while (nbytes > 0) {
        strm.write(block, nbytes /*os.gcount()*/);
        nbytes = read(fd, block, sizeof block);
    }
}

