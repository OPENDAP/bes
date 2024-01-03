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


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exception>
#include <sstream>      // std::stringstream
#include <thread>
#include <future>

#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/BaseType.h>
#include <libdap/escaping.h>

#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <BESUtil.h>
#include <TempFile.h>
#include <RequestServiceTimer.h>

#include <BESLog.h>
#include <BESError.h>
#include <BESInternalFatalError.h>
#include <BESDapError.h>
#include "BESDMRResponse.h"
#include <stringbuffer.h>

#include "FONcBaseType.h"
#include "FONcRequestHandler.h"
#include "FONcTransmitter.h"
#include "FONcTransform.h"

using namespace libdap;
using namespace std;
using namespace rapidjson;

#define MODULE "fonc"
#define prolog string("FONcTransmitter::").append(__func__).append("() - ")


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
    add_method(DATA_SERVICE, FONcTransmitter::send_dap2_data);
    add_method(DAP4DATA_SERVICE, FONcTransmitter::send_dap4_data);
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
void FONcTransmitter::send_dap2_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDEBUG(MODULE,  prolog << "BEGIN" << endl);

    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015
        auto bdds = dynamic_cast<BESDataDDSResponse *>(obj);
        if (!bdds) throw BESInternalFatalError("Expected a BESDataDDSResponse instance", __FILE__, __LINE__);
        auto dds = bdds->get_dds();

        string base_name = dds->filename().substr(dds->filename().find_last_of("/\\") + 1);

        // This object closes the file when it goes out of scope.
        bes::TempFile temp_file;
        string temp_file_name = temp_file.create(FONcRequestHandler::temp_dir, "dap2_nc_"+base_name);

        BESDEBUG(MODULE,  prolog << "Building response file " << temp_file_name << endl);

        ostream &strm = dhi.get_output_stream();
        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        BESDEBUG(MODULE,  prolog << "Transmitting temp file " << temp_file_name << endl);

        // Note that 'RETURN_CMD' is the same as the string that determines the file type:
        // netcdf 3 or netcdf 4. Hack. jhrg 9/7/16
        FONcTransform ft(obj, &dhi, temp_file_name, dhi.data[RETURN_CMD]);

        ft.transform_dap2(strm);
    }
    // This series of catch blocks is used to convert other errors into BESErrors.
    // Thus, we do not need to catch BESError here because it's already what we want.
    catch (const Error &e) {
        throw BESDapError(prolog + "Failed to read data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (const std::exception &e) {
        throw BESInternalError(prolog + "Failed to read data! Caught std::exception. Message; " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError(prolog + "Failed to get read data: Unknown exception caught", __FILE__, __LINE__);
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
    uint64_t bytes_sent = 0;
    try { // Expanded try block so all DAP errors are caught. ndp 12/23/2015

        auto bdmr = dynamic_cast<BESDMRResponse *>(obj);
        if (!bdmr) throw BESInternalFatalError("Expected a BESDMRResponse instance", __FILE__, __LINE__);
        auto dmr = bdmr->get_dmr();

        string base_name = dmr->filename().substr(dmr->filename().find_last_of("/\\") + 1);

        // This object closes the file when it goes out of scope.
        bes::TempFile temp_file;
        string temp_file_name = temp_file.create(FONcRequestHandler::temp_dir,  "dap4_nc_"+base_name);

        BESDEBUG(MODULE,  prolog << "Building response file " << temp_file_name << endl);
        // Note that 'RETURN_CMD' is the same as the string that determines the file type:
        // netcdf 3 or netcdf 4. Hack. jhrg 9/7/16
        // FONcTransform ft(loaded_dmr, dhi, temp_file.get_name(), dhi.data[RETURN_CMD]);
        FONcTransform ft(obj, &dhi, temp_file_name, dhi.data[RETURN_CMD]);

        // Call the transform function for DAP4.
        ft.transform_dap4();

        ostream &strm = dhi.get_output_stream();

#if !NDEBUG
        stringstream msg;
        msg << prolog << "Using ostream: " << (void *) &strm << endl;
        BESDEBUG(MODULE,  msg.str());
        INFO_LOG( msg.str());
#endif

        if (!strm) throw BESInternalError("Output stream is not set, can not return as", __FILE__, __LINE__);

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired("ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        BESDEBUG(MODULE,  prolog << "Transmitting temp file " << temp_file_name << endl);

        // FONcTransmitter::write_temp_file_to_stream(temp_file.get_fd(), strm); //, loaded_dds->filename(), ncVersion);
        bytes_sent = BESUtil::file_to_stream(temp_file_name,strm);
    }
    // This series of catch blocks is used to convert other errors into BESErrors.
    // Thus, we do not need to catch BESError here because it's already what we want.
    catch (const Error &e) {
        throw BESDapError(prolog + "Failed to read data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (const std::exception &e) {
        throw BESInternalError(prolog + "Failed to read data! Caught std::exception. Message: " + string(e.what()), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError(prolog + "Failed to get read data: Unknown exception caught", __FILE__, __LINE__);
    }

    BESDEBUG(MODULE,  prolog << "END  Transmitted as netcdf. bytes_sent: " << bytes_sent << endl);
}






