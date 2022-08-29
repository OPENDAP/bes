// JPEG2000Transmitter.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

#include "config.h"

#include <unistd.h>

#include <cstdio>
#include <cstdlib>

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <libdap/DataDDS.h>
#include <libdap/BaseType.h>
#include <libdap/escaping.h>

using namespace libdap;

#include "JPEG2000Transmitter.h"
#include "FONgTransform.h"

#include <BESUtil.h>
#include <BESInternalError.h>
#include <BESDapError.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>
#include <TempFile.h>
#include <DapFunctionUtils.h>

#include <TheBESKeys.h>

#define JPEG2000_TEMP_DIR "/tmp"
#define JPEG2000_GCS "WGS84"

string JPEG2000Transmitter::temp_dir;
string JPEG2000Transmitter::default_gcs;

/** @brief Construct the JPEG2000Transmitter, adding it with name geotiff to be
 * able to transmit a data response
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as a geotiff file.
 *
 * The OPeNDAP data object is written to a geotiff file locally in a
 * temporary directory specified by the BES configuration parameter
 * JPEG2000.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition JPEG2000_TEMP_DIR.
 *
 * @note The mapping from a 'returnAs' of "geotiff" to this code
 * is made in the JPEG2000Module class.
 *
 * @see JPEG2000Module
 */
JPEG2000Transmitter::JPEG2000Transmitter() :  BESTransmitter()
{
    // DATA_SERVICE == "dods"
    add_method(DATA_SERVICE, JPEG2000Transmitter::send_data_as_jp2);

    if (JPEG2000Transmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "JPEG2000.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, JPEG2000Transmitter::temp_dir, found);
        if (!found || JPEG2000Transmitter::temp_dir.empty()) {
            JPEG2000Transmitter::temp_dir = JPEG2000_TEMP_DIR;
        }
        string::size_type len = JPEG2000Transmitter::temp_dir.size();
        if (JPEG2000Transmitter::temp_dir[len - 1] == '/') {
            JPEG2000Transmitter::temp_dir = JPEG2000Transmitter::temp_dir.substr(0, len - 1);
        }
    }

    if (JPEG2000Transmitter::default_gcs.empty()) {
        // Use what as the default Geographic coordinate system?
        bool found = false;
        string key = "JPEG2000.Default_GCS";
        TheBESKeys::TheKeys()->get_value(key, JPEG2000Transmitter::default_gcs, found);
        if (!found || JPEG2000Transmitter::default_gcs.empty()) {
            JPEG2000Transmitter::default_gcs = JPEG2000_GCS;
        }
    }
}

/** @brief The static method registered to transmit OPeNDAP data objects as
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
void JPEG2000Transmitter::send_data_as_jp2(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("cast error", __FILE__, __LINE__);
    }

    DDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    ostream &strm = dhi.get_output_stream();
    if (!strm)
    	throw BESInternalError("Output stream is not set, cannot return as", __FILE__, __LINE__);

    BESDEBUG("JPEG20002", "JPEG2000Transmitter::send_data - parsing the constraint" << endl);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        bdds->get_ce().parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        throw BESDapError("Failed to parse the constraint expression: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (...) {
        throw BESInternalError("Failed to parse the constraint expression: Unknown exception caught", __FILE__, __LINE__);
    }

    // now we need to read the data
    BESDEBUG("JPEG20002", "JPEG2000Transmitter::send_data - reading data into DataDDS" << endl);

    bes::TempFile temp_file;
    string temp_file_name = temp_file.create(JPEG2000Transmitter::temp_dir, "jp2000_");
#if 0
    // Huh? Put the template for the temp file name in a char array. Use vector<char>
    // to avoid using new/delete.
    string temp_file_name = JPEG2000Transmitter::temp_dir + '/' + "jp2XXXXXX";
    vector<char> temp_file(temp_file_name.size() + 1);
    string::size_type len = temp_file_name.copy(temp_file.data(), temp_file_name.size());
    temp_file[len] = '\0';

    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);

    // Make and open (an atomic operation) the temporary file. Then reset the umask
    int fd = mkstemp(temp_file.data());
    umask(original_mode);

    if (fd == -1)
        throw BESInternalError("Failed to open the temporary file: " + temp_file_name, __FILE__, __LINE__);
#endif
    // transform the OPeNDAP DataDDS to the geotiff file
    BESDEBUG("JPEG20002", "JPEG2000Transmitter::send_data - transforming into temporary file " << temp_file_name << endl);

    try {

        // Handle *functional* constraint expressions specially
        if (bdds->get_ce().function_clauses()) {
            BESDEBUG("fong2", "processing a functional constraint clause(s)." << endl);
            DDS *tmp_dds = bdds->get_ce().eval_function_clauses(*dds);
            delete dds;
            dds = tmp_dds;
            bdds->set_dds(dds);

            // This next step utilizes a well known function, promote_function_output_structures()
            // to look for one or more top level Structures whose name indicates (by way of ending
            // with "_uwrap") that their contents should be promoted (aka moved) to the top level.
            // This is in support of a hack around the current API where server side functions
            // may only return a single DAP object and not a collection of objects. The name suffix
            // "_unwrap" is used as a signal from the function to the the various response
            // builders and transmitters that the representation needs to be altered before
            // transmission, and that in fact is what happens in our friend
            // promote_function_output_structures()
            promote_function_output_structures(dds);

        }
        else {
            // Iterate through the variables in the DataDDS and read
            // in the data if the variable has the send flag set.
            for (DDS::Vars_iter i = dds->var_begin(); i != dds->var_end(); i++) {
                if ((*i)->send_p()) {
                    (*i)->intern_data(bdds->get_ce(), *dds);
                }
            }
        }

    }
    catch (Error &e) {
        throw BESDapError("Failed to read data: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
        throw;
    }
    catch (...) {
        throw BESInternalError("Failed to read data: Unknown exception caught", __FILE__, __LINE__);
    }

    try {
        FONgTransform ft(dds, bdds->get_ce(), temp_file_name);

        // Now that we are ready to start building the response data we
        // cancel any pending timeout alarm according to the configuration.
        BESUtil::conditional_timeout_cancel();

        // transform() opens the temporary file, dumps data to it and closes it.
        ft.transform_to_jpeg2000();

        BESDEBUG("JPEG20002", "JPEG2000Transmitter::send_data - transmitting temp file " << temp_file_name << endl );

        JPEG2000Transmitter::return_temp_stream(temp_file_name, strm);
    }
    catch (Error &e) {
#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif
        throw BESDapError("Failed to transform data to JPEG2000: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
    }
    catch (BESError &e) {
#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif
        throw;
    }
    catch (...) {
#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif
        throw BESInternalError("Fileout GeoTiff, was not able to transform to JPEG2000, unknown error", __FILE__, __LINE__);
    }

#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif

    BESDEBUG("JPEG20002", "JPEG2000Transmitter::send_data - done transmitting to jp2" << endl);
}

/** @brief stream the temporary file back to the requester
 *
 * Streams the temporary file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void JPEG2000Transmitter::return_temp_stream(const string &filename, ostream &strm)
{
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os)
        throw BESInternalError("Cannot connect to data source", __FILE__, __LINE__);

    char block[4096];
    os.read(block, sizeof block);
    int nbytes = os.gcount();
    if (nbytes == 0) {
        os.close();
        throw BESInternalError("Internal server error, got zero count on stream buffer.", __FILE__, __LINE__);
    }

    bool found = false;
    string context = "transmit_protocol";
    string protocol = BESContextManager::TheManager()->get_context(context, found);
    if (protocol == "HTTP") {
        strm << "HTTP/1.0 200 OK\n";
        strm << "Content-type: application/octet-stream\n";
        strm << "Content-Description: " << "BES dataset" << "\n";
        strm << "Content-Disposition: filename=" << filename << ".jp2;\n\n";
        strm << flush;
    }
    strm.write(block, nbytes);

    while (os) {
        os.read(block, sizeof block);
        nbytes = os.gcount();
        strm.write(block, nbytes);
    }

    os.close();
}

