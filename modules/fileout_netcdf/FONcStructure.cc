// FONcStructure.cc

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

#include <libdap/util.h>        // is_simple_type()
#include <libdap/Array.h>
#include <BESInternalError.h>
#include <BESDebug.h>

#include "FONcStructure.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

/** @brief Constructor for FONcStructure that takes a DAP Structure
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Structure instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be a Structure
 * @throws BESInternalError if the BaseType is not a Structure
 */
FONcStructure::FONcStructure(BaseType *b) :
    FONcBaseType(), _s(0)
{
    _s = dynamic_cast<Structure *>(b);
    if (!_s) {
        string s = (string) "File out netcdf, write_structure was passed a " + "variable that is not a structure";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    
    for (const auto &bt:_s->variables()) {
         
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
            can_handle_str_memb = false;
            break;
        }
        
    }
}

/** @brief Destructor that cleans up the structure
 *
 * Delete each of the FONcBaseType instances that is a part of this
 * structure.
 */
FONcStructure::~FONcStructure()
{
    bool done = false;
    while (!done) {
        vector<FONcBaseType *>::iterator i = _vars.begin();
        vector<FONcBaseType *>::iterator e = _vars.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONc types, not the actual ones
            FONcBaseType *b = (*i);
            delete b;
            _vars.erase(i);
        }
    }
}

/** @brief Creates the FONc objects for each variable of the DAP structure
 *
 * For each of the variables of the DAP Structure we convert to a
 * similar FONc object. Because NetCDF does not support structures, we
 * must flatten out the structures. To do this, we embed the name of the
 * structure as part of the name of the children variables. For example,
 * if the structure, called s1, contains an array called a1 and an int
 * called i1, then two variables are created in the netcdf file called
 * s1.a1 and s1.i1.
 *
 * @note This method only converts the variables that are to be sent. Thsi keeps
 * the convert() and write() methods below from operating on DAP variables
 * that should not be sent.
 *
 * @param embed The parent names of this structure.
 * @throws BESInternalError if there is a problem converting this
 * structure
 */
void FONcStructure::convert(vector<string> embed,bool _dap4, bool is_dap4_group)
{

    if (can_handle_str_memb) {
        if (_dap4)
            _s->intern_data();
        else
            _s->intern_data(*get_eval(),*get_dds());
    }

    FONcBaseType::convert(embed,_dap4,is_dap4_group);
    embed.push_back(name());
    Constructor::Vars_iter vi = _s->var_begin();
    Constructor::Vars_iter ve = _s->var_end();
    for (; vi != ve; vi++) {
        BaseType *bt = *vi;
        if (bt->send_p()) {
            BESDEBUG("fonc", "FONcStructure::convert - converting " << bt->name() << endl);
            bool is_classic_model = true;
            if(true == isNetCDF4_ENHANCED())
                is_classic_model = false;
            FONcBaseType *fbt = FONcUtils::convert(bt, this->d_ncVersion, is_classic_model);
            _vars.push_back(fbt);
            fbt->convert(embed,_dap4,is_dap4_group);
        }
    }
}

/** @brief Define the members of the structure in the netcdf file
 *
 * Since netcdf does not support structures, we define the members of
 * the structure to include the name of the structure in their name.
 *
 * @note This will call the FONcBaseType's define() method for the FONcBaseType
 * variables. Because the FONcStructure::convert() method above only
 * builds a FONcBaseType for elements of the DAP Structure with send_p true,
 * the code is protected from trying to operate on DAP variables that should
 * not be sent. This is important because those variables likely don't contain
 * any values!
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem defining the structure
 */
void FONcStructure::define(int ncid)
{
    if (!d_defined) {
        BESDEBUG("fonc", "FONcStructure::define - defining " << d_varname << endl);

        vector<FONcBaseType *>::const_iterator i = _vars.begin();
        vector<FONcBaseType *>::const_iterator e = _vars.end();
        for (; i != e; i++) {
            FONcBaseType *fbt = (*i);
            BESDEBUG("fonc", "defining " << fbt->name() << endl);
            fbt->define(ncid);
        }

        d_defined = true;

        BESDEBUG("fonc", "FONcStructure::define - done defining " << d_varname << endl);
    }
}

/** @brief write the member variables of the structure to the netcdf
 * file
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing out the
 * members of the structure.
 */
void FONcStructure::write(int ncid)
{

    if (can_handle_str_memb == false) {
        if (d_is_dap4) 
            _s->intern_data();
        else 
            _s->intern_data(*get_eval(),*get_dds());
    }
    BESDEBUG("fonc", "FONcStructure::write - writing " << d_varname << endl);
    vector<FONcBaseType *>::const_iterator i = _vars.begin();
    vector<FONcBaseType *>::const_iterator e = _vars.end();
    for (; i != e; i++) {
        FONcBaseType *fbt = (*i);

        fbt->set_dds(get_dds());
        fbt->set_eval(get_eval());

        fbt->write(ncid);
        nc_sync(ncid);
    }
    BESDEBUG("fonc", "FONcStructure::define - done writing " << d_varname << endl);
}

/** @brief Returns the name of the structure
 *
 * @returns The name of the structure
 */
string FONcStructure::name()
{
    return _s->name();
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including the members of the structure by calling dump on those FONc
 * objects.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcStructure::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONcStructure::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << _s->name() << " {" << endl;
    BESIndent::Indent();
    vector<FONcBaseType *>::const_iterator i = _vars.begin();
    vector<FONcBaseType *>::const_iterator e = _vars.end();
    for (; i != e; i++) {
        FONcBaseType *fbt = *i;
        fbt->dump(strm);
    }
    BESIndent::UnIndent();
    strm << BESIndent::LMarg << "}" << endl;
    BESIndent::UnIndent();
}

