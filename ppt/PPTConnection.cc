// PPTConnection.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
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

#include <poll.h>

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <map>

#include "PPTConnection.h"
#include "PPTProtocolNames.h"
#include "Socket.h"
#include "BESLog.h"
#include "BESDebug.h"
#include "BESInternalError.h"

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::ostringstream;
using std::istringstream;
using std::hex;
using std::setw;
using std::setfill;
using std::map;
using std::ostream;
using std::string;


#define MODULE "ppt"
#define prolog string("PPTConnection::").append(__func__).append("() - ")

PPTConnection::~PPTConnection()
{
	if (_inBuff) {
		delete[] _inBuff;
	}
}

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
 chunk-size     = 8HEX
 last-chunk     = 7("0") d

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
void PPTConnection::send(const string &buffer, map<string, string> &extensions)
{
	if (!buffer.empty()) {
		sendChunk(buffer, extensions);

		// send the last chunk without the extensions
		map<string, string> no_extensions;
		sendChunk("", no_extensions);
	}
	else {
		sendChunk("", extensions);
	}
}

/** @brief Send the exit token as an extension
 */
void PPTConnection::sendExit()
{
	map<string, string> extensions;
	extensions["status"] = PPT_EXIT_NOW;
	send("", extensions);

	// Need to send a zero-length chunk here
	extensions.clear();
	sendChunk("", extensions);
}

/** @brief Send a chunk to the server
 *
 * A chunk is either the chunk with the data or the last-chunk
 *
 * @param buffer The data buffer to send
 * @param extensions name/value pairs to send
 * @see PPTConnection::send
 */
void PPTConnection::sendChunk(const string &buffer, map<string, string> &extensions)
{
	ostringstream strm;
	if (extensions.size()) {
		sendExtensions(extensions);
	}
	strm << hex << setw(7) << setfill('0') << buffer.size() << "d";
	if (!buffer.empty()) {
		strm << buffer;
	}
	string toSend = strm.str();
	send(toSend);
}

/** @brief send the specified extensions
 *
 * @param extensions name/value paris to be sent
 */
void PPTConnection::sendExtensions(map<string, string> &extensions)
{
	ostringstream strm;
	if (extensions.size()) {
		ostringstream estrm;
		map<string, string>::const_iterator i = extensions.begin();
		map<string, string>::const_iterator ie = extensions.end();
		for (; i != ie; i++) {
			estrm << (*i).first;
			string value = (*i).second;
			if (!value.empty()) {
				estrm << "=" << value;
			}
			estrm << ";";
		}
		string xstr = estrm.str();
		strm << hex << setw(7) << setfill('0') << xstr.size() << "x" << xstr;
		string toSend = strm.str();
		send(toSend);
	}
}

/** @brief sends the buffer to the socket
 *
 * the buffer includes the length, extensions, data, whatever is to be sent
 *
 * @param buffer data buffer to send to the socket
 */
void PPTConnection::send(const string &buffer)
{
	BESDEBUG(MODULE, prolog << "Sending " << buffer << endl);
	_mySock->send(buffer, 0, buffer.size());
}

/** @brief read a buffer of data from the socket
 *
 * @param buffer buffer to store the data received from the socket in
 * @param buffer_size max size of the data to be received
 * @return number of bytes actually read
 */
int PPTConnection::readBuffer(char *buffer, const unsigned int buffer_size)
{
	return _mySock->receive(buffer, buffer_size);
}

int PPTConnection::readChunkHeader(char *buffer, /*unsigned */int buffer_size)
{
	char *temp_buffer = buffer;
	int totalBytesRead = 0;
	bool done = false;
	while (!done) {
		int bytesRead = readBuffer(temp_buffer, buffer_size);
		BESDEBUG( MODULE, prolog << "Read " << bytesRead << " bytes" << endl );
		// change: this what < 0 but it can never be < 0 because Socket::receive()
		// will throw a BESInternalError. However, 0 indicates EOF. Note that if
		// the read(2) call in Socket::receive() returns without reading any bytes,
		// that code will call read(2) again. jhrg 3/4/14
		if (bytesRead == 0) {
			return bytesRead;
		}
		if (bytesRead < buffer_size) {
			buffer_size = buffer_size - bytesRead;
			temp_buffer = temp_buffer + bytesRead;
			totalBytesRead += bytesRead;
		}
		else {
			totalBytesRead += bytesRead;
			done = true;
		}
	}
	buffer[totalBytesRead] = '\0';
	return totalBytesRead;
}

/** @brief receive a chunk of either extensions into the specified map or data
 * into the specified stream
 *
 * This receive will read a chunk of information from the socket and
 * determine if what is read are extensions, where they are stored in the
 * extensions map passed, or data, which is written to the specified stream
 *
 * The first 7 bytes is the length of the information that was passed. The
 * 5th character is either the character 'x', signifying that extensions
 * were sent, or 'd', signifying that data was sent.
 *
 * @param extensions map to store the name/value paris into
 * @param strm output stream to write the received data into
 * @return true if what was received is the last chunk, false otherwise
 */
bool PPTConnection::receive(map<string, string> &extensions, ostream *strm)
{
	ostream *use_strm = _out;
	if (strm) use_strm = strm;

	// If the receive buffer has not yet been created, get the receive size
	// and create the buffer.
	BESDEBUG( MODULE, prolog << "buffer size = " << _inBuff_len << endl );
	if (!_inBuff) {
		_inBuff_len = _mySock->getRecvBufferSize() + 1;
		_inBuff = new char[_inBuff_len + 1];
	}

	// The first buffer will contain the length of the chunk at the beginning.
	// read the first 8 bytes. The first 7 are the length and the next 1
	// if x then extensions follow, if d then data follows.
	int bytesRead = readChunkHeader(_inBuff, 8);
	BESDEBUG( MODULE, prolog << "Reading header, read " << bytesRead << " bytes" << endl );
    if (bytesRead == 0) {
        INFO_LOG("PPTConnection::receive: read EOF from the OLFS, beslistener exiting.\n");
        // Since this is the exit condition, set extensions["status"] so that it's value is PPT_EXIT_NOW.
        // See BESServerHandler::execute(Connection *connection) and note that Connection::exit()
        // returns a string value (!). It does not do what unix exit() does! This is how the server
        // knows to exit. jhrg 5/30/24
        // Note that when this child listener exits, it will use exit code '6' and that will be picked up
        // by the master listener. That will show up in the bes.log. jhrg 5/30/24
        extensions["status"] = PPT_EXIT_NOW;
        return true;
    }
    else if (bytesRead != 8) {
        throw BESInternalError(prolog + "Failed to read chunk header", __FILE__, __LINE__);
    }

	char lenbuffer[8];
	lenbuffer[0] = _inBuff[0];
	lenbuffer[1] = _inBuff[1];
	lenbuffer[2] = _inBuff[2];
	lenbuffer[3] = _inBuff[3];
	lenbuffer[4] = _inBuff[4];
	lenbuffer[5] = _inBuff[5];
	lenbuffer[6] = _inBuff[6];
	lenbuffer[7] = '\0';
	istringstream lenstrm(lenbuffer);
	unsigned long inlen = 0;
	lenstrm >> hex >> setw(7) >> inlen;
	BESDEBUG( MODULE, prolog << "Reading header, chunk length = " << inlen << endl );
	BESDEBUG( MODULE, prolog << "Reading header, chunk type = " << _inBuff[7] << endl );

	if (_inBuff[7] == 'x') {
		ostringstream xstrm;
		receive(xstrm, inlen);
		read_extensions(extensions, xstrm.str());
	}
	else if (_inBuff[7] == 'd') {
		if (!inlen) {
			// we've received the last chunk, return true, there
			// is nothing more to read from the socket
			return true;
		}
		receive(*use_strm, inlen);
	}
	else {
		string err = (string) "type of data is " + _inBuff[7] + ", should be x for extensions or d for data";
		throw BESInternalError(err, __FILE__, __LINE__);
	}

	return false;
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
void PPTConnection::receive(ostream &strm, const /* unsigned */int len)
{
	BESDEBUG( MODULE, prolog << "len = " << len << endl );
		if( !_inBuff )
		{
			string err = "buffer has not been initialized";
			throw BESInternalError( err, __FILE__, __LINE__ );
		}

		/* unsigned */int to_read = len;
		if( len > _inBuff_len )
		{
			to_read = _inBuff_len;
		}
		BESDEBUG( MODULE, prolog << "to_read = " << to_read << endl );

		// read a buffer
		int bytesRead = readBuffer( _inBuff, to_read );
		if( bytesRead <= 0 )
		{
			string err = "Failed to read data from socket";
			throw BESInternalError( err, __FILE__, __LINE__ );
		}
		BESDEBUG( MODULE, prolog << "bytesRead = " << bytesRead << endl );

		// write the buffer read to the stream
		_inBuff[bytesRead] = '\0';
		strm.write( _inBuff, bytesRead );

		// if bytesRead is less than the chunk length, then we need to go get
		// some more. It doesn't matter what _inBuff_len is, because we need
		// len bytes to be read and we read bytesRead bytes.
		if( bytesRead < len )
		{
			BESDEBUG( MODULE, prolog << "remaining = " << (len - bytesRead) << endl );
			receive( strm, len - bytesRead );
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
void PPTConnection::read_extensions(map<string, string> &extensions, const string &xstr)
{
	// extensions are in the form var[=val]; There is always a semicolon at the end
	// if there is no equal sign then there is no value.

	string var;
	string val;
	unsigned int index = 0;
	bool done = false;
	while (!done) {
		string::size_type semi = xstr.find(';', index);
		if (semi == string::npos) {
			string err = "malformed extensions " + xstr.substr(index, xstr.size() - index) + ", missing semicolon";
			throw BESInternalError(err, __FILE__, __LINE__);
		}
		string::size_type eq = xstr.find('=', index);
		if (eq == string::npos || eq > semi) {
			// there is no value for this variable
			var = xstr.substr(index, semi - index);
			extensions[var] = "";
		}
		else if (eq == semi - 1) {
			string err = "malformed extensions " + xstr.substr(index, xstr.size() - index)
					+ ", missing value after =";
			throw BESInternalError(err, __FILE__, __LINE__);
		}
		else {
			var = xstr.substr(index, eq - index);
			val = xstr.substr(eq + 1, semi - eq - 1);
			extensions[var] = val;
		}
		index = semi + 1;
		if (index >= xstr.size()) {
			done = true;
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
 * @param buffer_size the size of the passed buffer
 * @return number of bytes read in, -1 if failed to read anything
 */
int PPTConnection::readBufferNonBlocking(char *inBuff, const /* unsigned*/int buffer_size)
{
    struct pollfd arr[1];
    arr[0].fd = getSocket()->getSocketDescriptor();
    arr[0].events = POLLIN;
    arr[0].revents = 0;

	// Let's loop _timeout times with a delay block on poll of 1000 milliseconds
	// and see if there are any data.
	for (int j = 0; j < _timeout; j++) {
		if (poll(arr, 1, 1000) < 0) {
			// Allow this call to be interrupted without it being an error. jhrg 6/15/11
			if (errno == EINTR || errno == EAGAIN) continue;

			throw BESInternalError(string("poll error") + " " + strerror(errno), __FILE__, __LINE__);
		}
		else {
			if (arr[0].revents == POLLIN) {
				return readBuffer(inBuff, buffer_size);
			}
			else {
				cout << " " << j << flush;
			}
		}
	}
	cout << endl;
	return -1;
}

unsigned int PPTConnection::getRecvChunkSize()
{
	return _mySock->getRecvBufferSize() - PPT_CHUNK_HEADER_SPACE;
}

unsigned int PPTConnection::getSendChunkSize()
{
	return _mySock->getSendBufferSize() - PPT_CHUNK_HEADER_SPACE;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void PPTConnection::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "PPTConnection::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	Connection::dump(strm);
	BESIndent::UnIndent();
}

