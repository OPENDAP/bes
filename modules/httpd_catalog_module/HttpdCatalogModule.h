
// -*- mode: c++; c-basic-offset:4 -*-
//
// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.
//
// Copyright (c) 2018 OPeNDAP, Inc.
// Author: Nathan Potter <ndp@opendap.org>
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

#ifndef I_HttpdCatalogModule_H
#define I_HttpdCatalogModule_H 1


#include "BESAbstractModule.h"

namespace httpd_catalog {

class HttpdCatalogModule: public BESAbstractModule {
public:
    HttpdCatalogModule()
	{
	}

	virtual ~HttpdCatalogModule()
	{
	}

	virtual void initialize(const std::string &modname);
	virtual void terminate(const std::string &modname);

	void dump(std::ostream &strm) const override;
};

} // namespace httpd_catalog

#endif // I_HttpdCatalogModule_H
