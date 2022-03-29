// ServerApp.h

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

#include <string>

#include "BESModuleApp.h"

class TcpSocket;
class UnixSocket;
class PPTServer;

class ServerApp: public BESModuleApp {
private:
	int d_port {0};
	bool d_got_port {false};
	std::string d_ip_value;
	bool d_got_ip {false};
	std::string d_unix_socket_value;
	bool d_is_secure {false};
	pid_t d_pid {0};
	TcpSocket *d_tcp_socket {nullptr};
	UnixSocket *d_unix_socket {nullptr};
	PPTServer *d_ppt_server {nullptr};

public:
	ServerApp();
	~ServerApp() override;
	int initialize(int argC, char **argV) override;
	int run() override;
	int terminate(int status = 0) override;

	void dump(std::ostream &strm) const override;
};

