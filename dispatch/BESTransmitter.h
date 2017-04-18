// BESTransmitter.h

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

#ifndef A_BESTransmitter_h
#define A_BESTransmitter_h 1

#include <string>

using std::string;

#include "BESObj.h"

#include "BESDataHandlerInterface.h"
#include "BESResponseObject.h"

class BESInfo;

typedef void (*p_transmitter)(BESResponseObject *obj, BESDataHandlerInterface &dhi);

class BESTransmitter: public BESObj {
private:
	map<string, p_transmitter> _method_list;
	typedef map<string, p_transmitter>::const_iterator _method_citer;
	typedef map<string, p_transmitter>::iterator _method_iter;

public:
	BESTransmitter()
	{
	}
	virtual ~BESTransmitter()
	{
	}

	virtual bool add_method(string method_name, p_transmitter trans_method);
	virtual bool remove_method(string method_name);
	virtual p_transmitter find_method(string method_name);

	// TODO I think BESResponseObject may be superfluous here since it's in the DHI,
	// but maybe not... should check. jhrg 2/20/15
	virtual void send_response(const string &method, BESResponseObject *obj, BESDataHandlerInterface &dhi);

	virtual void send_text(BESInfo &info, BESDataHandlerInterface &dhi); // = 0;
	virtual void send_html(BESInfo &info, BESDataHandlerInterface &dhi); // = 0;

	virtual void dump(ostream &strm) const;
};

#endif // A_BESTransmitter_h
