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

#include <netcdf.h>

#include <BESDebug.h>
#include <BESInternalError.h>
#include <BESUtil.h>
#include <cstdlib>

#include "DapFunctionUtils.h"

#include "FONcAttributes.h"
#include "FONcUtils.h"

using namespace std;

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
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing the attributes for
 * the variable.
 */
void FONcAttributes::add_variable_attributes(int ncid, int varid, BaseType *b, bool is_nc_enhanced, bool is_dap4) {
    string emb_name;
    BaseType *parent = b->get_parent();
    if (parent) {
        //BESDEBUG("dap", "FONcAttributes::parent name is "<< parent->name() <<endl);
        //BESDEBUG("dap", "FONcAttributes::parent type is "<< parent->type() <<endl);
        if (true != is_dap4 || parent->type() != dods_group_c)
            FONcAttributes::add_variable_attributes_worker(ncid, varid, parent, emb_name, is_nc_enhanced, is_dap4);
    }
    // addattrs_workerA(ncid, varid, b, "");
    // Add DAP4 attribute support by using attributes().

    BESDEBUG("dap", "FONcAttributes::add_variable_attributes() after parent " << endl);
    if (is_dap4)
        add_dap4_attributes(ncid, varid, b->attributes(), b->name(), "", is_nc_enhanced);
    else
        add_attributes(ncid, varid, b->get_attr_table(), b->name(), "", is_nc_enhanced);

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
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing the attributes for
 * the variable.
 */
void FONcAttributes::add_variable_attributes_worker(int ncid, int varid, BaseType *b, string &emb_name,
                                                    bool is_nc_enhanced, bool is_dap4) {

    BaseType *parent = b->get_parent();
    if (parent) {
        FONcAttributes::add_variable_attributes_worker(ncid, varid, parent, emb_name, is_nc_enhanced, is_dap4);
    }
    if (!emb_name.empty()) {
        emb_name += FONC_EMBEDDED_SEPARATOR;
    }
    emb_name += b->name();
    // addattrs_workerA(ncid, varid, b, emb_name);
    if (is_dap4)
        add_dap4_attributes(ncid, varid, b->attributes(), b->name(), emb_name, is_nc_enhanced);
    else
        add_attributes(ncid, varid, b->get_attr_table(), b->name(), emb_name, is_nc_enhanced);
}


/** @brief helper function for add_attributes
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param attrs The OPeNDAP AttrTable containing the attributes
 * @param var_name A string to prepend to the attribute name
 * @param prepend_attr Any name to prepend to the name of the attribute.
 * As far as I know, this is no longer used; when standard attributes are prefixed
 * client programs will (often) fail to recognize the standard attributes (since the
 * names are no longer really standard).
 * @param is_nc_enhanced A flag to indicate if we want to map datatypes to netCDF-4
 *
 * @throws BESInternalError if there are any problems writing out the
 * attributes for the data object.
 */
void FONcAttributes::add_attributes(int ncid, int varid, AttrTable &attrs, const string &var_name,
                                    const string &prepend_attr, bool is_nc_enhanced) {

    unsigned int num_attrs = attrs.get_size();
    if (num_attrs) {
        AttrTable::Attr_iter i = attrs.attr_begin();
        AttrTable::Attr_iter e = attrs.attr_end();
        for (; i != e; i++) {
            unsigned int num_vals = attrs.get_attr_num(i);
            if (num_vals) {
                add_attributes_worker(ncid, varid, var_name, attrs, i, prepend_attr, is_nc_enhanced);
            }
        }
    }
}

/** @brief  add_dap4_attributes
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param attrs The OPenDAP DAP4 attributes
 * @param var_name any variable name to prepend to the attribute name
 * @param prepend_attr Any name to prepend to the name of the attribute.
 * As far as I know, this is no longer used; when standard attributes are prefixed
 * client programs will (often) fail to recognize the standard attributes (since the
 * names are no longer really standard).
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there are any problems writing out the
 * attributes for the data object.
 */
void FONcAttributes::add_dap4_attributes(int ncid, int varid, D4Attributes *d4_attrs, const string &var_name,
                                         const string &prepend_attr, bool is_nc_enhanced) {

    BESDEBUG("dap", "FONcAttributes::add_dap4_attributes() number of attributes " << d4_attrs << endl);
    for (D4Attributes::D4AttributesIter ii = d4_attrs->attribute_begin(), ee = d4_attrs->attribute_end();
         ii != ee; ++ii) {
        string name = (*ii)->name();
        unsigned int num_vals = (*ii)->num_values();
        if (num_vals || varid == NC_GLOBAL)
            add_dap4_attributes_worker(ncid, varid, var_name, *ii, prepend_attr, is_nc_enhanced);
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
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing this attribute
 */
void FONcAttributes::add_attributes_worker(int ncid, int varid, const string &var_name,
                                           AttrTable &attrs, AttrTable::Attr_iter &attr,
                                           const string &prepend_attr, bool is_nc_enhanced) {

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
        if (varid == NC_GLOBAL && attrType == Attr_container && BESUtil::endsWith(attr_name, "_GLOBAL")) {
            BESDEBUG("fonc", "Suppressing global AttributeTable name '" << attr_name
                << "' from inclusion in NetCDF attributes namespace chain." << endl);
            new_attr_name = "";
        }
        else {
            new_attr_name = attr_name;
        }
    }

    if (attrs.get_attr_num(attr_name) >1 && new_attr_name == "_FillValue")
        new_attr_name = "Multi_FillValues";

//Note: Leave the following #if 0 #endif block for the time being. Don't change to NBEBUG.
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
    BESDEBUG("fonc", "FONcAttributes name:  " << new_name << endl);
    BESDEBUG("fonc", "FONcAttributes type:  " << attrType << endl);


    if (varid == NC_GLOBAL) {
        BESDEBUG("fonc", "FONcAttributes::add_attributes_worker() - Adding global attributes " << attr_name << endl);
    }
    else {
        BESDEBUG("fonc", "FONcAttributes::add_attributes_worker() - Adding attribute " << new_name << endl);
    }

    // If we want to map the attributes of the datatypes to those of netCDF-4, KY 2020-02-14
    if (is_nc_enhanced == true)
        write_attrs_for_nc4_types(ncid, varid, var_name, new_attr_name, new_name, attrs, attr, is_nc_enhanced);
    else {
        int stax = NC_NOERR;
        unsigned int attri = 0;
        unsigned int num_vals = attrs.get_attr_num(attr);
        switch (attrType) {
            case Attr_container: {
                // flatten
                BESDEBUG("fonc",
                         "Attribute " << attr_name << " is an attribute container. new_attr_name: \"" << new_attr_name
                                      << "\"" << endl);
                AttrTable *container = attrs.get_attr_table(attr);
                if (container) {
                    add_attributes(ncid, varid, *container, var_name, new_attr_name, is_nc_enhanced);
                }
            }
                break;
            case Attr_byte: {
                // unsigned char
                // This should be converted to short to be consistent with the array. 
                // The classic model doesn't support unsigned char.
                vector<short> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    istringstream is(val);
                    unsigned int uival = 0;
                    is >> uival;
                    vals[attri] = (short) uival;
                }
                stax = nc_put_att_short(ncid, varid, new_name.c_str(), NC_SHORT,  num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, failed to write byte attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;
            case Attr_int16: {
                // short
                vector<short> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    istringstream is(val);
                    short sval = 0;
                    is >> sval;
                    vals[attri] = sval;
                }
                stax = nc_put_att_short(ncid, varid, new_name.c_str(), NC_SHORT, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write short attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;
            case Attr_uint16: {
                // unsigned short
                // (needs to be big enough to store an unsigned short
                vector<int> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    istringstream is(val);
                    int ival = 0;
                    is >> ival;
                    vals[attri] = ival;
                }
                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write unsinged short attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;
            case Attr_int32: {
                // int
                vector<int> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    istringstream is(val);
                    int ival = 0;
                    is >> ival;
                    vals[attri] = ival;
                }
                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write int attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;
            case Attr_uint32: {
                // uint
                // needs to be big enough to store an unsigned int
                string err = (string) "File out netcdf, " + "failed to write unsigned int attribute " + new_name;
                err = err + " for classic model because of potential overflow. ";
                err = err + " Please use the netCDF4 enhanced model. ";
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                // Note: the following #if 0 #endif is for reminder to developers only.
                // Don't change it to NBEDBUG.
#if 0
                vector<int>vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    istringstream is(val);
                    int lval = 0;
                    is >> lval;
                    vals[attri] = lval;
                }
                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals,
                                      vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, "
                                 + "failed to write unsigned int attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
#endif
            }
                break;
            case Attr_float32: {
                // float
                vector<float> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    const char *cval = val.c_str();
                    //istringstream is(val);
                    float fval = 0;
                    fval = strtod(cval, NULL);
                    //is >> fval;
                    vals[attri] = fval;
                }
                stax = nc_put_att_float(ncid, varid, new_name.c_str(), NC_FLOAT, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write float attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;
            case Attr_float64: {
                // double
                vector<double> vals;
                vals.resize(num_vals);
                for (attri = 0; attri < num_vals; attri++) {
                    string val = attrs.get_attr(attr, attri);
                    const char *cval = val.c_str();
                    //istringstream is(val);
                    double dval = 0;
                    dval = strtod(cval, NULL);
                    //is >> dval;
                    vals[attri] = dval;
                }
                stax = nc_put_att_double(ncid, varid, new_name.c_str(), NC_DOUBLE, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write double attribute " + new_name;
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
                if (attr_name != "_FillValue") {
                    stax = nc_put_att_text(ncid, varid, new_name.c_str(), val.size(), val.c_str());
                }
                else {
                    BESDEBUG("fonc",
                             "FONcAttributes::add_attributes_worker - Original attribute value is first character: "
                                     << val.c_str()[0] << endl);
                    stax = nc_put_att_text(ncid, varid, new_name.c_str(), 1, val.c_str());
                    if (stax == NC_NOERR) {
                        // New name for attribute _FillValue with original value
                        string new_name_fillvalue = "Orig_FillValue";
                        BESDEBUG("fonc",
                                 "FONcAttributes::add_attributes_worker - New attribute value is original value: "
                                         << val.c_str() << endl);
                        // This line causes the segmentation fault since attrs is changed and the original iterator of attrs doesn't exist anymore.
                        // So it causes the segmentation fault when next attribute is fetched in the for loop of the add_attributes(). KY 2019-12-13
#if 0
                        attrs.append_attr(new_name_fillvalue,"String", val);
#endif
                        stax = nc_put_att_text(ncid, varid, new_name_fillvalue.c_str(), val.size(), val.c_str());
                    }
                }

                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf, " + "failed to write string attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;

            case Attr_unknown: {
                string err = (string) "File out netcdf, " + "failed to write unknown type of attribute " + new_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
                break;
        }
    }
}

/** @brief helper function for add_dap4_attributes that writes out a single
 * attribute
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param var_name the name of the variable (flattened)
 * @param attr the DAP4 attribute to be written
 * @param prepend_attr any attribute name to prepend to the name of this
 * attribute. Use of this parameter is deprecated.
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing this attribute
 */
void FONcAttributes::add_dap4_attributes_worker(int ncid, int varid, const string &var_name,
                                                D4Attribute *attr,
                                                const string &prepend_attr, bool is_nc_enhanced) {
    D4AttributeType d4_attr_type = attr->type();

    string d4_attr_name = attr->name();
    BESDEBUG("dap", "FONcAttributes:: D4 attribute name is " << d4_attr_name << endl);
    string new_attr_name("");
    if (!prepend_attr.empty()) {
        new_attr_name = prepend_attr + FONC_EMBEDDED_SEPARATOR + d4_attr_name;
        BESDEBUG("dap", "FONcAttributes:: D4 new attribute name is " << new_attr_name << endl);
    }
    else {

        // If we're doing global attributes AND it's an attr table, and its name is "special"
        // (ends with "_GLOBAL"), then we suppress the use of the attrTable name in
        // the NetCDF Attributes name.
        if (varid == NC_GLOBAL && d4_attr_type == attr_container_c
            && (BESUtil::endsWith(d4_attr_name, "_GLOBAL")
            || BESUtil::endsWith(d4_attr_name, "HDF5_GLOBAL_integer_64"))) {
            BESDEBUG("fonc", "Suppressing global AttributeTable name '" << d4_attr_name
                << "' from inclusion in NetCDF attributes namespace chain." << endl);
            new_attr_name = "";
        }
        else {
            new_attr_name = d4_attr_name;
        }
    }

    if (attr->num_values() >1 && new_attr_name == "_FillValue")
        new_attr_name = "Multi_FillValues";



//Note: Leave the following #if 0 #endif block for the time being. Don't change to NBEBUG.
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

#if !NDEBUG
    if (varid == NC_GLOBAL) {
        BESDEBUG("fonc", "FONcAttributes::addattrs() - Adding global attributes " << d4_attr_name << endl);
    }
    else {
        BESDEBUG("fonc", "FONcAttributes::addattrs() - Adding attribute " << new_name << endl);
    }
#endif

    // If we want to map the attributes of the datatypes to those of netCDF-4, KY 2020-02-14
    if (is_nc_enhanced == true)
        //write_dap4_attrs_for_nc4_types(ncid, varid, var_name, new_attr_name, new_name, d4_attrs, attr, is_nc_enhanced);
        write_dap4_attrs_for_nc4_types(ncid, varid, var_name, new_attr_name, new_name, attr, is_nc_enhanced);
    else {
        int stax = NC_NOERR;
        string attr_type = "unknown";   // Used for error messages. jhrg 6/18/20
        unsigned int attri = 0;
        //unsigned int num_vals = attrs.get_attr_num(attr);
        unsigned int num_vals = attr->num_values();
        switch (d4_attr_type) {
            case attr_container_c: {
                // flatten
                BESDEBUG("fonc",
                         "Attribute " << d4_attr_name << " is an attribute container. new_attr_name: \""
                                      << new_attr_name
                                      << "\"" << endl);
                D4Attributes *c_attributes = attr->attributes();
                if (c_attributes) {
                    add_dap4_attributes(ncid, varid, c_attributes, var_name, new_attr_name, is_nc_enhanced);
                }

            }
                break;

            case attr_int8_c: {

                // 8-bit integer
                vector <int8_t> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    int uival = 0;
                    is >> uival;
                    vals[attri] = (int8_t) uival;
                    ++attri;
                }
                stax = nc_put_att_schar(ncid, varid, new_name.c_str(), NC_BYTE, num_vals, vals.data());
                if (stax != NC_NOERR) {
                    string err = (string) "File out netcdf-4 classic for DAP4,"
                            + " failed to write signed 8-bit integer attribute " + new_name;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
            }
                break;

            case attr_byte_c:
            case attr_uint8_c: {
                vector<short> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    // Follow the classic code, the stringstream doesn't work well with int8, 
                    // no overflow when casted back to vals.
                    unsigned int uival = 0;
                    is >> uival;
                    vals[attri] = (short) uival;
                    ++attri;
                }
                stax = nc_put_att_short(ncid, varid, new_name.c_str(), NC_SHORT, num_vals, vals.data());

            }
                break;

            case attr_int16_c: {
                // short
                vector<short> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    short sval = 0;
                    is >> sval;
                    vals[attri] = sval;
                    ++attri;
                }

                stax = nc_put_att_short(ncid, varid, new_name.c_str(), NC_SHORT, num_vals, vals.data());

                break;
            }

            case attr_uint16_c: {
                // unsigned short
                // (needs to be big enough to store an unsigned short
                attri = 0;
                //int vals[num_vals];
                vector<int> vals;
                vals.resize(num_vals);
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    int ival = 0;
                    is >> ival;
                    vals[attri] = ival;
                    ++attri;
                }

                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals, vals.data());

                break;
            }

            case attr_int32_c: {
                // int
                vector<int> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    int sval = 0;
                    is >> sval;
                    vals[attri] = sval;
                    ++attri;
                }

                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals, vals.data());

                break;
            }

            case attr_uint32_c: {
                // uint
                // needs to be big enough to store an unsigned int
                vector<int> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    istringstream is(val);
                    int sval = 0;
                    is >> sval;
                    vals[attri] = sval;
                    ++attri;
                }

                stax = nc_put_att_int(ncid, varid, new_name.c_str(), NC_INT, num_vals, vals.data());

                break;
            }

            case attr_float32_c: {
                // float
                vector<float> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    const char *cval = val.c_str();
                    //istringstream is(val);
                    float sval = 0;
                    sval = strtod(cval, NULL);
                    //is >> sval;
                    vals[attri] = sval;
                    ++attri;
                }

                stax = nc_put_att_float(ncid, varid, new_name.c_str(), NC_FLOAT, num_vals, vals.data());

                break;
            }

            case attr_float64_c: {
                // double
                vector<double> vals;
                vals.resize(num_vals);
                attri = 0;
                for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                    string val = *vi;
                    const char *cval = val.c_str();
                    //istringstream is(val);
                    double sval = 0;
                    sval = strtod(cval, NULL);
                    //is >> sval;
                    vals[attri] = sval;
                    ++attri;
                }
                stax = nc_put_att_double(ncid, varid, new_name.c_str(), NC_DOUBLE, num_vals, vals.data());

                break;
            }

            case attr_str_c:
            case attr_url_c:
            case attr_otherxml_c: {    // Added. jhrg 12.27.2011
                attr_type = "string";
                D4Attribute::D4AttributeIter vi, ve;
                vi = attr->value_begin();
                ve = attr->value_end();

                string val = (*vi);

                vi++;
                for (; vi != ve; vi++) {
                    val += "\n" + *vi;
                }

                if (d4_attr_name != "_FillValue") {
                    stax = nc_put_att_text(ncid, varid, new_name.c_str(), val.size(), val.c_str());
                }
                else {
                    BESDEBUG("fonc",
                             "FONcAttributes::add_attributes_worker - Original attribute value is first character: "
                                     << val.c_str()[0] << endl);
                    stax = nc_put_att_text(ncid, varid, new_name.c_str(), 1, val.c_str());
                    if (stax == NC_NOERR) {
                        // New name for attribute _FillValue with original value
                        string new_name_fillvalue = "Orig_FillValue";
                        BESDEBUG("fonc",
                                 "FONcAttributes::add_attributes_worker - New attribute value is original value: "
                                         << val.c_str() << endl);
                        // This line causes the segmentation fault since attrs is changed and the original iterator of attrs doesn't exist anymore.
                        // So it causes the segmentation fault when next attribute is fetched in the for loop of the add_attributes(). KY 2019-12-13
#if 0
                        attrs.append_attr(new_name_fillvalue,"String", val);
#endif
                        stax = nc_put_att_text(ncid, varid, new_name_fillvalue.c_str(), val.size(), val.c_str());
                    }
                }

                break;
            }

            case attr_null_c:
            case attr_int64_c:
            case attr_uint64_c:
            case attr_enum_c:
            case attr_opaque_c:
            default: {
                // Temporarily don't support these types.TODO: add the support.
                string err =
                        (string) "File out netcdf, failed to write unknown/unsupported type of attribute " + new_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                break;
            }
        }

        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, failed to write " + attr_type + " attribute " + new_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
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
                                   orig.size(), orig.c_str());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write change of name attribute for "
                         + var_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
}

/** @brief writes out a single attribute that maps the datatype to netCDF-4
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param var_name the name of the variable (flattened)
 * @param global_attr_name the name of the global attribute name
 * @param var_attr_name the name of the variable name
 * @param attrs the AttrTable that contains the attribute to be written
 * @param attr the iterator into the AttrTable for the attribute to be written
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing this attribute
 * Note: This function only maps DAP2 attribute types to NC4.
 */
void
FONcAttributes::write_attrs_for_nc4_types(int ncid, int varid, const string &var_name, const string &global_attr_name,
                                          const string &var_attr_name, AttrTable attrs, AttrTable::Attr_iter &attr,
                                          bool is_nc_enhanced) {

    int stax = NC_NOERR;
    string attr_type = "unknown"; // Used for error messages. jhrg 6/18/20
    AttrType attrType = attrs.get_attr_type(attr);
    BESDEBUG("fonc", "FONcAttributes write_attrs_for_nc4_type name:  " << var_attr_name << endl);
    BESDEBUG("fonc", "FONcAttributes write_attrs_for_nc4_type type:  " << attrType << endl);
    unsigned int attri = 0;
    unsigned int num_vals = attrs.get_attr_num(attr);
    switch (attrType) {
        case Attr_container: {
            // flatten
            BESDEBUG("fonc", "This is an attribute container. attr_name: \"" << global_attr_name << "\"" << endl);
            AttrTable *container = attrs.get_attr_table(attr);
            if (container) {
                add_attributes(ncid, varid, *container, var_name, global_attr_name, is_nc_enhanced);
            }
            break;
        }

        case Attr_byte: {
            // unsigned char
            vector<unsigned char> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                istringstream is(val);
                unsigned int uival = 0;
                is >> uival;
                vals[attri] = (unsigned char) uival;
            }
            stax = nc_put_att_uchar(ncid, varid, var_attr_name.c_str(), NC_UBYTE, num_vals, vals.data());
#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write byte attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif
            break;
        }

        case Attr_int16: {
            // short
            vector<short> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                istringstream is(val);
                short sval = 0;
                is >> sval;
                vals[attri] = sval;
            }
            stax = nc_put_att_short(ncid, varid, var_attr_name.c_str(), NC_SHORT, num_vals, vals.data());
#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write short attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_uint16: {
            // unsigned short
            // (needs to be big enough to store an unsigned short
            //unsigned short vals[num_vals];
            vector<unsigned short> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                istringstream is(val);
                unsigned short ival = 0;
                is >> ival;
                vals[attri] = ival;
            }
            stax = nc_put_att_ushort(ncid, varid, var_attr_name.c_str(), NC_USHORT, num_vals, vals.data());
#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write unsigned short attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_int32: {
            // int
            //int vals[num_vals];
            vector<int> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                istringstream is(val);
                int ival = 0;
                is >> ival;
                vals[attri] = ival;
            }
            stax = nc_put_att_int(ncid, varid, var_attr_name.c_str(), NC_INT, num_vals, vals.data());

#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write int attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_uint32: {
            // uint
            //unsigned int vals[num_vals];
            vector<unsigned int> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                istringstream is(val);
                unsigned int lval = 0;
                is >> lval;
                vals[attri] = lval;
            }
            stax = nc_put_att_uint(ncid, varid, var_attr_name.c_str(), NC_UINT, num_vals, vals.data());
#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write byte attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_float32: {
            // float
            //float vals[num_vals];
            vector<float> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                const char *cval = val.c_str();
                //istringstream is(val);
                float fval = 0;
                fval = strtod(cval, NULL);
                //is >> fval;
                vals[attri] = fval;
            }
            stax = nc_put_att_float(ncid, varid, var_attr_name.c_str(), NC_FLOAT, num_vals, vals.data());

#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, " + "failed to write float attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_float64: {
            // double
            //double vals[num_vals];
            vector<double> vals;
            vals.resize(num_vals);
            for (attri = 0; attri < num_vals; attri++) {
                string val = attrs.get_attr(attr, attri);
                const char *cval = val.c_str();
                //istringstream is(val);
                double dval = 0;
                dval = strtod(cval, NULL);
                //is >> dval;
                vals[attri] = dval;
            }
            stax = nc_put_att_double(ncid, varid, var_attr_name.c_str(), NC_DOUBLE, num_vals, vals.data());

#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, "  + "failed to write double attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }

        case Attr_string:
        case Attr_url:
        case Attr_other_xml: {    // Added. jhrg 12.27.2011
            // string
            string val = attrs.get_attr(attr, 0);
            for (attri = 1; attri < num_vals; attri++) {
                val += "\n" + attrs.get_attr(attr, attri);
            }
            string attr_name = attrs.get_name(attr);
            if (attr_name != "_FillValue") {
                stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), val.size(), val.c_str());
            }
            else {
                BESDEBUG("fonc",
                         "FONcAttributes::add_attributes_worker - Original attribute value is first character: "
                                 << val.c_str()[0] << endl);
                stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), 1, val.c_str());
                if (stax == NC_NOERR) {
                    // New name for attribute _FillValue with original value
                    string var_attr_name_fillvalue = "Orig_FillValue";
                    BESDEBUG("fonc",
                             "FONcAttributes::add_attributes_worker - New attribute value is original value: "
                                     << val.c_str() << endl);
                    // This line causes the segmentation fault since attrs is changed and the original iterator of attrs doesn't exist anymore.
                    // So it causes the segmentation fault when next attribute is fetched in the for loop of the add_attributes(). KY 2019-12-13
                    // Note: Leave the following #if 0 #endif for the time being. 
#if 0
                    attrs.append_attr(var_attr_name_fillvalue,"String", val);
#endif
                    stax = nc_put_att_text(ncid, varid, var_attr_name_fillvalue.c_str(), val.size(), val.c_str());
                }
            }

#if !NDEBUG
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf, "
                             + "failed to write string attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
#endif

            break;
        }


        case Attr_unknown:
        default: {
            string err = (string) "File out netcdf, failed to write unknown type of attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            break;  // Not actually needed since FONcUtils::handle_error throws
        }
    }

    if (stax != NC_NOERR) {
        string err = (string) "File out netcdf, failed to write " + attr_type + " attribute " + var_attr_name;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }
}

/** @brief writes out a single attribute that maps the dap4 datatype to netCDF-4
 *
 * @param ncid The id of the netcdf file being written to
 * @param varid The netcdf variable id
 * @param var_name the name of the variable (flattened)
 * @param global_attr_name the name of the global attribute name
 * @param var_attr_name the name of the variable name
 * @param attr the DAP4 attribute to be written
 * @param is_nc_enhanced The flag to indicate if we want to map datatypes to netCDF-4
 * @throws BESInternalError if there is a problem writing this attribute
 * Note: the DAP4 attributes are mapped to NC4. Now only 64-bit integer are added.
 */
void
FONcAttributes::write_dap4_attrs_for_nc4_types(int ncid,
                                               int varid,
                                               const string &var_name,
                                               const string &global_attr_name,
                                               const string &var_attr_name,
                                               D4Attribute *attr,
                                               bool is_nc_enhanced) {

    D4AttributeType d4_attr_type = attr->type();
    int stax = NC_NOERR;
    unsigned int attri = 0;
    unsigned int num_vals = attr->num_values();
    switch (d4_attr_type) {
        case attr_container_c: {
            // flatten
            BESDEBUG("fonc", "This is an attribute container. attr_name: \"" << global_attr_name << "\"" << endl);
            D4Attributes *c_attributes = attr->attributes();
            if (c_attributes) {
                add_dap4_attributes(ncid, varid, c_attributes, var_name, global_attr_name, is_nc_enhanced);
            }
        }
            break;
        case attr_byte_c:
        case attr_uint8_c: {
            // unsigned char
            vector<unsigned char> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                unsigned int uival = 0;
                is >> uival;
                vals[attri] = (unsigned char) uival;
                ++attri;
            }
            stax = nc_put_att_uchar(ncid, varid, var_attr_name.c_str(), NC_UBYTE,
                                    num_vals, vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write byte attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_int8_c: {
            // 8-bit integer
            vector <int8_t> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                int uival = 0;
                is >> uival;
                vals[attri] = (int8_t) uival;
                ++attri;
            }
            stax = nc_put_att_schar(ncid, varid, var_attr_name.c_str(), NC_BYTE,
                                    num_vals, vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write signed 8-bit integer attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_int16_c: {
            // short
            vector<short> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                short sval = 0;
                is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_short(ncid, varid, var_attr_name.c_str(), NC_SHORT,
                                    num_vals, vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write short attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_uint16_c: {
            // unsigned short
            attri = 0;
            vector<unsigned short> vals;
            vals.resize(num_vals);
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                unsigned short ival = 0;
                is >> ival;
                vals[attri] = ival;
                ++attri;
            }

            stax = nc_put_att_ushort(ncid, varid, var_attr_name.c_str(), NC_USHORT, num_vals,
                                     vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write unsigned short attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_int32_c: {
            vector<int> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                int sval = 0;
                is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_int(ncid, varid, var_attr_name.c_str(), NC_INT, num_vals,
                                  vals.data());

            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write int attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_uint32_c: {
            // uint
            vector<unsigned int> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                unsigned int sval = 0;
                is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_uint(ncid, varid, var_attr_name.c_str(), NC_UINT, num_vals,
                                   vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write unsigned int attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_int64_c: {
            vector<long long> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                long long sval = 0;
                is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_longlong(ncid, varid, var_attr_name.c_str(), NC_INT64, num_vals,
                                       vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write 64-bit int attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_uint64_c: {
            vector<unsigned long long> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                istringstream is(val);
                unsigned long long sval = 0;
                is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_ulonglong(ncid, varid, var_attr_name.c_str(), NC_UINT64, num_vals,
                                        vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write unsigned 64-bit int attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;

        case attr_float32_c: {
            vector<float> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                const char *cval = val.c_str();
                //istringstream is(val);
                float sval = 0;
                sval = strtod(cval, NULL);
                //is >> sval;
                vals[attri] = sval;
                ++attri;
            }

            stax = nc_put_att_float(ncid, varid, var_attr_name.c_str(), NC_FLOAT,
                                    num_vals, vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write float attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_float64_c: {
            vector<double> vals;
            vals.resize(num_vals);
            attri = 0;
            for (D4Attribute::D4AttributeIter vi = attr->value_begin(), ve = attr->value_end(); vi != ve; vi++) {
                string val = *vi;
                const char *cval = val.c_str();
                //istringstream is(val);
                double sval = 0;
                sval = strtod(cval, NULL);
                //is >> sval;
                vals[attri] = sval;
                ++attri;
            }
            stax = nc_put_att_double(ncid, varid, var_attr_name.c_str(), NC_DOUBLE,
                                     num_vals, vals.data());
            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write double attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;
        case attr_str_c:
        case attr_url_c:
        case attr_otherxml_c:    // Added. jhrg 12.27.2011
        {

            if (attr->num_values() == 0)
                stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), 0, "");
            else {

                D4Attribute::D4AttributeIter vi, ve;
                vi = attr->value_begin();
                ve = attr->value_end();


                string val = (*vi);

                vi++;
                for (; vi != ve; vi++) {
                    val += "\n" + *vi;
                }

                if (var_attr_name != "_FillValue") {
                    stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), val.size(), val.c_str());
                }
                else {
                    BESDEBUG("fonc",
                             "FONcAttributes::add_attributes_worker - Original attribute value is first character: "
                                     << val.c_str()[0] << endl);
                    stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), 1, val.c_str());
                    if (stax == NC_NOERR) {
                        // New name for attribute _FillValue with original value
                        string var_attr_name_fillvalue = "Orig_FillValue";
                        BESDEBUG("fonc",
                                 "FONcAttributes::add_attributes_worker - New attribute value is original value: "
                                         << val.c_str() << endl);
                        // This line causes the segmentation fault since attrs is changed and the original iterator of attrs doesn't exist anymore.
                        // So it causes the segmentation fault when next attribute is fetched in the for loop of the add_attributes(). KY 2019-12-13
#if 0
                        attrs.append_attr(var_attr_name_fillvalue,"String", val);
#endif
                        stax = nc_put_att_text(ncid, varid, var_attr_name_fillvalue.c_str(), val.size(), val.c_str());
                    }
                }
            }

            if (stax != NC_NOERR) {
                string err = (string) "File out netcdf-4 enhanced for DAP4, "
                             + "failed to write string attribute " + var_attr_name;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
            break;

        case attr_null_c:
        case attr_enum_c:
        case attr_opaque_c: {
            string err = (string) "File out netcdf, "
                         + "failed to write unknown type of attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
            break;
    }
}

// Note: Leave the following #if 0 #endif block for the time being. They can be removed in the future
// release. KY 2021-06-15
#if 0
int stax = NC_NOERR;
AttrType attrType = attrs.get_attr_type(attr);
unsigned int attri = 0;
unsigned int num_vals = attrs.get_attr_num(attr);
switch (attrType) {
    case Attr_container: {
        // flatten
        BESDEBUG("fonc", "This is an attribute container. attr_name: \"" << global_attr_name << "\"" << endl);
        AttrTable *container = attrs.get_attr_table(attr);
        if (container) {
            add_attributes(ncid, varid, *container, var_name, global_attr_name, is_nc_enhanced);
        }
    }
        break;
    case Attr_byte: {
        // unsigned char
        //unsigned char vals[num_vals];
        vector<unsigned char> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            unsigned int uival = 0;
            is >> uival;
            vals[attri] = (unsigned char) uival;
        }
        stax = nc_put_att_uchar(ncid, varid, var_attr_name.c_str(), NC_UBYTE,
                                num_vals, vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write byte attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_int16: {
        // short
        //short vals[num_vals];
        vector<short> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            short sval = 0;
            is >> sval;
            vals[attri] = sval;
        }
        stax = nc_put_att_short(ncid, varid, var_attr_name.c_str(), NC_SHORT,
                                num_vals, vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write short attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_uint16: {
        // unsigned short
        // (needs to be big enough to store an unsigned short
        //unsigned short vals[num_vals];
        vector<unsigned short> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            unsigned short ival = 0;
            is >> ival;
            vals[attri] = ival;
        }
        stax = nc_put_att_ushort(ncid, varid, var_attr_name.c_str(), NC_USHORT, num_vals,
                                 vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write unsinged short attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_int32: {
        // int
        //int vals[num_vals];
        vector<int> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            int ival = 0;
            is >> ival;
            vals[attri] = ival;
        }
        stax = nc_put_att_int(ncid, varid, var_attr_name.c_str(), NC_INT, num_vals,
                              vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write int attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_uint32: {
        // uint
        //unsigned int vals[num_vals];
        vector<unsigned int> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            istringstream is(val);
            unsigned int lval = 0;
            is >> lval;
            vals[attri] = lval;
        }
        stax = nc_put_att_uint(ncid, varid, var_attr_name.c_str(), NC_UINT, num_vals,
                               vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write byte attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_float32: {
        // float
        //float vals[num_vals];
        vector<float> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            const char *cval = val.c_str();
            //istringstream is(val);
            float fval = 0;
            fval = strtod(cval,NULL);
            //is >> fval;
            vals[attri] = fval;
        }
        stax = nc_put_att_float(ncid, varid, var_attr_name.c_str(), NC_FLOAT,
                                num_vals, vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write float attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;
    case Attr_float64: {
        // double
        //double vals[num_vals];
        vector<double> vals;
        vals.resize(num_vals);
        for (attri = 0; attri < num_vals; attri++) {
            string val = attrs.get_attr(attr, attri);
            const char *cval = val.c_str();
            //istringstream is(val);
            double dval = 0;
            dval = strtod(cval,NULL);
            //is >> dval;
            vals[attri] = dval;
        }
        stax = nc_put_att_double(ncid, varid, var_attr_name.c_str(), NC_DOUBLE,
                                 num_vals, vals.data());
        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write double attribute " + var_attr_name;
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
        string attr_name = attrs.get_name(attr);
        if (attr_name != "_FillValue") {
            stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), val.size(), val.c_str());
        } else {
            BESDEBUG("fonc",
                     "FONcAttributes::add_attributes_worker - Original attribute value is first character: "
                             << val.c_str()[0] << endl);
            stax = nc_put_att_text(ncid, varid, var_attr_name.c_str(), 1, val.c_str());
            if (stax == NC_NOERR) {
                // New name for attribute _FillValue with original value
                string var_attr_name_fillvalue = "Orig_FillValue";
                BESDEBUG("fonc",
                         "FONcAttributes::add_attributes_worker - New attribute value is original value: "
                                 << val.c_str() << endl);
                // This line causes the segmentation fault since attrs is changed and the original iterator of attrs doesn't exist anymore.
                // So it causes the segmentation fault when next attribute is fetched in the for loop of the add_attributes(). KY 2019-12-13
#if 0
                attrs.append_attr(var_attr_name_fillvalue,"String", val);
#endif
                stax = nc_put_att_text(ncid, varid, var_attr_name_fillvalue.c_str(), val.size(), val.c_str());
            }
        }

        if (stax != NC_NOERR) {
            string err = (string) "File out netcdf, "
                         + "failed to write string attribute " + var_attr_name;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
        break;

    case Attr_unknown: {
        string err = (string) "File out netcdf, "
                     + "failed to write unknown type of attribute " + var_attr_name;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }
        break;
}

#endif

