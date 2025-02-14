// BESStreamResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundatiion; either
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

#include <unistd.h>

#include <cerrno>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

using std::ifstream;
using std::ios;
using std::endl;
using std::string;
using std::ostream;

#include "BESStreamResponseHandler.h"
#include "BESForbiddenError.h"
#include "BESNotFoundError.h"
#include "BESInternalError.h"
#include "BESContainer.h"
#include "BESDataHandlerInterface.h"
#include "BESUtil.h"
#include "RequestServiceTimer.h"

#define BES_STREAM_BUFFER_SIZE 4096

#define MODULE "bes"
#define prolog std::string("BESStreamResponseHandler::").append(__func__).append("() - ")

/** @brief executes the command 'get file <filename>;' by
 * streaming the specified file
 *
 * @param dhi structure that holds request and response information
 * @throws BESNotFoundError if the specified file to stream does not exist
 * @throws BESInternalError if not all required information is provided
 * @see BESDataHandlerInterface
 * @see BESHTMLInfo
 * @see BESRequestHandlerList
 */
void BESStreamResponseHandler::execute(BESDataHandlerInterface &dhi)
{
    d_response_object = nullptr;

    // Verify the request hasn't exceeded bes_timeout, and disable timeout if allowed.
    RequestServiceTimer::TheTimer()->throw_if_timeout_expired(prolog + "ERROR: bes-timeout expired before transmit", __FILE__, __LINE__);
    BESUtil::conditional_timeout_cancel();

    // What if there is a special way to stream back a data file?
    // Should we pass this off to the request handlers and put
    // this code into a different class for reuse? For now
    // just keep it here. pcw 10/11/06

    // I thought about putting this in the transmit method below
    // but decided that this is like executing a non-buffered
    // request, so kept it here. Plus the idea expressed above
    // led me to leave the code in the execute method.
    // pcw 10/11/06
    if (dhi.containers.size() != 1) {
        string err = (string) "Unable to stream file: " + "no container specified";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    dhi.first_container();
    BESContainer *container = dhi.container;
    string filename = container->access();
    if (filename.empty()) {
        string err = (string) "Unable to stream file: " + "filename not specified";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    ifstream os;
    os.open(filename.c_str(), ios::in);
    int myerrno = errno;
    if (!os) {
        string serr = (string) "Unable to stream file because it cannot be opened. file: '" + filename + "'  msg: ";
        char *err = strerror(myerrno);
        if (err)
            serr += err;
        else
            serr += "Unknown error";

        // ENOENT means that the node wasn't found.
        // On some systems a file that doesn't exist returns ENOTDIR because: w.f.t?
        // Otherwise, access is being denied for some other reason
        if (myerrno == ENOENT || myerrno == ENOTDIR) {
            // On some systems a file that doesn't exist returns ENOTDIR because: w.f.t?
            throw BESNotFoundError(serr, __FILE__, __LINE__);
        }
        // Not a 404? Then we'll go with the forbidden fruit theory...
        throw BESForbiddenError(serr, __FILE__, __LINE__);
    }

    std::streamsize nbytes;
    char block[BES_STREAM_BUFFER_SIZE];
    os.read(block, sizeof block);   // read() returns the istream&
    nbytes = os.gcount();   // gcount() returns the number of chars read above
    while (nbytes) {
        dhi.get_output_stream().write((char*) block, nbytes);
        os.read(block, sizeof block);
        nbytes = os.gcount();
    }

    os.close();
}

/** @brief transmit the file, streaming it back to the client
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESTransmitter
 * @see BESDataHandlerInterface
 */
void BESStreamResponseHandler::transmit(BESTransmitter */*transmitter*/, BESDataHandlerInterface &/*dhi*/)
{
    // The Data is transmitted when it is read, dumped to stdout, so there is nothing
    // to transmit here.
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void BESStreamResponseHandler::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "BESStreamResponseHandler::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    BESResponseHandler::dump(strm);
    BESIndent::UnIndent();
}

BESResponseHandler *
BESStreamResponseHandler::BESStreamResponseBuilder(const string &name)
{
    return new BESStreamResponseHandler(name);
}

