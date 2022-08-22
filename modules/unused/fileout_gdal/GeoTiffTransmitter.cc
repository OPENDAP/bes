// GeoTiffTransmitter.cc

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

#define MODULE "fong"
#define prolog string("FONgTransmitter::").append(__func__).append("() - ")

#include "GeoTiffTransmitter.h"
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

#include "RequestServiceTimer.h"

#define FONG_TEMP_DIR "/tmp"
#define FONG_GCS "WGS84"

string GeoTiffTransmitter::temp_dir;
string GeoTiffTransmitter::default_gcs;

/** @brief Construct the GeoTiffTransmitter, adding it with name geotiff to be
 * able to transmit a data response
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as a geotiff file.
 *
 * The OPeNDAP data object is written to a geotiff file locally in a
 * temporary directory specified by the BES configuration parameter
 * FONg.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FONG_TEMP_DIR.
 *
 * @note The mapping from a 'returnAs' of "geotiff" to this code
 * is made in the FONgModule class.
 *
 * @see FONgModule
 */
GeoTiffTransmitter::GeoTiffTransmitter() :  BESTransmitter()
{
    // DATA_SERVICE == "dods"
    add_method(DATA_SERVICE, GeoTiffTransmitter::send_data_as_geotiff);

    if (GeoTiffTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FONg.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, GeoTiffTransmitter::temp_dir, found);
        if (!found || GeoTiffTransmitter::temp_dir.empty()) {
            GeoTiffTransmitter::temp_dir = FONG_TEMP_DIR;
        }
        string::size_type len = GeoTiffTransmitter::temp_dir.size();
        if (GeoTiffTransmitter::temp_dir[len - 1] == '/') {
            GeoTiffTransmitter::temp_dir = GeoTiffTransmitter::temp_dir.substr(0, len - 1);
        }
    }

    if (GeoTiffTransmitter::default_gcs.empty()) {
        // Use what as the default Geographic coordinate system?
        bool found = false;
        string key = "FONg.Default_GCS";
        TheBESKeys::TheKeys()->get_value(key, GeoTiffTransmitter::default_gcs, found);
        if (!found || GeoTiffTransmitter::default_gcs.empty()) {
            GeoTiffTransmitter::default_gcs = FONG_GCS;
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
void GeoTiffTransmitter::send_data_as_geotiff(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds)
        throw BESInternalError("cast error", __FILE__, __LINE__);

    DDS *dds = bdds->get_dds();
    if (!dds)
        throw BESInternalError("No DataDDS has been created for transmit", __FILE__, __LINE__);

    ostream &strm = dhi.get_output_stream();
    if (!strm)
        throw BESInternalError("Output stream is not set, cannot return as", __FILE__, __LINE__);

    BESDEBUG("fong2", "GeoTiffTransmitter::send_data - parsing the constraint" << endl);

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
    BESDEBUG("fong2", "GeoTiffTransmitter::send_data - reading data into DataDDS" << endl);

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

    // This closes the file when it goes out of scope. jhrg 8/25/17
    bes::TempFile temp_file;
    string temp_file_name = temp_file.create(GeoTiffTransmitter::temp_dir, "geotiff_");
#if 0
    // Huh? Put the template for the temp file name in a char array. Use vector<char>
    // to avoid using new/delete.
    string temp_file_name = GeoTiffTransmitter::temp_dir + '/' + "geotiffXXXXXX";
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
    BESDEBUG("fong2", "GeoTiffTransmitter::send_data - transforming into temporary file " << temp_file_name << endl);

    try {
        FONgTransform ft(dds, bdds->get_ce(), temp_file_name);

        // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
        RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
        BESUtil::conditional_timeout_cancel();

        // transform() opens the temporary file, dumps data to it and closes it.
        ft.transform_to_geotiff();

        BESDEBUG("fong2", "GeoTiffTransmitter::send_data - transmitting temp file " << temp_file_name << endl );

        GeoTiffTransmitter::return_temp_stream(temp_file_name, strm);
    }
    catch (Error &e) {
#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif
        throw BESDapError("Failed to transform data to GeoTiff: " + e.get_error_message(), false, e.get_error_code(), __FILE__, __LINE__);
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
        throw BESInternalError("Fileout GeoTiff, was not able to transform to geotiff, unknown error", __FILE__, __LINE__);
    }

#if 0
        close(fd);
        (void) unlink(temp_file.data());
#endif

    BESDEBUG("fong2", "GeoTiffTransmitter::send_data - done transmitting to geotiff" << endl);
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
void GeoTiffTransmitter::return_temp_stream(const string &filename, ostream &strm)
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

    // I think this is never used - we never run Hyrax where the BES is accessed
    // directly by HTTP.
    bool found = false;
    string protocol = BESContextManager::TheManager()->get_context("transmit_protocol", found);
    if (protocol == "HTTP") {
        strm << "HTTP/1.0 200 OK\n";
        strm << "Content-type: application/octet-stream\n";
        strm << "Content-Description: " << "BES dataset" << "\n";
        strm << "Content-Disposition: filename=" << filename << ".tif;\n\n";
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

