// PPTConnection.h

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

#ifndef PPTConnection_h
#define PPTConnection_h 1

#include "Connection.h"
#include "PPTProtocol.h"

class Socket;

#define PPT_CHUNK_HEADER_SPACE 15

class PPTConnection: public Connection {
private:
	int _timeout;
	char * _inBuff;
	int _inBuff_len;
#if 0
	int _bytesRead;
#endif

	PPTConnection() : _timeout(0), _inBuff(0), _inBuff_len(0) //, _bytesRead(0)
	{
	}

	virtual int readChunkHeader(char *inBuff,
	/*unsigned*/int buff_size);
	virtual void sendChunk(const std::string &buffer, std::map<std::string, std::string> &extensions);
	virtual void receive(std::ostream &strm, const /*unsigned*/int len);

protected:
	PPTConnection(int timeout) : _timeout(timeout), _inBuff(0), _inBuff_len(0) //, _bytesRead(0)
	{
	}

	virtual int readBuffer(char *inBuff, const unsigned int buff_size);
	virtual int readBufferNonBlocking(char *inBuff, const /*unsigned*/int buff_size);

	virtual void send(const std::string &buffer);
	virtual void read_extensions(std::map<std::string, std::string> &extensions, const std::string &xstr);

public:
	virtual ~PPTConnection();

	virtual void initConnection() = 0;
	virtual void closeConnection() = 0;

	virtual std::string exit()
	{
		return PPTProtocol::PPT_EXIT_NOW;
	}

	virtual void send(const std::string &buffer, std::map<std::string, std::string> &extensions);
	virtual void sendExtensions(std::map<std::string, std::string> &extensions);
	virtual void sendExit();
	virtual bool receive(std::map<std::string, std::string> &extensions, std::ostream *strm = 0);

	virtual unsigned int getRecvChunkSize();
	virtual unsigned int getSendChunkSize();

	virtual void dump(std::ostream &strm) const;
};

#endif // PPTConnection_h
