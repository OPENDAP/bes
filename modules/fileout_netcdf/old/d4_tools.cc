// d4_tools.cc

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
//      slloyd      Samuel Lloyd <slloyd@opendap.org>

#include <libdap/D4Group.h>
#include <libdap/D4Attributes.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>

#include <exception>

#include <BESInternalError.h>
#include <vector>

#include "d4_tools.h"
#include "FONcAttributes.h"
#include "FONcTransmitter.h"
#include "history_utils.h"

#include "DMR.h"
#include "DDS.h"

using namespace libdap;
using namespace std;

#define prolog std::string("d4_tools::").append(__func__).append("() - ")

bool d4_tools::has_dap4_types(D4Attribute *attr)
{
    bool has_d4_attr = false;
    switch(attr->type()){
        case attr_int8_c:
        case attr_int64_c:
        case attr_uint64_c:
            has_d4_attr=true;
            break;
        case attr_container_c:
            has_d4_attr = has_dap4_types(attr->attributes());
            break;
        default:
            break;
    }
    return has_d4_attr;
}

bool d4_tools::has_dap4_types(const D4Attributes *attrs)
{
    bool has_d4_attr = false;
    for (auto attr: attrs->attributes()) {
        if(has_dap4_types(attr))
            has_d4_attr |= has_dap4_types(attr);
    }
    return has_d4_attr;
}

bool d4_tools::is_dap4_projected(libdap::BaseType *var, vector<BaseType *> &inv) {

    switch(var->type()){
        case libdap::dods_int8_c:
        case libdap::dods_int64_c:
        case libdap::dods_uint64_c:
            inv.push_back(var);
            return true;
        case libdap::dods_array_c: {
            auto *a = dynamic_cast<Array*>(var);
            if (d4_tools::is_dap4_projected(a->var(), inv)) {
                inv.pop_back(); // removing array template type ...
                inv.push_back(var); // ... and add array type into inventory
                return true;
            }
            break;
            }
        case libdap::dods_structure_c:
        case libdap::dods_grid_c:
        case libdap::dods_sequence_c: {
            auto *svar = dynamic_cast<Constructor*>(var);
            bool is_dap4 = false;
            for (auto bvar = svar->var_begin(), evar = svar->var_end(); bvar != evar; ++bvar){

                if (d4_tools::is_dap4_projected(*bvar, inv)){
                    is_dap4 = true;
                }
            }
            if(is_dap4) return true;
        }
        default:
            break;
    }
    // Projected variable might not be DAP4, but what about the attributes?
    if(has_dap4_types(var->attributes())){
        // It has DAP4 Attributes so that variable is a de-facto dap4 variable.
        inv.push_back(var);
        return true;
    }
    return false;
}

#if 1
bool d4_tools::is_dap4_projected(DDS *_dds, vector<BaseType *> &projected_dap4_variable_inventory) {
    bool has_dap4_var = false;
    for (auto btp = _dds->var_begin(), ve = _dds->var_end(); btp != ve; ++btp) {

        BaseType *var = *btp;
        if(var->send_p() && is_dap4_projected(var, projected_dap4_variable_inventory)){
                has_dap4_var = true;
        }
    }
    return has_dap4_var;
}
#endif

#if 1
bool d4_tools::is_dap4_projected(DMR *_dmr, vector<BaseType *> &projected_dap4_variable_inventory) {

    bool has_dap4_var = false;

    has_dap4_var = is_dap4_projected( _dmr->root(), projected_dap4_variable_inventory);

    return has_dap4_var;
}
#endif

#if 1
bool d4_tools::is_dap4_projected(D4Group *group, vector<BaseType *> &projected_dap4_variable_inventory){
    bool has_dap4_var = false;
    // variables in the group

    // check all variables in the root group
    for (auto btp = group->var_begin(), ve = group->var_end(); btp != ve; ++btp) {
        BaseType *var = *btp;

        if(var->send_p() && var->type() != libdap::dods_group_c){ //dods_group_c <- d4Groups
            if (is_dap4_projected(var, projected_dap4_variable_inventory)) {
                has_dap4_var = true;
            }
        }
        else if (var->send_p() && var->type() == libdap::dods_group_c) {
            try {
                D4Group *d4g = dynamic_cast<D4Group *>(var);
                if (is_dap4_projected(d4g, projected_dap4_variable_inventory)) {
                    has_dap4_var = true;
                }
            }
            catch (const exception &e){
                throw BESInternalError("Exception: " + string(e.what()), __FILE__, __LINE__);
            }
        }
    }

    //check if group has any subgroups and check them for dap4 projected variables
    for (auto gb = group->grp_begin(), ge = group->grp_end(); gb != ge; ++gb){
        try {
            D4Group *d4g = *gb;
            if (is_dap4_projected(d4g, projected_dap4_variable_inventory)) {
                has_dap4_var = true;
            }
        }
        catch (const exception &e){
            throw BESInternalError("Exception: " + string(e.what()), __FILE__, __LINE__);
        }

    }
    return has_dap4_var;
}
#endif