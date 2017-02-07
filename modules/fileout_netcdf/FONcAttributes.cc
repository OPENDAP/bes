// FONcAttributes.cc

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

#include <sstream>

using std::istringstream;

#include <netcdf.h>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESUtil.h>

#include "DapFunctionUtils.h"

#include "FONcAttributes.h"
#include "FONcUtils.h"

/** @brief Add the attributes for an OPeNDAP variable to the netcdf file
 *
 * This method writes out any attributes for the provided variable and for
 * any parent BaseType objects of this variable. For example, if this
 * variable is a part of a structure, which is a part of another structure,
 * then we write out all of the attributes for the outermost structure, the
 * inner structure, and the variable of the structure. For example, if there
 * is a structure Alpha with attribute a1, that contains a structure Point
 * with attribute s2, which contains two variables x, with attribute a3,
 * and y, with attribute a4, then variable Alpha.Point.x will contain
 * attributes Alpha.a1, Alpha.Point.s2, and a3 and Alpha.Point.y will
 * contain attributes Alpha.a1, Alpha.Point.s2, and a4.
 *
 * Attributes can also be attribute containers, and not just simple
 * attributes. In this case, just as with structures, we flatten out the
 * attribute containers. For example, if there is an attribute container
 * called name, with attributes first and last contained in it, then two
 * attributes are created called name.first and name.last.
 *
 * OPeNDAP can have multiple string values for a given attribute. This can
 * not be represented in netcdf3 (can in netcdf4). To accomplish this we
 * take each of the string values for the given attribute and append them
 * together using a newline as a separator (recommended by Unidata).
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id to associate the attributes to
 * @param b The OPeNDAP variable containing the attributes.
 * @throws BESInternalError if there is a problem writing the attributes for
 * the variable.
 */
void FONcAttributes::add_variable_attributes(int ncid, int varid, BaseType *b) {
    string emb_name;
    BaseType *parent = b->get_parent();
    if (parent) {
        FONcAttributes::add_variable_attributes_worker(ncid, varid, parent, emb_name);
    }
    // addattrs_workerA(ncid, varid, b, "");
    add_attributes(ncid, varid,  b->get_attr_table(), b->name(), "");

}

/** @brief writes any parent BaseType attributes out for a BaseType
 *
 * This function is used to dump the attributes of any parent
 * constructor classes for the variable. It starts with the outermost
 * parent of the variable.
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id to associate the attributes to
 * @param b The OPeNDAP variable containing the parent's attributes.
 * @param emb_name The name of the embedded BaseType
 * @throws BESInternalError if there is a problem writing the attributes for
 * the variable.
 */
void FONcAttributes::add_variable_attributes_worker(int ncid, int varid, BaseType *b, string &emb_name) {

    BaseType *parent = b->get_parent();
    if (parent) {
        FONcAttributes::add_variable_attributes_worker(ncid, varid, parent, emb_name);
    }
    if (!emb_name.empty()) {
        emb_name += FONC_EMBEDDED_SEPARATOR;
    }
    emb_name += b->name();
    // addattrs_workerA(ncid, varid, b, emb_name);
    add_attributes(ncid, varid,  b->get_attr_table(), b->name(), emb_name);
}


/** @brief helper function for add_attributes
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param attrs The OPenDAP AttrTable containing the attributes
 * @param var_name any variable name to prepend to the attribute name
 * @param prepend_attr Any name to prepend to the name of the attribute.
 * As far as I know, this is no longer used; when standard attributes are prefixed
 * client programs will (often) fail to recognize the standard attributes (since the
 * names are no longer really standard).
 * @throws BESInternalError if there are any problems writing out the
 * attributes for the data object.
 */
void FONcAttributes::add_attributes(int ncid, int varid, AttrTable &attrs, const string &var_name, const string &prepend_attr) {

    unsigned int num_attrs = attrs.get_size();
    if (num_attrs) {
        AttrTable::Attr_iter i = attrs.attr_begin();
        AttrTable::Attr_iter e = attrs.attr_end();
        for (; i != e; i++) {
            unsigned int num_vals = attrs.get_attr_num(i);
            if (num_vals) {
                add_attributes_worker(ncid, varid, var_name, attrs, i, prepend_attr);
            }
        }
    }
}



/** @brief helper function for add_attributes that writes out a single
 * attribute
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param var_name the name of the variable (flattened)
 * @param attrs the AttrTable that contains the attribute to be written
 * @param attr the iterator into the AttrTable for the attribute to be written
 * @param prepend_attr any attribute name to prepend to the name of this
 * attribute. Use of this parameter is deprecated.
 * @throws BESInternalError if there is a problem writing this attribute
 */
void FONcAttributes::add_attributes_worker(int ncid, int varid, const string &var_name,
        AttrTable &attrs, AttrTable::Attr_iter &attr,
        const string &prepend_attr) {

    AttrType attrType = attrs.get_attr_type(attr);

    string attr_name = attrs.get_name(attr);
    string new_attr_name("");
    if (!prepend_attr.empty()) {
        new_attr_name = prepend_attr + FONC_EMBEDDED_SEPARATOR + attr_name;
    }
    else {

        // If we're doing global attributes AND it's an attr table, and its name is "special"
        // (ends with "_GLOBAL"), then we suppress the use of the attrTable name in
        // the NetCDF Attributes name.
        if (varid == NC_GLOBAL  && attrType==Attr_container && BESUtil::endsWith(attr_name, "_GLOBAL")) {
            BESDEBUG("fonc",
                    "Suppressing global AttributeTable name '" << attr_name << "' from inclusion in NetCDF attributes namespace chain." << endl);
            new_attr_name = "";
        }
        else {
            new_attr_name = attr_name;
        }
    }

#if 0
    // This was the old way of doing it and it polluted the attribute names
    // by prepending full qualified variable names to the attribute name..
    string new_name = new_attr_name;
    if (!var_name.empty()) {
        new_name = var_name + FONC_ATTRIBUTE_SEPARATOR + new_attr_name;
    }

    // BESDEBUG("fonc","new_name: " << new_name << " new_attr_name: " << new_attr_name << " var_name: " << var_name << endl);

    new_name = FONcUtils::id2netcdf(new_name);
#endif

    string new_name = FONcUtils::id2netcdf(new_attr_name);;


    if (varid == NC_GLOBAL) {
        BESDEBUG("fonc", "FONcAttributes::addattrs() - Adding global attributes " << attr_name << endl);
    }
    else {
        BESDEBUG("fonc", "FONcAttributes::addattrs() - Adding attribute " << new_name << endl);
    }

    int stax = NC_NOERR;
    unsigned int attri = 0;
    unsigned int num_vals = attrs.get_attr_num(attr);
    switch (attrType) {
    case Attr_container: {
        // flatten
        BESDEBUG("fonc", "Attribute " << attr_name << " is an attribute container. new_attr_name: \"" << new_attr_name << "\"" << endl);
        AttrTable *container = attrs.get_attr_table(attr);
        if (container) {
            add_attributes(ncid, varid, *container, var_name, new_attr_name);
        }
    }
        break;
    case Attr_byte: {
        // unsigned char
        unsigned char vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            unsigned int uival = 0;
            is >> uival;
            vals[attri] = (unsigned char) uival;
        }
        stax = nc_put_att_uchar(ncid, varid, new_name.c_str(), NC_BYTE,
                num_vals, vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write byte attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_int16: {
        // short
        short vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            short sval = 0;
            is >> sval;
            vals[attri] = sval;
        }
        stax = nc_put_att_short(ncid, varid, new_name.c_str(), NC_SHORT,
                num_vals, vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write short attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_uint16: {
        // unsigned short
        // (needs to be big enough to store an unsigned short
        int vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            int ival = 0;
            is >> ival;
            vals[attri] = ival;
        }
        stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals,
                vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write unsinged short attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_int32: {
        // int
        int vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            int ival = 0;
            is >> ival;
            vals[attri] = ival;
        }
        stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals,
                vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write int attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_uint32: {
        // uint
        // needs to be big enough to store an unsigned int
        int vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            int lval = 0;
            is >> lval;
            vals[attri] = lval;
        }
        stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals,
                vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write byte attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_float32: {
        // float
        float vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            float fval = 0;
            is >> fval;
            vals[attri] = fval;
        }
        stax = nc_put_att_float(ncid, varid, new_name.c_str(), NC_FLOAT,
                num_vals, vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write float attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_float64: {
        // double
        double vals[num_vals];
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            double dval = 0;
            is >> dval;
            vals[attri] = dval;
        }
        stax = nc_put_att_double(ncid, varid, new_name.c_str(), NC_DOUBLE,
                num_vals, vals);
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write double attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_string:
    case Attr_url:
    case Attr_other_xml:    // Added. jhrg 12.27.2011
    {
        // string
        string val = attrs.get_attr(attr, 0);
        for (attri = 1; attri < num_vals; attri++) {
            val += "\n" + attrs.get_attr(attr, attri);
        }
        stax = nc_put_att_text(ncid, varid, new_name.c_str(), val.length(),
                val.c_str());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write string attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_unknown: {
        string err = (string) "File out netcdf, "
                + "failed to write unknown type of attribute " + new_name;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }
        break;
    }
}

/** @brief Adds an attribute for the variable if the variable name had
 * to be modified in any way
 *
 * When generating a new name for a variable, the original name might
 * contain characters that are not allowed in netcdf files. For this
 * reason, the characters are modified and we add an attribute to the
 * netcdf file for that variable to signify the change.
 *
 * @param ncid The id of the netcdf open file
 * @param varid The id of the variable to add the attribute to
 * @param var_name The name of the variable as it appears in the new * file
 * @param orig The name of the variable before it was modified
 */
void FONcAttributes::add_original_name(int ncid, int varid,
        const string &var_name, const string &orig) {
    if (var_name != orig) {
        string attr_name = FONC_ORIGINAL_NAME;
        int stax = nc_put_att_text(ncid, varid, attr_name.c_str(),
                orig.length(), orig.c_str());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                    + "failed to write change of name attribute for "
                    + var_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
}

