// FONcTransform.h

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
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

#ifndef FONcTransfrom_h_
#define FONcTransfrom_h_ 1

#include <netcdf.h>

#include <string>
#include <vector>
#include <map>

using std::string ;
using std::vector ;
using std::map ;

#include <DDS.h>
#include <Array.h>

using namespace::libdap ;

#include <BESObj.h>
#include <BESDataHandlerInterface.h>

class FONcBaseType ;

/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * netcdf file
 *
 * This class transforms each variable of the DataDDS to a netcdf file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONcTransform: public BESObj {
private:
	int _ncid;
	DDS *_dds;
	string _localfile;
	string _returnAs;
	vector<FONcBaseType *> _fonc_vars;

public:
	/**
	 * Build a FONcTransform object. By default it builds a netcdf 3 file; pass "netcdf-4"
	 * to get a netcdf 4 file.
	 *
	 * @note added default value to fourth param to preserve the older API. 5/6/13 jhrg
	 * @param dds
	 * @param dhi
	 * @param localfile
	 * @param netcdfVersion
	 */
	FONcTransform(DDS *dds, BESDataHandlerInterface &dhi, const string &localfile, const string &netcdfVersion = "netcdf");
	virtual ~FONcTransform();
	virtual void transform();

	virtual void dump(ostream &strm) const;

};

#endif // FONcTransfrom_h_

