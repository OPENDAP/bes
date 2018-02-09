// SocketListener.cc

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

#include <string>
#include <ctype.h>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <sys/socket.h>

// Added for CentOS 6 jhrg
#include <sys/wait.h>

// Added for OSX 10.9 jhrg
#include <sys/select.h>

#include "SocketListener.h"
#include "BESInternalError.h"
#include "Socket.h"
#include "SocketConfig.h"
#include "BESDebug.h"

//extern volatile int bes_num_children; // defined in PPTServer.cc jhrg 3/5/14
//extern string bes_exit_message(int cpid, int stat);

using namespace std;

SocketListener::SocketListener() :
		_accepting(false)
{
}

SocketListener::~SocketListener()
{
}

void SocketListener::listen(Socket *s)
{
	if (_accepting)
		throw BESInternalError("Already accepting connections, no more sockets can be added", __FILE__, __LINE__);

	if (s && !s->isConnected() && !s->isListening()) {
		s->listen();
		_socket_list[s->getSocketDescriptor()] = s;
	}
	else {
		if (!s)
			throw BESInternalError("null socket passed", __FILE__, __LINE__);
		else if (s->isConnected())
			throw BESInternalError("socket already connected, cannot listen", __FILE__, __LINE__);
		else if (s->isListening())
			throw BESInternalError("socket already listening", __FILE__, __LINE__);
	}
}

/** Use the select() system call to wait for an incoming connection */
Socket *
SocketListener::accept()
{
	BESDEBUG("ppt", "SocketListener::accept() - START" << endl);

	fd_set read_fd;
	FD_ZERO(&read_fd);

	int maxfd = 0;
	for (Socket_citer i = _socket_list.begin(), e = _socket_list.end(); i != e; i++) {
		Socket *s_ptr = (*i).second;
		if (s_ptr->getSocketDescriptor() > maxfd) maxfd = s_ptr->getSocketDescriptor();
		FD_SET(s_ptr->getSocketDescriptor(), &read_fd);
	}

	struct timeval timeout;
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;
	int status = select(maxfd + 1, &read_fd, (fd_set*) NULL, (fd_set*) NULL, &timeout);
	if (status < 0) {
	    // left over and not needed. jhrg 10/14/15
	    // while (select(maxfd + 1, &read_fd, (fd_set*) NULL, (fd_set*) NULL, &timeout) < 0) {
		switch (errno) {
		case EAGAIN:	// rerun select on interrupted calls, ...
			BESDEBUG("ppt2", "SocketListener::accept() - select encountered EAGAIN" << endl);
			// This case and the one below used to just 'break' so that the select call
			// above would run again. I modified it to return null so that the caller could
			// do other things, like process the results of signals.
			return 0;

		case EINTR:
			BESDEBUG("ppt2", "SocketListener::accept() - select encountered EINTR" << endl);
			return 0;

		default:
			throw BESInternalError(string("select: ") + strerror(errno), __FILE__, __LINE__);
		}
	}

	BESDEBUG("ppt", "SocketListener::accept() - select() completed without error." << endl);

	for (Socket_citer i = _socket_list.begin(), e = _socket_list.end(); i != e; i++) {
		Socket *s_ptr = (*i).second;
		if (FD_ISSET( s_ptr->getSocketDescriptor(), &read_fd )) {
			struct sockaddr from;
			socklen_t len_from = sizeof(from);

			BESDEBUG("ppt", "SocketListener::accept() - Attempting to accept on "<< s_ptr->getIp() << ":"
			    << s_ptr->getPort() << endl);

			int msgsock;
			while ((msgsock = ::accept(s_ptr->getSocketDescriptor(), &from, &len_from)) < 0) {
				if (errno == EINTR) {
					continue;
				}
				else {
					throw BESInternalError(string("accept: ") + strerror(errno), __FILE__, __LINE__);
				}
			}

			BESDEBUG("ppt", "SocketListener::accept() - END (returning new Socket)" << endl);
			return s_ptr->newSocket(msgsock, (struct sockaddr *) &from);
		}
	}

	BESDEBUG("ppt", "SocketListener::accept() - END (returning 0)" << endl);
	return 0;
}

/** @brief dumps information about this object
 *
 * Displays the pointer value of this instance
 *
 * @param strm C++ i/o stream to dump the information to
 */
void SocketListener::dump(ostream &strm) const
{
	strm << BESIndent::LMarg << "SocketListener::dump - (" << (void *) this << ")" << endl;
	BESIndent::Indent();
	if (_socket_list.size()) {
		strm << BESIndent::LMarg << "registered sockets:" << endl;
		Socket_citer i = _socket_list.begin();
		Socket_citer ie = _socket_list.end();
		for (; i != ie; i++) {
			strm << BESIndent::LMarg << "socket: " << (*i).first;
			Socket *s_ptr = (*i).second;
			s_ptr->dump(strm);
		}
	}
	else {
		strm << BESIndent::LMarg << "registered sockets: none" << endl;
	}
	BESIndent::UnIndent();
}

