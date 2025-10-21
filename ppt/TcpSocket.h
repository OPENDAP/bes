// TcpSocket.h

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

#ifndef TcpSocket_h
#define TcpSocket_h 1

#include <string>

#include "Socket.h"

class TcpSocket: public Socket {
private:
	std::string _host;
	int _portVal;

	void setTcpRecvBufferSize();
	void setTcpSendBufferSize();
	bool _haveRecvBufferSize;
	unsigned int _recvBufferSize;
	bool _haveSendBufferSize;
	unsigned int _sendBufferSize;
public:
	/**
	 * Build a TcPSocket object.
	 * @param host A DNS name or an IPV4 number
	 * @param portVal The port to listen on
	 */
    TcpSocket(const std::string &host, int portVal) :
            Socket(), _host(host), _portVal(portVal), _haveRecvBufferSize(false), _recvBufferSize(0), _haveSendBufferSize(
                    false), _sendBufferSize(0)
    {
    }
	TcpSocket(int portVal) :
			Socket(), _host(""), _portVal(portVal), _haveRecvBufferSize(false), _recvBufferSize(0), _haveSendBufferSize(
					false), _sendBufferSize(0)
	{
	}
	TcpSocket(int socket, struct sockaddr *addr) :
			Socket(socket, addr), _host(""), _portVal(0), _haveRecvBufferSize(false), _recvBufferSize(0), _haveSendBufferSize(
					false), _sendBufferSize(0)
	{
	}
	virtual ~TcpSocket()
	{
	}
	virtual void connect();
	virtual void listen();

	virtual unsigned int getRecvBufferSize();
	virtual unsigned int getSendBufferSize();

	virtual Socket * newSocket(int socket, struct sockaddr *addr)
	{
		return new TcpSocket(socket, addr);
	}

	virtual bool allowConnection();

	void dump(std::ostream &strm) const override;
};

#endif // TcpSocket_h
