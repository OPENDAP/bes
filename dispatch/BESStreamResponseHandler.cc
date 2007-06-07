// BESStreamResponseHandler.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

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
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdio.h>

using std::ifstream ;
using std::ios ;
using std::cerr ;
using std::endl ;
using std::string ;

#include "BESStreamResponseHandler.h"
#include "BESRequestHandlerList.h"
#include "BESHandlerException.h"
#include "BESDataNames.h"
#include "BESContainer.h"

#define BES_STREAM_BUFFER_SIZE 4096

BESStreamResponseHandler::BESStreamResponseHandler( const string &name )
    : BESResponseHandler( name )
{
}

BESStreamResponseHandler::~BESStreamResponseHandler( )
{
}

/** @brief executes the command 'get file &lt;filename&gt;;' by
 * streaming the specified file
 *
 * @param dhi structure that holds request and response information
 * @throws BESHandlerException if there is a problem building the
 * response object
 * @throws BESResponseException upon fatal error building the response
 * object
 * @see _BESDataHandlerInterface
 * @see BESHTMLInfo
 * @see BESRequestHandlerList
 */
void
BESStreamResponseHandler::execute( BESDataHandlerInterface &dhi )
{
    _response = 0 ;

    // What if there is a special way to stream back a data file?
    // Should we pass this off to the request handlers and put
    // this code into a different class for reuse? For now
    // just keep it here. pcw 10/11/06

    // I thought about putting this in the transmit method below
    // but decided that this is like executing a non-buffered
    // request, so kept it here. Plus the idea expressed above
    // led me to leave the code in the execute method.
    // pcw 10/11/06
    if( dhi.containers.size() != 1 )
    {
	string err = (string)"Unable to stream file: "
	             + "no container specified" ;
	throw BESHandlerException( err, __FILE__, __LINE__ ) ;
    }

    dhi.first_container() ;
    BESContainer *container = dhi.container ;
    string filename = container->access() ;
    if( filename.empty() )
    {
	string err = (string)"Unable to stream file: "
	             + "filename not specified" ;
	throw BESHandlerException( err, __FILE__, __LINE__ ) ;
    }

    int bytes = 0 ;
    ifstream os ;
    os.open( filename.c_str(), ios::in ) ;
    if( !os )
    {
	string err = (string)"Unable to stream file: "
	             + "can not open file "
		     + filename ;
	throw BESHandlerException( err, __FILE__, __LINE__ ) ;
    }

    int nbytes ;
    int fd = fileno( stdout ) ;
    char block[BES_STREAM_BUFFER_SIZE] ;
    os.read( block, sizeof block ) ;
    nbytes = os.gcount() ;
    while( nbytes )
    {
	bytes += nbytes ;
	int written = write( fd, (void*)block, nbytes ) ;
	os.read( block, sizeof block ) ;
	nbytes = os.gcount() ;
    }
    os.close() ;
}

/** @brief transmit the file, streaming it back to the client
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see BESHTMLInfo
 * @see BESTransmitter
 * @see _BESDataHandlerInterface
 */
void
BESStreamResponseHandler::transmit( BESTransmitter *transmitter,
                                 BESDataHandlerInterface & )
{
    // The Data is transmitted when it is read, dumped to stdout
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
BESStreamResponseHandler::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESStreamResponseHandler::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    BESResponseHandler::dump( strm ) ;
    BESIndent::UnIndent() ;
}

BESResponseHandler *
BESStreamResponseHandler::BESStreamResponseBuilder( const string &name )
{
    return new BESStreamResponseHandler( name ) ;
}

