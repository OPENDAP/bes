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
//      kyang       Kent Yang <myang6@hdfgroup.org> for the DAP4 support.

#ifndef FONcTransfrom_h_
#define FONcTransfrom_h_ 1

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
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

/** @brief Transformation object that converts an OPeNDAP DataDDS or DMR to a
 * netcdf file
 *
 * This class transforms each variable of the DataDDS/DMR to a netcdf file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONcTransform: public BESObj {
private:
    int _ncid = 0;
    libdap::DDS *_dds = nullptr;
    libdap::DMR *_dmr = nullptr;
    BESResponseObject *d_obj = nullptr;
    BESDataHandlerInterface *d_dhi = nullptr;
    std::string _localfile;
    std::string _returnAs;

    // The following variables are for DMR group support.
    std::vector<FONcBaseType *> _fonc_vars;
    std::vector<FONcBaseType *> _total_fonc_vars_in_grp;
    std::set<std::string> _included_grp_names;
    std::map<std::string,int64_t> GFQN_dimname_to_dimsize;
    std::map<std::string,int64_t> VFQN_dimname_to_dimsize;

    // The following is for enum support.
    std::unordered_map<std::string,std::vector<std::pair<std::string,int>>> GFQN_to_en_typeid_vec;

    // Flag to check if we can do the direct IO optimization. 
    // If this flag is false, we cannot do direct IO for this file at all.
    // Even if this flag is true, it doesn't mean we can do direct IO for every variable. 
    // Other conditions need to be fulfilled.
    bool global_dio_flag = false; 

    // The module may add fake dimension name if dimension names are not provided. 
    // The fake dimension names are just dim0, dim1,dim2 etc regardless of the dimension size.
    // The following members are used to reduce the dimension names by having the 
    // one dimension name for any dimension that has the same dimension size.
    bool do_reduce_dim = false;
    std::unordered_map<int64_t, std::vector<std::string>> dimsize_to_dup_dimnames;
    int reduced_dim_num = 0;

    // Handle unlimited dimensions under the root.
    bool is_root_no_grp_unlimited_dim = false;
    std::vector<std::string> root_no_grp_unlimited_dimnames; 

public:
    FONcTransform(BESResponseObject *obj, BESDataHandlerInterface *dhi, const std::string &localfile, const std::string &ncVersion = "netcdf");
    virtual ~FONcTransform();
    virtual void transform_dap2();
    virtual void transform_dap4();

    virtual void dump(ostream &strm) const;


private:
    virtual void transform_dap4_no_group();
    virtual void transform_dap4_group(libdap::D4Group*,bool is_root, int par_grp_id, std::map<std::string, int>&, std::vector<int>&);
    virtual void transform_dap4_group_internal(libdap::D4Group*, bool is_root, int par_grp_id, std::map<std::string, int>&, std::vector<int>&);
    virtual void check_and_obtain_dimensions(libdap::D4Group *grp, bool);
    virtual void check_and_obtain_dimensions_internal(libdap::D4Group *grp);
    virtual bool check_group_support();
    virtual void gen_included_grp_list(libdap::D4Group *grp);
    virtual void gen_nc4_enum_type(libdap::D4Group *d4_grp,int nc4_grp_id);

    virtual bool check_reduce_dim();
    virtual bool check_reduce_dim_internal(libdap::D4Group *grp);
    virtual bool check_var_dim(libdap::BaseType *bt);
    virtual void build_reduce_dim();
    virtual void build_reduce_dim_internal(libdap::D4Group *grp, libdap::D4Group *root_grp);

    virtual bool obtain_unlimited_dimension_info_helper(libdap::D4Attributes *d4_attrs, vector<string> &unlimited_dim_names);
    virtual bool obtain_unlimited_dimension_info(libdap::D4Group *grp, std::vector<std::string> &unlimited_dim_names);
    virtual bool check_var_unlimited_dimension(libdap::Array *t_a, const std::vector<std::string> &unlimited_dim_names);

    void throw_if_dap2_response_too_big(DDS *dds, const string &dap2_ce="");
    void throw_if_dap4_response_too_big(DMR *dmr, const string &dap4_ce="");
    string too_big_error_msg(
            const unsigned dap_version,
            const string &return_encoding,
            const unsigned long long config_max_response_size_kb,
            const unsigned long long  contextual_max_response_size_kb,
            const string &ce);
    void set_max_size_and_encoding(unsigned long long &max_request_size_kb, string &return_encoding);

    //void set_constraint_var_dio_flag(libdap::BaseType*) const;
    void set_constraint_var_dio_flag(libdap::Array*) const;

};

#endif // FONcTransfrom_h_

