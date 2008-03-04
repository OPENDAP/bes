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

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>

using std::cout ;
using std::endl ;
using std::flush ;
using std::ostringstream ;
using std::istringstream ;
using std::hex ;
using std::setw ;
using std::setfill ;

#include "PPTConnection.h"
#include "PPTProtocol.h"
#include "Socket.h"
#include "BESInternalError.h"

/** @brief Send a message to the server
 *
 * Sends the specified message buffer to the server
 * followed by a buffer of length 0 to signify the
 * end of the message
 *
 * A buffer sent to the server will follow the following form:
 * @code
       Chunked-Body   = chunk-extensions
                        chunk
                        last-chunk

       chunk-extensions= chunk-size 'x' *( chunk-ext-name [ "=" chunk-ext-val ] ;
       chunk          = chunk-size 'd' chunk-data
       chunk-size     = 4HEX
       last-chunk     = 4("0") d

       chunk-ext-name = token
       chunk-ext-val  = token | quoted-string
       chunk-data     = chunk-size(OCTET)
 * @endcode
 *
 * If there are extensions then they are sent first. The length of
 * the extensions is sent first, followed by the character 'x', and
 * then the extensions in the format name[=value];
 *
 * Then the buffer itself is sent. The length of the buffer is sent
 * followed by the character 'd' signifying data is being transmitted.
 *
 * if the buffer is empty then this represents the last chunk
 *
 * @param buffer buffer of data to send
 * @param extensions list of name/value pairs sent
 */
void
PPTConnection::send( const string &buffer,
                     map<string,string> &extensions )
{
    if( !buffer.empty() )
    {
	sendChunk( buffer, extensions ) ;

	// send the last chunk without the extensions
	map<string,string> no_extensions ;
	sendChunk( "", no_extensions ) ;
    }
    else
    {
	sendChunk( "", extensions ) ;
    }
}

/** @brief Send the exit token as an extension
 */
void
PPTConnection::sendExit()
{
    map<string,string> extensions ;
    extensions["status"] = PPTProtocol::PPT_EXIT_NOW ;
    send( "", extensions ) ;
}

/** @brief Send a chunk to the server
 *
 * A chunk is either the chunk with the data or the last-chunk
 *
 * @param buffer The data buffer to send
 * @param extensions name/value pairs to send
 * @see PPTConnection::send
 */
void
PPTConnection::sendChunk( const string &buffer, map<string,string> &extensions )
{
    ostringstream strm ;
    if( extensions.size() )
    {
	sendExtensions( extensions ) ;
    }
    strm << hex << setw( 4 ) << setfill( '0' ) << buffer.length() << "d" ;
    if( !buffer.empty() )
    {
	strm << buffer ;
    }
    string toSend = strm.str() ;
    send( toSend ) ;
}

/** @brief send the specified extensions
 *
 * @param extensions name/value paris to be sent
 */
void
PPTConnection::sendExtensions( map<string,string> &extensions )
{
    ostringstream strm ;
    if( extensions.size() )
    {
	ostringstream estrm ;
	map<string,string>::const_iterator i = extensions.begin() ;
	map<string,string>::const_iterator ie = extensions.end() ;
	for( ; i != ie; i++ )
	{
	    estrm << (*i).first ;
	    string value = (*i).second ;
	    if( !value.empty() )
	    {
		estrm << "=" << value ;
	    }
	    estrm << ";" ;
	}
	string xstr = estrm.str() ;
	strm << hex << setw( 4 ) << setfill( '0' ) << xstr.length() << "x" << xstr ;
	string toSend = strm.str() ;
	send( toSend ) ;
    }
}

/** @brief sends the buffer to the socket
 *
 * the buffer includes the length, extensions, data, whatever is to be sent
 *
 * @param buffer data buffer to send to the socket
 */
void
PPTConnection::send( const string &buffer )
{
    _mySock->send( buffer, 0, buffer.length() ) ;
    _mySock->sync() ;
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

/* WORKING TOKEN RECEIVE
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

    char *inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE+1] ;
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
		    // check for the mark. If we've found the entire marker
		    // then markCheck returns true. If we haven't yet found
		    // the entire mark but have found part of it then
		    // markCheck will return false and the markIndex will be
		    // greater than 0. If the check failed because the
		    // character checked is not part of the marker then the
		    // markIndex is set to 0 and false is returned.
		    done = mf.markCheck( inBuff[i] ) ;
		    if( !done )
		    {
			// didn't find the mark yet, check if we've found
			// part of it
			if( mf.getMarkIndex() > 0 )
			{
			    // we found part of it, so cache what we found
			    // in markBuffer
			    markBuffer[markBufBytes++] = inBuff[i] ;
			}
			else
			{
			    // we are here because inBuff[i] does not match
			    // the current position in the marker.

			    // if we found part of the mark (there's some
			    // characters in markBuffer meaning markBufBytes
			    // is greater than 0) then we need to send the
			    // first character in the markBuffer and begiin
			    // checking the rest of the characters in the
			    // markBuffer against the markFinder. inBuff[i]
			    // could still be part of the marker, so don't
			    // send it to the stream just yet. The case here
			    // is that the inBuff contains PPPT instead of
			    // PPT as the beginning of the marker. PP is in
			    // the markBuffer and we've just checked the
			    // third 'P', which returns false and markIndex
			    // is set to 0. So, send the first 'P' in
			    // markBuffer, shift the remaining characters
			    // and check them against the markFinder.
			    if( markBufBytes > 0 )
			    {
				// let's put inBuf[i] in the markBuffer so
				// that we don't have to worry about it
				// anymore
				markBuffer[markBufBytes++] = inBuff[i] ;

				bool isdone = false ;
				while( !isdone )
				{
				    // always throw the first character iin
				    // markBuffer to the stream
				    (*use_strm) << markBuffer[0] ;

				    // shift the rest of the characters in
				    // markBuffer to the left one
				    for( int j = 1; j < markBufBytes; j++ )
				    {
					markBuffer[j-1] = markBuffer[j] ;
				    }

				    // we've sent the first character to the
				    // stream, so reduce the number in
				    // markBuffer
				    markBufBytes-- ;

				    // start checking the rest of the
				    // characters in markBuffer
				    bool partof = true ;
				    for( int j = 0; j < markBufBytes && partof; j++ )
				    {
					// don't need to look at the result
					// of markCheck because we already
					// know that we have not
					// found the complete marker, we're
					// just dealing with potentially
					// part of the marker
					mf.markCheck( markBuffer[j] ) ;
					if( mf.getMarkIndex() == 0 )
					{
					    // if the markIndex is 0 then
					    // the character we just checked
					    // is not part of the marker, so
					    // repeat this process starting
					    // at for( !isdone )
					    partof = false ;
					}
				    }

				    if( partof == true )
				    {
					// if we've made it this far then
					// what's in the markBuffer is part
					// of the marker, so move on to the
					// next character in inBuff
					isdone = true ;
				    }
				}
			    }
			    else
			    {
				// There's nothing in the markBuffer so just
				// send inBuff on to the stream
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
*/

/** @brief read a buffer of data from the socket
 *
 * @param buffer buffer to store the data received from the socket in
 * @param buffer_size max size of the data to be received
 * @return number of bytes actually read
 */
int
PPTConnection::readBuffer( char *buffer, unsigned int buffer_size )
{
    return _mySock->receive( buffer, buffer_size ) ;
}

/** @brief receive a chunk of either extensions into the specified map or data
 * into the specified stream
 *
 * This receive will read a chunk of information from the socket and determine if what is
 * read are extensions, where they are stored in the extensions map passed, or data, 
 * which is written to the specified stream
 *
 * The first 4 bytes is the length of the information that was passed. The 5th character is
 * either the character 'x', signifying that extensions were sent, or 'd', signifying that
 * data was sent.
 *
 * @param extensions map to store the name/value paris into
 * @param strm output stream to write the received data into
 * @return true if what was received is the last chunk, false otherwise
 */
bool
PPTConnection::receive( map<string,string> &extensions,
			ostream *strm )
{
    ostream *use_strm = _out ;
    if( strm )
	use_strm = strm ;

    // The first buffer will contain the length of the chunk at the beginning.
    if( !_inBuff )
	_inBuff = new char[PPT_PROTOCOL_BUFFER_SIZE+1] ;

    // read the first 5 bytes. The first 4 are the length and the next 1
    // if x then extensions follow, if d then data follows.
    int bytesRead = readBuffer( _inBuff, 5 ) ;
    if( bytesRead != 5 )
    {
	string err = "Failed to read length and type of chunk" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    char lenbuffer[5] ;
    lenbuffer[0] = _inBuff[0] ;
    lenbuffer[1] = _inBuff[1] ;
    lenbuffer[2] = _inBuff[2] ;
    lenbuffer[3] = _inBuff[3] ;
    lenbuffer[4] = '\0' ;
    istringstream lenstrm( lenbuffer ) ;
    unsigned short inlen = 0 ;
    lenstrm >> hex >> setw(4) >> inlen ;

    if( _inBuff[4] == 'x' )
    {
	ostringstream xstrm ;
	receive( xstrm, inlen ) ;
	read_extensions( extensions, xstrm.str() ) ;
    }
    else if( _inBuff[4] == 'd' )
    {
	if( !inlen )
	{
	    // we've received the last chunk, return true, there
	    // is nothing more to read from the socket
	    return true ;
	}
	receive( *use_strm, inlen ) ;
    }
    else
    {
	string err = (string)"type of data is " + _inBuff[4]
	             + ", should be x for extensions or d for data" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }

    return false ;
}

/** @brief receive from the socket the number of bytes specified
 * until done
 *
 * This method receives data from the socket until there is nothing
 * more to be read
 *
 * @param strm output stream to write what is received to
 * @param len number of bytes remaining to be read
 */
void
PPTConnection::receive( ostream &strm, unsigned short len )
{
    if( !_inBuff )
    {
	string err = "buffer has not been initialized" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    // I added this test because in PPTConnection::receive( map<string,string>,
	// ostream ) this method is called with 'len' passed a value that's read from
	// the input stream. That value could be manipulated to cause a bufer
	// overflow. Note that _inBuff is PPT_PROTOCOL_BUFFER_SIZE + 1 so reading
	// that many bytes leaves room for the null byte. jhrg 3/3/08
    if( len > PPT_PROTOCOL_BUFFER_SIZE )
    {
	string err = "buffer is not large enough" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    int bytesRead = readBuffer( _inBuff, len ) ;
    if( bytesRead <= 0 )
    {
	string err = "Failed to read data from socket" ;
	throw BESInternalError( err, __FILE__, __LINE__ ) ;
    }
    _inBuff[bytesRead] = '\0' ;
    strm.write( _inBuff, bytesRead ) ;
    if( bytesRead < len )
    {
	receive( strm, len - bytesRead ) ;
    }
}

/** @brief the string passed are extensions, read them and store the name/value pairs into
 * the passed map
 *
 * It has already been determined that extensions were read in the chunk. Deconstruct
 * the name/value pairs and store them into the map passed. Each extension ends with
 * a semicolon.
 *
 * @param extensions map to store the name/value pairs in
 * @param xstr string of extensions in the form *(name[=value];)
 */
void
PPTConnection::read_extensions( map<string,string> &extensions, const string &xstr )
{
    // extensions are in the form var[=val]; There is always a semicolon at the end
    // if there is no equal sign then there is no value.

    string var ;
    string val ;
    int index = 0 ;
    bool done = false ;
    while( !done )
    {
	string::size_type semi = xstr.find( ';', index ) ;
	if( semi == string::npos )
	{
	    string err = "malformed extensions "
	                 + xstr.substr( index, xstr.length() - index )
			 + ", missing semicolon" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	string::size_type eq = xstr.find( '=', index ) ;
	if( eq == string::npos || eq > semi )
	{
	    // there is no value for this variable
	    var = xstr.substr( index, semi-index ) ;
	    extensions[var] = "" ;
	}
	else if( eq == semi-1 )
	{
	    string err = "malformed extensions "
	                 + xstr.substr( index, xstr.length() - index )
			 + ", missing value after =" ;
	    throw BESInternalError( err, __FILE__, __LINE__ ) ;
	}
	else
	{
	    var = xstr.substr( index, eq-index ) ;
	    val = xstr.substr( eq+1, semi-eq-1 ) ;
	    extensions[var] = val ;
	}
	index = semi+1 ;
	if( index >= xstr.length() )
	{
	    done = true ;
	}
    }
}

/** @brief read a buffer of data from the socket without blocking
 *
 * Try to read a buffer of data without blocking. We will try
 * _timeout times, waiting 1000 milliseconds between each try. The
 * variable _timeout is passed into the constructor.
 *
 * @param inBuff buffer to store the data into
 * @return number of bytes read in, -1 if failed to read anything
 */
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
	    throw BESInternalError( error, __FILE__, __LINE__ ) ;
	}
	else
	{
	    if (arr[0].revents==POLLIN)
	    {
		return readBuffer( inBuff, PPT_PROTOCOL_BUFFER_SIZE ) ;
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

