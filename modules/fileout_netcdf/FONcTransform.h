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
#include <set>

#include <DDS.h>
#include <DMR.h>
#include <Array.h>

using namespace::libdap ;

#include <BESObj.h>
#include <BESDataHandlerInterface.h>

class FONcBaseType ;
class BESResponseObject;

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
    DMR *_dmr;
    BESResponseObject *d_obj;
    BESDataHandlerInterface *d_dhi;
	string _localfile;
	string _returnAs;
	vector<FONcBaseType *> _fonc_vars;
	vector<FONcBaseType *> _total_fonc_vars_in_grp;
    set<string> _included_grp_names;
    map<string,unsigned long> GFQN_dimname_to_dimsize;
    map<string,unsigned long> VFQN_dimname_to_dimsize;
    

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
	FONcTransform(DMR *dmr, BESDataHandlerInterface &dhi, const string &localfile, const string &netcdfVersion = "netcdf");
    FONcTransform(BESResponseObject *obj, BESDataHandlerInterface *dhi, const string &localfile, const string &ncVersion = "netcdf");
    virtual ~FONcTransform();
	virtual void transform_dap2(ostream &strm);
	virtual void transform_dap4();


	virtual void dump(ostream &strm) const;
private:
    virtual void transform_dap4_no_group();
    virtual void transform_dap4_group(D4Group*,bool is_root, int par_grp_id,std::map<std::string,int>&,std::vector<int>&);
    virtual void transform_dap4_group_internal(D4Group*,bool is_root, int par_grp_id,std::map<std::string,int>&,std::vector<int>&);
    virtual void check_and_obtain_dimensions(D4Group*grp,bool);
    virtual void check_and_obtain_dimensions_internal(D4Group*grp);
    virtual bool check_group_support();
    virtual void gen_included_grp_list(D4Group*grp);

    virtual bool is_streamable();
    virtual bool is_dds_streamable();
    virtual bool is_dmr_streamable(D4Group *group);

};

#endif // FONcTransfrom_h_

