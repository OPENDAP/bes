// PPTServer.h

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

#ifndef PPTServer_h
#define PPTServer_h 1

#include "PPTConnection.h"

class ServerHandler;
class SocketListener;
class Socket;

class PPTServer: public PPTConnection {
private:
	ServerHandler * _handler;
	SocketListener * _listener;
	bool _secure {false};
	int _securePort {0};
	std::string _cfile;
	std::string _cafile;
	std::string _kfile;

	volatile int d_num_children {0};

	int welcomeClient();
	void authenticateClient();
	void get_secure_files();

public:
	PPTServer(ServerHandler *handler, SocketListener *listener, bool isSecure);
	~PPTServer() override = default;

	int get_num_children() const { return d_num_children; }
	void incr_num_children() { ++d_num_children; }
	void decr_num_children() { --d_num_children; }

	void initConnection() override;
	void closeConnection() override;

	void dump(std::ostream &strm) const override;
};

#endif // PPTServer_h
