// FONcArrayStructure.cc

// This file is part of BES Netcdf File Out Module

// // Copyright (c) The HDF Group, Inc. and OPeNDAP, Inc.
//
// This is free software; you can redistribute it and/or modify it under the
// terms of the GNU Lesser General Public License as published by the Free
// Software Foundation; either version 2.1 of the License, or (at your
// option) any later version.
//
// This software is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact OPeNDAP, Inc. at PO Box 112, Saunderstown, RI. 02874-0112.
// You can contact The HDF Group, Inc. at 410 E University Ave,
// Suite 200, Champaign, IL 61820

#include <libdap/util.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcArrayStructure.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcArrayStructure that takes a DAP Array Structure
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP Array
 * Structure instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be an array of Structure
 * @throws BESInternalError if the BaseType is not a an array
 */
FONcArrayStructure::FONcArrayStructure(BaseType *b) :
    FONcBaseType(),_as(dynamic_cast<Array*>(b))
{
    if (!_as) {
        string s = "File out netcdf, array of structure was passed a variable that is not an array ";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (_as->var()->type() != dods_structure_c) {
        string s =  "File out netcdf, array of structure was passed a variable that is not a structure ";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
}

/** @brief Destructor that cleans up the array of structure
 *
 * Delete each of the FONcBaseType instances that is a part of this
 * structure.
 */
FONcArrayStructure::~FONcArrayStructure()
{
    for (auto &var: _vars) {
        delete var;
    }

}

/** @brief Creates the FONc objects for each variable of the DAP array of structure
 *
 * For each of the variables of the DAP Structure we convert to a
 * similar FONc object. We still follow the current way to map structure to netCDF-3.
 * We must flatten out the structures. To do this, we embed the name of the
 * structure as part of the name of the children variables. For example,
 * if the structure, called s1, contains an array called a1 and an int
 * called i1, then two variables are created in the netcdf file called
 * s1.a1 and s1.i1.
 *
 * @note This method only converts the variables that are to be sent. This keeps
 * the convert() and write() methods below from operating on DAP variables
 * that should not be sent.
 *
 * @param embed The parent names of this structure.
 * @throws BESInternalError if there is a problem converting this
 * structure
 */
void FONcArrayStructure::convert(vector<string> embed, bool _dap4, bool is_dap4_group){
    set_is_dap4(_dap4);
    FONcBaseType::convert(embed,_dap4,is_dap4_group);
    embed.push_back(name());
#if 0
    if (d_is_dap4)
        _as->intern_data();
    else 
        _as->intern_data(*get_eval(),*get_dds());
#endif

    auto as_v = dynamic_cast<Structure *>(_as->var());
    if (!as_v) {
        string s = "File out netcdf, write an array of structure was passed a variable that is not a structure";
        throw BESInternalError(s, __FILE__, __LINE__);
    }

    if (false == isNetCDF4_ENHANCED()) {
        throw BESInternalError("Fileout netcdf: We don't support array of structure for the classical model now. ",
                               __FILE__, __LINE__);
    }

    
    for (auto &bt: as_v->variables()) {

        Type t_bt = bt->type();
        // Only support array or scalar of float/int/string.
        if (libdap::is_simple_type(t_bt) == false) {

            if (t_bt != dods_array_c) {
                can_handle_str_memb = false;
                break;
            }
            else {
                auto t_a = dynamic_cast<Array *>(bt);
                Type t_array_var = t_a->var()->type();
                if (!libdap::is_simple_type(t_array_var) || t_array_var == dods_url_c || t_array_var == dods_enum_c || t_array_var==dods_opaque_c) {
                    can_handle_str_memb = false;
                    break;
                }
            }
        }
        else if (t_bt == dods_url_c || t_bt == dods_enum_c || t_bt==dods_opaque_c) {
            can_handle_str_memb= false;
            break;
        }
    }
    
    if(can_handle_str_memb) {
    if (d_is_dap4)
        _as->intern_data();
    else 
        _as->intern_data(*get_eval(),*get_dds());
    }


    for (auto &bt: as_v->variables()) {
        if (bt->send_p()) {
            BESDEBUG("fonc", "FONcArrayStructure::convert - converting " << bt->name() << endl);
            auto fsf = new FONcArrayStructureField(bt, _as, isNetCDF4_ENHANCED());
            _vars.push_back(fsf);
            fsf->convert(embed,_dap4,is_dap4_group);
        }
    }

}

/** @brief Define the members of the array of structure in the netcdf file
 *
 * We still follow the current structure implementation to define the members of
 * the structure to include the name of the structure in their name.
 *
 * @note This will call the FONcArrayStructureField's define() method for individual member.
 * Because the FONcArrayStructure::convert() method above only
 * builds a FONcArrayStructure for elements of the DAP Structure with send_p true,
 * the code is protected from trying to operate on DAP variables that should
 * not be sent. This is important because those variables likely don't contain
 * any values!
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem defining the structure
 */
void FONcArrayStructure::define(int ncid)
{

    if (!d_defined) {
        BESDEBUG("fonc", "FONcArrayStructure::define - defining " << d_varname << endl);

        for (auto &fbt:_vars) {
            BESDEBUG("fonc", "defining " << fbt->name() << endl);
            fbt->define(ncid);
        }

        d_defined = true;

        BESDEBUG("fonc", "FONcArrayStructure::define - done defining " << d_varname << endl);
    }

}

/** @brief write the member variables of the structure to the netcdf
 * file
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing out the
 * members of the structure.
 */
void FONcArrayStructure::write(int ncid)
{

    BESDEBUG("fonc", "FONcArrayStructure::write - writing " << d_varname << endl);
    if(!can_handle_str_memb) {
    if (d_is_dap4)
        _as->intern_data();
    else
        _as->intern_data(*get_eval(),*get_dds());
    }

    for (const auto &var:_vars) {
        var->write(ncid);
    }
    BESDEBUG("fonc", "FONcArrayStructure::define - done writing " << d_varname << endl);
}

/** @brief Returns the name of the structure
 *
 * @returns The name of the structure
 */
string FONcArrayStructure::name()
{
    return _as->name();
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including the members of the structure by calling dump on those FONc
 * objects.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcArrayStructure::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcArrayStructure::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _as->name() << " {" << endl;
    BESIndent::Indent();
    for (const auto & _var:_vars)
        _var->dump(strm);
    BESIndent::UnIndent();
    strm << BESIndent::LMarg << "}" << endl;
    BESIndent::UnIndent();
}

