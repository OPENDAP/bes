// -*- mode: c++; c-basic-offset:4 -*-

// This file is part of nc_handler, a data handler for the OPeNDAP data
// server.

// Copyright (c) 2002,2003 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
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

// (c) COPYRIGHT URI/MIT 1994-1996
// Please read the full copyright statement in the file COPYRIGHT.
//
// Authors:
//      reza            Reza Nekovei (reza@intcomm.net)

// This file contains functions which read the variables and their attributes
// from a netcdf file and build the in-memeory DAS. These functions form the
// core of the server-side software necessary to extract the DAS from a
// netcdf data file.
//
// It also contains test code which will print the in-memory DAS to
// stdout. It uses both the DAS class as well as the netcdf library.
// In addition, parts of these functions were taken from the netcdf program
// ncdump, from the netcdf standard distribution (ver 2.3.2)
//
// jhrg 9/23/94

#include "config_nc.h"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

#include <netcdf.h>

#include <util.h>
#include <escaping.h>
#include <DAS.h>

#include <BESDebug.h>

#include "NCRequestHandler.h"
#include "nc_util.h"

#define ATTR_STRING_QUOTE_FIX

#if NETCDF_VERSION >= 4
#define READ_ATTRIBUTES_MACRO read_attributes_netcdf4
#else
#define READ_ATTRIBUTES_MACRO read_attributes_netcdf3
#endif

using namespace libdap;

/** Given the type, array index number and pointer to the associated attribute,
 Return the string representation of the attribute's value.

 This function is modeled on code from ncdump. I modified the original
 version to use C++ string streams and also to call escattr() so that
 attributes with quotes would be handled correctly.

 @param type The nc_type of this attribute
 @param loc The offset within \e vals
 @param vals a void* to the array of values */
static string print_attr(nc_type type, int loc, void *vals)
{
    ostringstream rep;
    union {
        char *cp;
        char **stringp;
        short *sp;
        unsigned short *usp;
        int *i;
        unsigned int *ui;
        float *fp;
        double *dp;
    } gp;

    switch (type) {
#if NETCDF_VERSION >= 4
    case NC_UBYTE:
    unsigned char uc;
    gp.cp = (char *) vals;

    uc = *(gp.cp + loc);
    rep << (int) uc;
    return rep.str();
#endif

    case NC_BYTE:
        if (NCRequestHandler::get_promote_byte_to_short()) {
            signed char sc;
            gp.cp = (char *) vals;

            sc = *(gp.cp + loc);
            rep << (int) sc;
            return rep.str();
        }
        else {
            unsigned char uc;
            gp.cp = (char *) vals;

            uc = *(gp.cp + loc);
            rep << (int) uc;
            return rep.str();
        }

    case NC_CHAR:
#ifndef ATTR_STRING_QUOTE_FIX
        rep << "\"" << escattr(static_cast<const char*>(vals)) << "\"";
        return rep.str();
#else
        return escattr(static_cast<const char*>(vals));
#endif

#if NETCDF_VERSION >= 4
        case NC_STRING:
        gp.stringp = (char **) vals;
        rep << *(gp.stringp + loc);
        return rep.str();
#endif

    case NC_SHORT:
        gp.sp = (short *) vals;
        rep << *(gp.sp + loc);
        return rep.str();

#if NETCDF_VERSION >= 4
        case NC_USHORT:
        gp.usp = (unsigned short *) vals;
        rep << *(gp.usp + loc);
        return rep.str();
#endif

    case NC_INT:
        gp.i = (int *) vals; // warning: long int format, int arg (arg 3)
        rep << *(gp.i + loc);
        return rep.str();

#if NETCDF_VERSION >= 4
        case NC_UINT:
        gp.ui = (unsigned int *) vals;
        rep << *(gp.ui + loc);
        return rep.str();
#endif

    case NC_FLOAT: {
        gp.fp = (float *) vals;
        float valAtLoc = *(gp.fp + loc);

        rep << std::showpoint;
        rep << std::setprecision(9);

        if(isnan(valAtLoc)){
            rep << "NaN";
        }
        else {
            rep << valAtLoc;
        }
        // If there's no decimal point and the rep does not use scientific
        // notation, add a decimal point. This little jaunt was taken because
        // this code is modeled after older code and that's what it did. I'm
        // trying to keep the same behavior as the old code without it's
        // problems. jhrg 8/11/2006
        string tmp_value = rep.str();
        if (tmp_value.find('.') == string::npos && tmp_value.find('e') == string::npos
            && tmp_value.find('E') == string::npos && tmp_value.find("nan") == string::npos
            && tmp_value.find("NaN") == string::npos && tmp_value.find("NAN") == string::npos) rep << ".";
        return rep.str();
    }

    case NC_DOUBLE: {
        gp.dp = (double *) vals;
        double valAtLoc = *(gp.dp + loc);

        rep << std::showpoint;
        rep << std::setprecision(16);


        if(isnan(valAtLoc)){
            rep << "NaN";
        }
        else {
            rep << valAtLoc;
        }
        string tmp_value = rep.str();
        if (tmp_value.find('.') == string::npos && tmp_value.find('e') == string::npos
            && tmp_value.find('E') == string::npos && tmp_value.find("nan") == string::npos
            && tmp_value.find("NaN") == string::npos && tmp_value.find("NAN") == string::npos) rep << ".";
        return rep.str();
    }

    default:
        if (NCRequestHandler::get_ignore_unknown_types())
            cerr << "The netcdf handler tried to print an attribute that has an unrecognized type. (1)" << endl;
        else
            throw InternalErr(__FILE__, __LINE__,
                "The netcdf handler tried to print an attribute that has an unrecognized type. (1)");
        break;
    }

    return "";
}

/** Return the printed representation of a netcdf type -- in a form the
 handler can use.

 @param datatype A nc_type
 @return A string that holds the type name.
 */
static string print_type(nc_type datatype)
{
    switch (datatype) {
#if NETCDF_VERSION >= 4
    case NC_STRING:
#endif
    case NC_CHAR:
        return "String";

#if NETCDF_VERSION >= 4
        case NC_UBYTE:
        return "Byte";
#endif
    case NC_BYTE:
        if (NCRequestHandler::get_promote_byte_to_short()) {
            return "Int16";
        }
        else {
            return "Byte";
        }

    case NC_SHORT:
        return "Int16";

    case NC_INT:
        return "Int32";

#if NETCDF_VERSION >= 4
        case NC_USHORT:
        return "UInt16";

        case NC_UINT:
        return "UInt32";
#endif

    case NC_FLOAT:
        return "Float32";

    case NC_DOUBLE:
        return "Float64";

#if NETCDF_VERSION >= 4
        case NC_COMPOUND:
        return "NC_COMPOUND";
#endif

#if NETCDF_VERSION >= 4
        // These are all new netcdf 4 types that we don't support yet
        // as attributes. It's useful to have a print representation for
        // them so that we can return useful information about why some
        // information was elided or an exception thrown.
        case NC_INT64:
        return "NC_INT64";

        case NC_UINT64:
        return "NC_UINT64";

        case NC_VLEN:
        return "NC_VLEN";
        case NC_OPAQUE:
        return "NC_OPAQUE";
        case NC_ENUM:
        return "NC_ENUM";
#endif
    default:
        if (NCRequestHandler::get_ignore_unknown_types())
            cerr << "The netcdf handler tried to print an attribute that has an unrecognized type. (2)" << endl;
        else
            throw InternalErr(__FILE__, __LINE__,
                "The netcdf handler tried to print an attribute that has an unrecognized type. (2)");
        break;
    }

    return "";
}

/** Build and append values for attributes. The value are limited to
 * cardinal types; compound, ..., values are built by calling this once for
 * each of the subordinate types. Vectors of values are handled here, however,
 */
static void append_values(int ncid, int v, int len, nc_type datatype, char *attrname, AttrTable *at)
{
    size_t size;
    int errstat;
#if NETCDF_VERSION >= 4
    errstat = nc_inq_type(ncid, datatype, 0, &size);
    if (errstat != NC_NOERR)
    throw Error(errstat, "Could not get the size for the type.");
#else
    size = nctypelen(datatype);
#endif

    vector<char> value((len + 1) * size);
    errstat = nc_get_att(ncid, v, attrname, &value[0]);
    if (errstat != NC_NOERR) {
        throw Error(errstat, string("Could not get the value for attribute '") + attrname + string("'"));
    }

    // If the datatype is NC_CHAR then we have a string. netCDF 3
    // represents strings as arrays of char, but we represent them as X
    // strings. So... Add the null and set the length to 1
    if (datatype == NC_CHAR) {
        value[len] = '\0';
        len = 1;
    }

    // add all the attributes in the array
    for (int loc = 0; loc < len; loc++) {
        string print_rep = print_attr(datatype, loc, &value[0]);
        at->append_attr(attrname, print_type(datatype), print_rep);
    }
}

/** Given the netcdf file id, variable id, number of attributes for the
 variable, and an attribute table pointer, read the attributes and store
 their names and values in the attribute table.

 @note Attribute values in the DAP are stored only as strings.
 @param ncid The netcdf file id
 @param v The netcdf variable id
 @param natts The number of attributes
 @param at A value-result parameter; a point to the attribute table to which
 the new information will be added.
 */
static void read_attributes_netcdf3(int ncid, int v, int natts, AttrTable *at)
{
    char attrname[MAX_NC_NAME];
    nc_type datatype;
    size_t len;
    int errstat = NC_NOERR;

    for (int a = 0; a < natts; ++a) {
        errstat = nc_inq_attname(ncid, v, a, attrname);
        if (errstat != NC_NOERR) {
            string msg = "Could not get the name for attribute ";
            msg += long_to_string(a);
            throw Error(errstat, msg);
        }

        // len is the number of values. Attributes in netcdf can be scalars or
        // vectors
        errstat = nc_inq_att(ncid, v, attrname, &datatype, &len);
        if (errstat != NC_NOERR) {
            string msg = "Could not get the name for attribute '";
            msg += attrname + string("'");
            throw Error(errstat, msg);
        }

        switch (datatype) {
        case NC_BYTE:
        case NC_CHAR:
        case NC_SHORT:
        case NC_INT:
        case NC_FLOAT:
        case NC_DOUBLE:
            append_values(ncid, v, len, datatype, attrname, at);
            break;

        default:
            if (NCRequestHandler::get_ignore_unknown_types())
                cerr << "Unrecognized attribute type." << endl;
            else
                throw InternalErr(__FILE__, __LINE__, "Unrecognized attribute type.");
            break;
        }
    }
}

#if NETCDF_VERSION >= 4

/** Given the netcdf file id, variable id, number of attributes for the
 variable, and an attribute table pointer, read the attributes and store
 their names and values in the attribute table.

 @note Attribute values in the DAP are stored only as strings.
 @param ncid The netcdf file id
 @param v The netcdf variable id
 @param natts The number of attributes
 @param at A value-result parameter; a point to the attribute table to which
 the new information will be added.
 */
static void read_attributes_netcdf4(int ncid, int varid, int natts, AttrTable *at)
{
    BESDEBUG("nc", "In read_attributes_netcdf4" << endl);

    for (int attr_num = 0; attr_num < natts; ++attr_num) {
        int errstat = NC_NOERR;
        // Get the attribute name
        char attrname[MAX_NC_NAME];
        errstat = nc_inq_attname(ncid, varid, attr_num, attrname);
        if (errstat != NC_NOERR)
        throw Error(errstat, "Could not get the name for attribute " + long_to_string(attr_num));

        // Get datatype and len; len is the number of values.
        nc_type datatype;
        size_t len;
        errstat = nc_inq_att(ncid, varid, attrname, &datatype, &len);
        if (errstat != NC_NOERR)
        throw Error(errstat, "Could not get the name for attribute '" + string(attrname) + "'");

        BESDEBUG("nc", "nc_inq_att returned datatype = " << datatype << endl);

        if (is_user_defined_type(ncid, datatype)) {
            // datatype >= NC_FIRSTUSERTYPEID) {
            char type_name[NC_MAX_NAME+1];
            size_t size;
            nc_type base_type;
            size_t nfields;
            int class_type;
            errstat = nc_inq_user_type(ncid, datatype, type_name, &size, &base_type, &nfields, &class_type);
            if (errstat != NC_NOERR)
            throw(InternalErr(__FILE__, __LINE__, "Could not get information about a user-defined type (" + long_to_string(errstat) + ")."));

            BESDEBUG("nc", "Before switch(class_type)" << endl);
            switch (class_type) {
                case NC_COMPOUND: {
                    // Make recursive attrs work?
                    vector<unsigned char> values((len + 1) * size);

                    int errstat = nc_get_att(ncid, varid, attrname, &values[0]);
                    if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not get the value for attribute '") + attrname + string("'"));

                    for (size_t i = 0; i < nfields; ++i) {
                        char field_name[NC_MAX_NAME+1];
                        nc_type field_typeid;
                        size_t field_offset;
                        nc_inq_compound_field(ncid, datatype, i, field_name, &field_offset, &field_typeid, 0, 0);

                        at->append_attr(field_name, print_type(field_typeid), print_attr(field_typeid, 0, &values[0] + field_offset));
                    }
                    break;
                }

                case NC_VLEN:
                if (NCRequestHandler::get_ignore_unknown_types())
                cerr << "in build_user_defined; found a vlen." << endl;
                else
                throw Error("The netCDF handler does not yet support the NC_VLEN type.");
                break;

                case NC_OPAQUE: {
                    vector<unsigned char> values((len + 1) * size);

                    int errstat = nc_get_att(ncid, varid, attrname, &values[0]);
                    if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not get the value for attribute '") + attrname + string("'"));

                    for (size_t i = 0; i < size; ++i)
                    at->append_attr(attrname, print_type(NC_BYTE), print_attr(NC_BYTE, i, &values[0]));

                    break;
                }

                case NC_ENUM: {
                    vector<unsigned char> values((len + 1) * size);

                    int errstat = nc_get_att(ncid, varid, attrname, &values[0]);
                    if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not get the value for attribute '") + attrname + string("'"));

                    nc_type basetype;
                    errstat = nc_inq_enum(ncid, datatype, 0/*char *name*/, &basetype,
                        0/*size_t *base_sizep*/, 0/*size_t *num_membersp*/);
                    if (errstat != NC_NOERR)
                    throw Error(errstat, string("Could not get the size of the enum base type for '") + attrname + string("'"));

                    for (size_t i = 0; i < size; ++i)
                    at->append_attr(attrname, print_type(basetype), print_attr(basetype, i, &values[0]));

                    break;
                }

                default:
                throw InternalErr(__FILE__, __LINE__, "Expected one of NC_COMPOUND, NC_VLEN, NC_OPAQUE or NC_ENUM");
            }

            BESDEBUG("nc", "After switch(class-type)" << endl);
        }
        else {
            switch (datatype) {
                case NC_STRING:
                case NC_BYTE:
                case NC_CHAR:
                case NC_SHORT:
                case NC_INT:
                case NC_FLOAT:
                case NC_DOUBLE:
                case NC_UBYTE:
                case NC_USHORT:
                case NC_UINT:
                BESDEBUG("nc", "Before append_values ..." << endl);
                append_values(ncid, varid, len, datatype, attrname, at);
                BESDEBUG("nc", "After append_values ..." << endl);
                break;

                case NC_INT64:
                case NC_UINT64: {
                    string note = "Attribute edlided: Unsupported attribute type ";
                    note += "(" + print_type(datatype) + ")";
                    at->append_attr(attrname, "String", note);
                    break;
                }

                case NC_COMPOUND:
                case NC_VLEN:
                case NC_OPAQUE:
                case NC_ENUM:
                throw InternalErr(__FILE__, __LINE__, "user-defined attribute type not recognized as such!");

                default:
                throw InternalErr(__FILE__, __LINE__, "Unrecognized attribute type.");
            }
        }
    }
    BESDEBUG("nc", "Exiting read_attributes_netcdf4" << endl);
}
#endif

/** Given a reference to an instance of class DAS and a filename that refers
 to a netcdf file, read the netcdf file and extract all the attributes of
 each of its variables. Add the variables and their attributes to the
 instance of DAS.

 @param das A reference to the DAS object where the attribute information
 should be stored.
 @param filename The name of the source file.
 */
void nc_read_dataset_attributes(DAS &das, const string &filename)
{
    BESDEBUG("nc", "In nc_read_dataset_attributes" << endl);

    int ncid, errstat;
    errstat = nc_open(filename.c_str(), NC_NOWRITE, &ncid);
    if (errstat != NC_NOERR) throw Error(errstat, "NetCDF handler: Could not open " + path_to_filename(filename) + ".");

    // how many variables? how many global attributes?
    int nvars, ngatts;
    errstat = nc_inq(ncid, (int *) 0, &nvars, &ngatts, (int *) 0);
    if (errstat != NC_NOERR)
        throw Error(errstat, "NetCDF handler: Could not inquire about netcdf file: " + path_to_filename(filename) + ".");

    // for each variable
    char varname[MAX_NC_NAME];
    int natts = 0;
    nc_type var_type;
    for (int varid = 0; varid < nvars; ++varid) {
        BESDEBUG("nc", "Top of for loop; for each var..." << endl);

        errstat = nc_inq_var(ncid, varid, varname, &var_type, (int*) 0, (int*) 0, &natts);
        if (errstat != NC_NOERR)
            throw Error(errstat, "Could not get information for variable: " + long_to_string(varid));

        AttrTable *attr_table_ptr = das.get_table(varname);
        if (!attr_table_ptr) attr_table_ptr = das.add_table(varname, new AttrTable);

        READ_ATTRIBUTES_MACRO(ncid, varid, natts, attr_table_ptr);

        // Add a special attribute for string lengths
        if (var_type == NC_CHAR) {
            // number of dimensions and size of Nth dimension
            int num_dim;
            int vdimids[MAX_VAR_DIMS]; // variable dimension ids
            errstat = nc_inq_var(ncid, varid, (char *) 0, (nc_type *) 0, &num_dim, vdimids, (int *) 0);
            if (errstat != NC_NOERR)
                throw Error(errstat,
                    string("NetCDF handler: Could not read information about a NC_CHAR variable while building the DAS."));

            if (num_dim == 0) {
                // a scalar NC_CHAR is stuffed into a string of length 1
                int size = 1;
                string print_rep = print_attr(NC_INT, 0, (void *) &size);
                attr_table_ptr->append_attr("string_length", print_type(NC_INT), print_rep);
            }
            else {
                // size_t *dim_sizes = new size_t[num_dim];
                vector<size_t> dim_sizes(num_dim);
                for (int i = 0; i < num_dim; ++i) {
                    if ((errstat = nc_inq_dimlen(ncid, vdimids[i], &dim_sizes[i])) != NC_NOERR) {
                        throw Error(errstat,
                            string("NetCDF handler: Could not read dimension information about the variable `") + varname
                                + string("'."));
                    }
                }

                // add attribute
                string print_rep = print_attr(NC_INT, 0, (void *) (&dim_sizes[num_dim - 1]));
                attr_table_ptr->append_attr("string_length", print_type(NC_INT), print_rep);
            }
        }

#if NETCDF_VERSION >= 4
        else if (is_user_defined_type(ncid, var_type)) {
            //var_type >= NC_FIRSTUSERTYPEID) {
            vector<char> name(MAX_NC_NAME + 1);
            int class_type;
            errstat = nc_inq_user_type(ncid, var_type, &name[0], 0, 0, 0, &class_type);
            if (errstat != NC_NOERR)
            throw(InternalErr(__FILE__, __LINE__, "Could not get information about a user-defined type (" + long_to_string(errstat) + ")."));

            switch (class_type) {
                case NC_OPAQUE: {
                    attr_table_ptr->append_attr("DAP2_OriginalNetCDFBaseType", print_type(NC_STRING), "NC_OPAQUE");
                    attr_table_ptr->append_attr("DAP2_OriginalNetCDFTypeName", print_type(NC_STRING), &name[0]);
                    break;
                }

                case NC_ENUM: {
                    //vector<char> name(MAX_NC_NAME + 1);
                    nc_type base_nc_type;
                    size_t base_size, num_members;
                    errstat = nc_inq_enum(ncid, var_type, 0/*&name[0]*/, &base_nc_type, &base_size, &num_members);
                    if (errstat != NC_NOERR)
                    throw(InternalErr(__FILE__, __LINE__, "Could not get information about an enum(" + long_to_string(errstat) + ")."));

                    // If the base type is a 64-bit int, bail with an error or
                    // a message about unsupported types
                    if (base_nc_type == NC_INT64 || base_nc_type == NC_UINT64) {
                        if (NCRequestHandler::get_ignore_unknown_types())
                        cerr << "An Enum uses 64-bit integers, but this handler does not support that type." << endl;
                        else
                        throw Error("An Enum uses 64-bit integers, but this handler does not support that type.");
                        break;
                    }

                    for (size_t i = 0; i < num_members; ++i) {
                        vector<char> member_name(MAX_NC_NAME + 1);
                        vector<char> member_value(base_size);
                        errstat = nc_inq_enum_member(ncid, var_type, i, &member_name[0], &member_value[0]);
                        if (errstat != NC_NOERR)
                        throw(InternalErr(__FILE__, __LINE__, "Could not get information about an enum value (" + long_to_string(errstat) + ")."));
                        attr_table_ptr->append_attr("DAP2_EnumValues", print_type(base_nc_type), print_attr(base_nc_type, 0, &member_value[0]));
                        attr_table_ptr->append_attr("DAP2_EnumNames", print_type(NC_STRING), &member_name[0]);
                    }

                    attr_table_ptr->append_attr("DAP2_OriginalNetCDFBaseType", print_type(NC_STRING), "NC_ENUM");
                    attr_table_ptr->append_attr("DAP2_OriginalNetCDFTypeName", print_type(NC_STRING), &name[0]);

                    break;
                }

                default:
                break;
            }
        }
#endif // NETCDF_VERSION >= 4
    }

    BESDEBUG("nc", "Starting global attributes" << endl);

    // global attributes
    if (ngatts > 0) {
        AttrTable *attr_table_ptr = das.add_table("NC_GLOBAL", new AttrTable);
        READ_ATTRIBUTES_MACRO(ncid, NC_GLOBAL, ngatts, attr_table_ptr);
    }

    // Add unlimited dimension name in DODS_EXTRA attribute table
    int xdimid;
    char dimname[MAX_NC_NAME];
    nc_type datatype = NC_CHAR;
    if ((errstat = nc_inq(ncid, (int *) 0, (int *) 0, (int *) 0, &xdimid)) != NC_NOERR)
        throw InternalErr(__FILE__, __LINE__, string("NetCDF handler: Could not access variable information: ") + nc_strerror(errstat));
    if (xdimid != -1) {
        if ((errstat = nc_inq_dim(ncid, xdimid, dimname, (size_t *) 0)) != NC_NOERR)
            throw InternalErr(__FILE__, __LINE__, string("NetCDF handler: Could not access dimension information: ") + nc_strerror(errstat));
        string print_rep = print_attr(datatype, 0, dimname);
        AttrTable *attr_table_ptr = das.add_table("DODS_EXTRA", new AttrTable);
        attr_table_ptr->append_attr("Unlimited_Dimension", print_type(datatype), print_rep);
    }

    if (nc_close(ncid) != NC_NOERR) throw InternalErr(__FILE__, __LINE__, "NetCDF handler: Could not close the dataset!");

    BESDEBUG("nc", "Exiting nc_read_dataset_attributes" << endl);
}
