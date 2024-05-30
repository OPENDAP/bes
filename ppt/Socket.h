// Socket.h

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

#ifndef Socket_h
#define Socket_h 1

#include <netinet/in.h>

#include <string>

#include "BESObj.h"

class Socket: public BESObj {
protected:
	int _socket;
	bool _connected;
	bool _listening;
	std::string _ip;
	unsigned int _port;
	bool _addr_set;
public:
	Socket() :
			_socket(0), _connected(false), _listening(false), _ip(""), _port(0), _addr_set(false)
	{
	}

	Socket(int socket, struct sockaddr *addr);

	virtual ~Socket()
	{
		close();
	}

	virtual void connect() = 0;
	virtual bool isConnected()
	{
		return _connected;
	}
	virtual void listen() = 0;
	virtual bool isListening()
	{
		return _listening;
	}
	virtual void close();
	virtual void send(const std::string &str, int start, int end);
	virtual int receive(char *inBuff, const int inSize);

	virtual int getSocketDescriptor()
	{
		return _socket;
	}
	unsigned int getPort()
	{
		return _port;
	}
	std::string getIp()
	{
		return _ip;
	}

	virtual unsigned int getRecvBufferSize() = 0;
	virtual unsigned int getSendBufferSize() = 0;

	virtual Socket * newSocket(int socket, struct sockaddr *addr) = 0;

	virtual bool allowConnection() = 0;

	virtual void dump(std::ostream &strm) const;
};

#endif // Socket_h
