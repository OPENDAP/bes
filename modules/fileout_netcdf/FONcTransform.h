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

#include <string>
#include <vector>
#include <map>
#include <set>

#include <BESObj.h>

namespace libdap {
class DDS;
class DMR;
class D4Group;
}

class FONcBaseType;
class BESResponseObject;
class BESDataHandlerInterface;

/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * netcdf file
 *
 * This class transforms each variable of the DataDDS to a netcdf file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONcTransform: public BESObj {
private:
	int _ncid = {0};
	libdap::DDS *_dds = {nullptr};
    libdap::DMR *_dmr = {nullptr};
    BESResponseObject *d_obj = {nullptr};
    BESDataHandlerInterface *d_dhi = {nullptr};
	std::string _localfile;
	std::string _returnAs;
    std::vector<FONcBaseType *> _fonc_vars;
    std::vector<FONcBaseType *> _total_fonc_vars_in_grp;
    std::set<std::string> _included_grp_names;
    std::map<std::string,int64_t> GFQN_dimname_to_dimsize;
    std::map<std::string,int64_t> VFQN_dimname_to_dimsize;
  
    // TODO: This is for the temporary memory usage optimization. Once we can support the define() with or without dio for individual array.
    //       This flag is not necessary and should be removed. KY 11/29/23
    bool global_dio_flag = false; 

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
	FONcTransform(libdap::DDS *dds, BESDataHandlerInterface &dhi, const std::string &localfile, const std::string &netcdfVersion = "netcdf");
	FONcTransform(libdap::DMR *dmr, BESDataHandlerInterface &dhi, const std::string &localfile, const std::string &netcdfVersion = "netcdf");
    FONcTransform(BESResponseObject *obj, BESDataHandlerInterface *dhi, const std::string &localfile, const std::string &ncVersion = "netcdf");
    virtual ~FONcTransform();
	virtual void transform_dap2(ostream &strm);
	virtual void transform_dap4();

	virtual void dump(ostream &strm) const;

        // TODO: This is for the temporary memory usage optimization. Once we can support the define() with or without dio for individual array.
        //       This flag is not necessary and should be removed. KY 11/29/23
        bool get_gdio_flag() const {return global_dio_flag; }
        void set_gdio_flag() { global_dio_flag = true; }


private:
    virtual void transform_dap4_no_group();
    virtual void transform_dap4_group(libdap::D4Group*,bool is_root, int par_grp_id, std::map<std::string, int>&, std::vector<int>&);
    virtual void transform_dap4_group_internal(libdap::D4Group*, bool is_root, int par_grp_id, std::map<std::string, int>&, std::vector<int>&);
    virtual void check_and_obtain_dimensions(libdap::D4Group *grp, bool);
    virtual void check_and_obtain_dimensions_internal(libdap::D4Group *grp);
    virtual bool check_group_support();
    virtual void gen_included_grp_list(libdap::D4Group *grp);

    virtual bool is_streamable();
    virtual bool is_dds_streamable();
    virtual bool is_dmr_streamable(libdap::D4Group *group);
    void throw_if_dap2_response_too_big(DDS *dds, const string &dap2_ce="");
    void throw_if_dap4_response_too_big(DMR *dmr, const string &dap4_ce="");
    string too_big_error_msg(
            const unsigned dap_version,
            const string &return_encoding,
            const unsigned long long config_max_response_size_kb,
            const unsigned long long  contextual_max_response_size_kb,
            const string &ce);
    void set_max_size_and_encoding(unsigned long long &max_request_size_kb, string &return_encoding);

};

#endif // FONcTransfrom_h_

