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
//      kyang       Kent Yang <myang6@hdfgroup.org> (for DAP4/netCDF-4 enhancement)

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

#include <D4Group.h>
#include <D4Attributes.h>
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
#include <TempFile.h>

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

using namespace libdap;
using namespace std;

#define MODULE "fonc"
#define prolog string("FONcTransmitter::").append(__func__).append("() - ")

#if 0 // Moved to BESUtil.cc
// size of the buffer used to read from the temporary file built on disk and
// send data to the client over the network connection (socket/stream)
// #define OUTPUT_FILE_BLOCK_SIZE 4096
#endif

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
        BESTransmitter()
{
    add_method(DATA_SERVICE, FONcTransmitter::send_data);
    add_method(DAP4DATA_SERVICE, FONcTransmitter::send_dap4_data);
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
 * Creates the "history" text appended to source attribute "history".
 * We add:
 *  - Sub-setting information if any
 *  - SSFunction invocations
 *  - ResourceID? URL?
 *  - JSON?
 * @param request_url
 */
string create_history_txt(const string &request_url)
{
    // This code will be used only when the 'cf_histroy_context' is not set,
    // which should be never in an operating server. However, when we are
    // testing, often only the besstandalone code is running and the existing
    // baselines don't set the context, so we have this. It must do something
    // so the tests are not hopelessly obscure and filter out junk that varies
    // by host (e.g., the names of cached files that have been decompressed).
    // jhrg 6/3/16

    string cf_history_entry;
    std::stringstream ss;
    time_t raw_now;
    struct tm *timeinfo;
    time(&raw_now); /* get current time; same as: timer = time(NULL)  */
    timeinfo = localtime(&raw_now);

    char time_str[100];
    strftime(time_str, 100, "%Y-%m-%d %H:%M:%S", timeinfo);

    ss << time_str << " " << "Hyrax" << " " << request_url;
    cf_history_entry = ss.str();
    BESDEBUG(MODULE, prolog << "Adding cf_history_entry context. '" << cf_history_entry << "'" << endl);
    return cf_history_entry;
}

/**
* Gets the "history" attribute context.
*
* @request_url
*/
vector<string> get_history_entry (const string &request_url)
{
    vector<string> hist_entry_vec;
    bool foundIt = false;
    string cf_history_entry = BESContextManager::TheManager()->get_context("cf_history_entry", foundIt);
    if (!foundIt) {
        // If the cf_history_entry context was not set by the incoming command then
        // we compute and the value of the history string here.
        cf_history_entry = create_history_txt(request_url);
    }
    // And here we add to the returned vector.
    hist_entry_vec.push_back(cf_history_entry);
    return hist_entry_vec;
}

/**
 * Process the DAP2 "history" attribute.
 *
 * @param dds The DDS to modify
 * @param ce The constraint expression that produced this new netCDF file.
 */
void updateHistoryAttribute(DDS *dds, const string &ce)
{
    string request_url = dds->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/')+1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#')+1);
    if(ce != "") request_url += "?" + ce;

    std::vector<std::string> hist_entry_vec = get_history_entry(request_url);

    BESDEBUG(MODULE, prolog << "hist_entry_vec.size(): " << hist_entry_vec.size() << endl);

    // Add the new entry to the "history" attribute
    // Get the top level Attribute table.
    AttrTable &globals = dds->get_attr_table();

    // Since many files support "CF" conventions the history tag may already exist in the source data
    // and we should add an entry to it if possible.
    bool added_history = false; // Used to indicate that we located a toplevel AttrTable whose name ends in "_GLOBAL" and that has an existing "history" attribute.
    unsigned int num_attrs = globals.get_size();
    if (num_attrs) {
        // Here we look for a top level AttrTable whose name ends with "_GLOBAL" which is where, by convention,
        // data ingest handlers place global level attributes found in the source dataset.
        auto i = globals.attr_begin();
        auto e = globals.attr_end();
        for (; i != e; i++) {
            AttrType attrType = globals.get_attr_type(i);
            string attr_name = globals.get_name(i);
            // Test the entry...
            if (attrType == Attr_container && BESUtil::endsWith(attr_name, "_GLOBAL")) {
                // We are going to append to an existing history attribute if there is one
                // Or just add a histiry attribute if there is not one. In a most
                // handy API moment, append_attr() does just this.
                AttrTable *global_attr_tbl = globals.get_attr_table(i);
                global_attr_tbl->append_attr("history", "string", &hist_entry_vec);
                added_history = true;
                BESDEBUG(MODULE, prolog << "Added history entry to " << attr_name << endl);
            }
        }
        if(!added_history){
            auto dap_global_at = globals.append_container("DAP_GLOBAL");
            dap_global_at->set_name("DAP_GLOBAL");
            dap_global_at->append_attr("history", "string", &hist_entry_vec);
            BESDEBUG(MODULE, prolog << "No top level AttributeTable name matched '*_GLOBAL'. "
                                       "Created DAP_GLOBAL AttributeTable and added history Attribute to it." << endl);
        }
    }
}

/**
* Process the DAP4 "history" attribute.
*
* @param dmr The DMR to modify
* @param ce The constraint expression that produced this new netCDF file.
*/
void updateHistoryAttribute(DMR *dmr, const string &ce)
{
    string request_url = dmr->filename();
    // remove path info
    request_url = request_url.substr(request_url.find_last_of('/')+1);
    // remove 'uncompress' cache mangling
    request_url = request_url.substr(request_url.find_last_of('#')+1);
    if(ce != "") request_url += "?" + ce;
    vector<string> hist_entry_vector = get_history_entry(request_url);

    BESDEBUG(MODULE, prolog << "hist_entry_vec.size(): " << hist_entry_vector.size() << endl);
    bool added_history = false;
    D4Group* root_grp = dmr->root();
    D4Attributes *root_attrs = root_grp->attributes();
    for (auto attrs = root_attrs->attribute_begin(); attrs != root_attrs->attribute_end(); ++attrs) {
        string name = (*attrs)->name();
        BESDEBUG(MODULE, prolog << "Attribute name is "<<name <<endl);
        if ((*attrs)->type() && BESUtil::endsWith(name, "_GLOBAL")) {
            // Yup! Add our entry...
            D4Attribute *history_attr = (*attrs)->attributes()->find("history");
            if (!history_attr) {
                //if there is no source history attribute
                BESDEBUG(MODULE, prolog << "Adding history entry to " << name << endl);
                auto *new_history = new D4Attribute("history", attr_str_c);
                new_history->add_value_vector(hist_entry_vector);
                (*attrs)->attributes()->add_attribute(new_history);
            } else {
                (*attrs)->attributes()->find("history")->add_value_vector(hist_entry_vector);
            }
            added_history = true;
        }
    }
    if(!added_history){
        auto *dap_global = new D4Attribute("DAP_GLOBAL",attr_container_c);
        root_attrs->add_attribute(dap_global);
        auto *new_history = new D4Attribute("history", attr_str_c);
        new_history->add_value_vector(hist_entry_vector);
        dap_global->attributes()->add_attribute(new_history);
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
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015

#if 0
        BESDapResponseBuilder responseBuilder;
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16

        // Now that we are ready to start reading the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        BESDEBUG(MODULE,  prolog << "Reading data into DataDDS" << endl);
        DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);

        // ResponseBuilder splits the CE, so use the DHI or make two calls and
        // glue the result together: responseBuilder.get_btp_func_ce() + " " + responseBuilder.get_ce()
        // jhrg 9/6/16
        updateHistoryAttribute(loaded_dds, dhi.data[POST_CONSTRAINT]);
#endif

        // This object closes the file when it goes out of scope.
        bes::TempFile temp_file(FONcRequestHandler::temp_dir + "/ncXXXXXX");

        BESDEBUG(MODULE,  prolog << "Building response file " << temp_file.get_name() << endl);
        // Note that 'RETURN_CMD' is the same as the string that determines the file type:
        // netcdf 3 or netcdf 4. Hack. jhrg 9/7/16
        FONcTransform ft(obj, &dhi, temp_file.get_name(), dhi.data[RETURN_CMD]);
        ft.transform();

        ostream &strm = dhi.get_output_stream();
        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        BESDEBUG(MODULE,  prolog << "Transmitting temp file " << temp_file.get_name() << endl);

        // FONcTransmitter::write_temp_file_to_stream(temp_file.get_fd(), strm); //, loaded_dds->filename(), ncVersion);
        BESUtil::file_to_stream(temp_file.get_name(),strm);
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

    BESDEBUG(MODULE,  prolog << "END Transmitted as netcdf" << endl);
}

/**
 * Follow the send_data() 
 * @brief The static method registered to transmit OPeNDAP data objects as
 * a netcdf file.
 *
 * This function takes the OPeNDAP DMR object, reads in the data (can be
 * used with any data handler), transforms the data into a netcdf file, and
 * streams back that netcdf file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DMR or if
 * there are any problems reading the data, writing to a netcdf file, or
 * streaming the netcdf file
 */
void FONcTransmitter::send_dap4_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015
        BESDapResponseBuilder responseBuilder;
        // Use the DDS from the ResponseObject along with the parameters
        // from the DataHandlerInterface to load the DDS with values.
        // Note that the BESResponseObject will manage the loaded_dds object's
        // memory. Make this a shared_ptr<>. jhrg 9/6/16

        // Now that we are ready to start reading the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        BESDEBUG(MODULE,  prolog << "Reading data into DMR" << endl);
        //DDS *loaded_dds = responseBuilder.intern_dap2_data(obj, dhi);
        DMR *loaded_dmr = responseBuilder.intern_dap4_data(obj, dhi);
        updateHistoryAttribute(loaded_dmr, dhi.data[POST_CONSTRAINT]);

        // This object closes the file when it goes out of scope.
        bes::TempFile temp_file(FONcRequestHandler::temp_dir + "/ncXXXXXX");

        BESDEBUG(MODULE,  prolog << "Building response file " << temp_file.get_name() << endl);
        // Note that 'RETURN_CMD' is the same as the string that determines the file type:
        // netcdf 3 or netcdf 4. Hack. jhrg 9/7/16
        FONcTransform ft(loaded_dmr, dhi, temp_file.get_name(), dhi.data[RETURN_CMD]);

        // Call the transform function for DAP4.
        ft.transform_dap4();

        ostream &strm = dhi.get_output_stream();
        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        BESDEBUG(MODULE,  prolog << "Transmitting temp file " << temp_file.get_name() << endl);

        // FONcTransmitter::write_temp_file_to_stream(temp_file.get_fd(), strm); //, loaded_dds->filename(), ncVersion);
        BESUtil::file_to_stream(temp_file.get_name(),strm);
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

    BESDEBUG(MODULE,  prolog << "END  Transmitted as netcdf" << endl);
}

#if 0  // Moved to BESUtil.cc
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
#endif