// DapRequestHandler.h

// Copyright (c) 2013 OPeNDAP, Inc. Author: James Gallagher
// <jgallagher@opendap.org>, Patrick West <pwest@opendap.org>
// Nathan Potter <npotter@opendap.org>
//                                                                            
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2.1 of
// the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301 U\ SA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI.
// 02874-0112.
#ifndef I_DapRequestHandler_H
#define I_DapRequestHandler_H

#include "BESRequestHandler.h"

class DapRequestHandler: public BESRequestHandler {
public:
	DapRequestHandler(const string &name);
	virtual ~DapRequestHandler(void) { }

	static bool dap_build_das(BESDataHandlerInterface &dhi);
	static bool dap_build_dds(BESDataHandlerInterface &dhi);
	static bool dap_build_data(BESDataHandlerInterface &dhi);
	static bool dap_build_vers(BESDataHandlerInterface &dhi);
	static bool dap_build_help(BESDataHandlerInterface &dhi);

	virtual void dump(ostream &strm) const;
};

#endif // DapRequestHandler.h
