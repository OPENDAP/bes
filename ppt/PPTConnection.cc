// PPTConnection.cc

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

#include <poll.h>
#include <errno.h>
#include <iostream>

using std::cout ;
using std::flush ;

#include "PPTConnection.h"
#include "PPTProtocol.h"
#include "Socket.h"
#include "PPTException.h"
#include "PPTMarkFinder.h"

void
PPTConnection::send( const string &buffer )
{
    if( buffer != "" )
    {
	writeBuffer( buffer ) ;
    }
    writeBuffer( PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION ) ;
}

void
PPTConnection::sendExit()
{
    writeBuffer( PPTProtocol::PPT_EXIT_NOW ) ;
}

void
PPTConnection::writeBuffer( const string &buffer )
{
    _mySock->send( buffer, 0, buffer.length() ) ;
}
    
/* OLD RECEIVE
bool
PPTConnection::receive( ostream *strm )
{
    bool isDone = false ;
    ostream *use_strm = _out ;
    if( strm )
	use_strm = strm ;

    int termlen = PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION.length() ;
    int exitlen = PPTProtocol::PPT_EXIT_NOW.length() ;
    int start = termlen ;
    bool done = false ;
    char *inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE+termlen+1] ;
    while( done == false )
    {
	int bytesRead = readBuffer( inBuff+termlen ) ;
	if( bytesRead != 0 )
	{
	    if( !strncmp( inBuff+start, PPTProtocol::PPT_EXIT_NOW.c_str(), exitlen ) )
	    {
		done = true ;
		isDone = true ;
	    }
	    else
	    {
		int charsInBuff = bytesRead + ( termlen - start ) ;
		int writeBytes = charsInBuff - termlen ;
		if( charsInBuff >= termlen )
		{
		    if( !strncmp( inBuff+bytesRead, PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION.c_str(), termlen ) )
		    {
			done = true ;
		    }

		    for( int j = 0; j < writeBytes; j++ )
		    {
			if( use_strm )
			{
			    (*use_strm) << inBuff[start+j] ;
			}
		    }

		    if( !done )
		    {
			memcpy( inBuff, inBuff + bytesRead, termlen ) ;
			start = 0 ;
		    }
		}
		else
		{
		    int newstart = termlen - charsInBuff ;
		    memcpy( inBuff + newstart, inBuff + start, charsInBuff ) ;
		    start = newstart ;
		}
	    }
	}
	else
	{
	    done = true ;
	}
    }
    delete [] inBuff ;
    return isDone ;
}
*/

bool
PPTConnection::receive( ostream *strm )
{
    bool isDone = false ;
    ostream *use_strm = _out ;
    if( strm )
	use_strm = strm ;

    int bytesRead, markBufBytes, i ;

    int termlen = PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION.length() ;
    int exitlen = PPTProtocol::PPT_EXIT_NOW.length() ;

    PPTMarkFinder mf( (unsigned char *)PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION.c_str(),
                      termlen ) ;
    unsigned char markBuffer[termlen] ;
    markBufBytes = 0 ; // zero byte count in the mark buffer

    char *inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE+termlen+1] ;
    bool done = false;
    while( !done )
    {
	bytesRead = readBuffer( inBuff ) ;
	if( bytesRead != 0 )
	{
	    // did we find an exit string?
	    if( !strncmp( inBuff, PPTProtocol::PPT_EXIT_NOW.c_str(), exitlen ) )
	    {
		done = true ;
		isDone = true ;
	    }
	    else
	    {
		// look at what we got to find the exit or the term string
		for( i = 0; i < bytesRead && !done; i++ )
		{
		    // check for the mark. If we've found the entire marker then markCheck returns true. If we haven't yet found
		    // the entire mark but have found part of it then markCheck will return false and the markIndex will be
		    // greater than 0. If the check failed because the character checked is not part of the marker then the
		    // markIndex is set to 0 and false is returned.
		    done = mf.markCheck( inBuff[i] ) ;
		    if( !done )
		    {
			// didn't find the mark yet, check if we've found part of it
			if( mf.getMarkIndex() > 0 )
			{
			    // we found part of it, so cache what we found in markBuffer
			    markBuffer[markBufBytes++] = inBuff[i] ;
			}
			else
			{
			    // we are here because inBuff[i] does not match the current position in the marker.

			    // if we found part of the mark (there's some characters in markBuffer meaning markBufBytes is
			    // greater than 0) then we need to send the first character in the markBuffer and begiin
			    // checking the rest of the characters in the markBuffer against the markFinder. inBuff[i] could
			    // still be part of the marker, so don't send it to the stream just yet. The case here is
			    // that the inBuff contains PPPT instead of PPT as the beginning of the marker. PP is in the
			    // markBuffer and we've just checked the third 'P', which returns false and markIndex is set to 0.
			    // So, send the first 'P' in markBuffer, shift the remaining characters and check them against the
			    // markFinder.
			    if( markBufBytes > 0 )
			    {
				// let's put inBuf[i] in the markBuffer so that we don't have to worry about it anymore
				markBuffer[markBufBytes++] = inBuff[i] ;

				bool isdone = false ;
				while( !isdone )
				{
				    // always throw the first character iin markBuffer to the stream
				    (*use_strm) << markBuffer[0] ;

				    // shift the rest of the characters in markBuffer to the left one
				    for( int j = 1; j < markBufBytes; j++ )
				    {
					markBuffer[j-1] = markBuffer[j] ;
				    }

				    // we've sent the first character to the stream, so reduce the number in markBuffer
				    markBufBytes-- ;

				    // start checking the rest of the characters in markBuffer
				    bool partof = true ;
				    for( int j = 0; j < markBufBytes && partof; j++ )
				    {
					// don't need to look at the result of markCheck because we already know that we have not
					// found the complete marker, we're just dealing with potentially part of the marker
					mf.markCheck( markBuffer[j] ) ;
					if( mf.getMarkIndex() == 0 )
					{
					    // if the markIndex is 0 then the character we just checked is not part of the
					    // marker, so repeat this process starting at for( !isdone )
					    partof = false ;
					}
				    }

				    if( partof == true )
				    {
					// if we've made it this far then what's in the markBuffer is part of the marker, so
					// move on to the next character in inBuff
					isdone = true ;
				    }
				}
			    }
			    else
			    {
				// There's nothing in the markBuffer so just send inBuff on to the stream
				(*use_strm) << inBuff[i] ;
			    }
			}
		    }
		}
	    }
	}
	else
	{
	    done = true;
	}
    }
    delete [] inBuff ;
    return isDone ;
}

int
PPTConnection::readBuffer( char *inBuff )
{
    return _mySock->receive( inBuff, PPT_PROTOCOL_BUFFER_SIZE ) ;
}

int
PPTConnection::readBufferNonBlocking( char *inBuff )
{
    struct pollfd p ;
    p.fd = getSocket()->getSocketDescriptor();
    p.events = POLLIN ;
    struct pollfd arr[1] ;
    arr[0] = p ;

    // Lets loop _timeout times with a delay block on poll of 1000 milliseconds
    // and see if there is any data.
    for( int j = 0; j < _timeout; j++ )
    {
	if( poll( arr, 1, 1000 ) < 0 )
	{
	    string error( "poll error" ) ;
	    const char* error_info = strerror( errno ) ;
	    if( error_info )
		error += " " + (string)error_info ;
	    throw PPTException( error ) ;
	}
	else
	{
	    if (arr[0].revents==POLLIN)
	    {
		return readBuffer( inBuff ) ;
	    }
	    else
	    {
		cout << " " << j << flush ;
	    }
	}
    }
    cout << endl ;
    return -1 ;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
PPTConnection::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "PPTConnection::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    Connection::dump( strm ) ;
    BESIndent::UnIndent() ;
}

