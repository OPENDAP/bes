// FONcUtils.cc

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
//      kyang       Kent Yang <myang6@hdfgroup.org> (for netCDF-4 enhancement)

#include "config.h"

#include <cassert>

#include "FONcUtils.h"
#include "FONcDim.h"
#include "FONcByte.h"
#include "FONcInt8.h"
#include "FONcUByte.h"
#include "FONcStr.h"
#include "FONcShort.h"
#include "FONcUShort.h"
#include "FONcInt.h"
#include "FONcUInt.h"
#include "FONcInt64.h"
#include "FONcUInt64.h"
#include "FONcFloat.h"
#include "FONcDouble.h"
#include "FONcStructure.h"
#include "FONcGrid.h"
#include "FONcArray.h"
#include "FONcSequence.h"

#include <BESInternalError.h>
#include <BESDebug.h>
#include <D4Dimensions.h>

/** @brief If a variable name, dimension name, or attribute name begins
 * with a character that is not supported by netcdf, then use this
 * prefix to prepend to the name.
 */
string FONcUtils::name_prefix = "";

/** @brief Resets the FONc transformation for a new input and out file
 */
void FONcUtils::reset()
{
    FONcArray::Dimensions.clear();
    FONcGrid::Maps.clear();
    FONcDim::DimNameNum = 0;
}

/** @brief convert the provided string to a netcdf allowed
 * identifier.
 *
 * The function makes a copy of the incoming parameter to use and
 * returns the new string.
 *
 * @param in identifier to convert
 * @returns new netcdf compliant identifier
 */
string FONcUtils::id2netcdf(string in)
{
    // string of allowed characters in netcdf naming convention
    string allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-+_.@";
    // string of allowed first characters in netcdf naming
    // convention
    string first = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

    string::size_type i = 0;

    while ((i = in.find_first_not_of(allowed, i)) != string::npos) {
        in.replace(i, 1, "_");
        i++;
    }

    if (first.find(in[0]) == string::npos) {
        in = FONcUtils::name_prefix + in;
    }

    return in;
}

/** @brief translate the OPeNDAP data type to a netcdf data type
 *
 * @param element The OPeNDAP element to translate
 * @param IsNC4_ENHANCED the flag to indicate the output is in netCDF enhanced model
 * @return the netcdf data type
 */
nc_type FONcUtils::get_nc_type(BaseType *element,bool IsNC4_ENHANCED)
{
    nc_type x_type = NC_NAT; // the constant ncdf uses to define simple type

    string var_type = element->type_name();
    BESDEBUG("fonc", "FONcUtils() - var_type "<< var_type <<endl);
    BESDEBUG("fonc", "FONcUtils() - var_name "<< element->name() <<endl);
    if (var_type == "Byte" || var_type == "UInt8") { // check this for dods type
        if(IsNC4_ENHANCED) 
            x_type = NC_UBYTE;
        else 
            x_type = NC_SHORT;
    }
    else if(var_type =="Int8") 
        x_type = NC_BYTE;
    else if (var_type == "String")
        x_type = NC_CHAR;
    else if (var_type == "Int16")
        x_type = NC_SHORT;
    else if (var_type == "UInt16"){
        if(IsNC4_ENHANCED) 
            x_type = NC_USHORT;
        else 
            x_type = NC_INT;
    }
    else if (var_type == "Int32")
        x_type = NC_INT;
    else if (var_type == "UInt32"){
        if(IsNC4_ENHANCED) 
            x_type = NC_UINT;
        // classic model: don't support unsigned 32-bit int
    }
    else if (var_type == "Int64") {
        if(IsNC4_ENHANCED)
            x_type = NC_INT64;
        // classic model: don't support 64-bit int
    }
    else if (var_type == "UInt64"){
        if(IsNC4_ENHANCED) 
            x_type = NC_UINT64;
        // classic model: don't support 64-bit int
    }
    else if (var_type == "Float32")
        x_type = NC_FLOAT;
    else if (var_type == "Float64")
        x_type = NC_DOUBLE;

    BESDEBUG("fonc", "FONcUtils() - var_type returned"<< x_type <<endl);
    return x_type;
}

/** @brief generate a new name for the embedded variable
 *
 * This function takes the name of a variable as it exists in a data
 * file, and generates a new name given that netcdf does not have
 * structures or grids. Variables within structures and grids are
 * considered embedded variables, so a new name needs to be generated.
 *
 * The new name is then passed top id2netcdf to remove any characters
 * that are not allowed by netcdf.
 *
 * @param embed A list of names for parent structures
 * @param name The name of the variable to use for the new name
 * @param original The variable name before calling id2netcdf
 * @returns the newly generated name with embedded names preceeding it,
 * and converted using id2netcdf
 */
string FONcUtils::gen_name(const vector<string> &embed, const string &name, string &original)
{
    string new_name;
    vector<string>::const_iterator i = embed.begin();
    vector<string>::const_iterator e = embed.end();
    bool first = true;
    for (; i != e; i++) {
        if (first)
            new_name = (*i);
        else
            new_name += FONC_EMBEDDED_SEPARATOR + (*i);
        first = false;
    }
    if (first)
        new_name = name;
    else
        new_name += FONC_EMBEDDED_SEPARATOR + name;

    original = new_name;

    return FONcUtils::id2netcdf(new_name);
}

/** @brief Creates a FONc object for the given DAP object
 *
 * This is a simple factory for FONcBaseType objects that maps the
 * DAP2 data types into netCDF3 and netCDF4 types (actually instances of
 * FONcBaseType's specializations).
 *
 * @param v The DAP object to convert
 * @param ncdf_version The string that indicates if this is netCDF 3 or netCDF 4
 * @param is_classic_model The flag that indicates if the output is in classic model or not
 * @returns The FONc object created via the DAP object
 * @throws BESInternalError if the DAP object is not an expected type
 */
FONcBaseType *
FONcUtils::convert(BaseType *v, const string &ncdf_version, const bool is_classic_model) {
    map<string,int>fdimname_to_id;
    vector<int>rds_nums;
    return convert(v, ncdf_version, is_classic_model,fdimname_to_id,rds_nums);
}

/** @brief Creates a FONc object for the given DAP object
 *
 * This is a simple factory for FONcBaseType objects that maps the
 * DAP2 data types into netCDF3 and netCDF4 types (actually instances of
 * FONcBaseType's specializations).
 *
 * @param v The DAP object to convert
 * @param ncdf_version The string that indicates if this is netCDF 3 or netCDF 4
 * @param is_classic_model The flag that indicates if the output is in classic model or not
 * @param fdimname_to_id The file level(global) dimension names to dimension IDs map
 *                       DAP4 Dimension to netCDF-4 enhanced can be one to one map if
 *                       a DAP4 dimension is provided.
 *                       However, DAP4 also allows dimension that doesn't have a name,
 *                       so we need to this map to keep track of the case when there is
 *                       a one-to-one map from DAP4 to netCDF-4 enhanced.
 *                       This parameter is only used when the incoming object is DAP4 DMR.
 *@param rds_nums   The vector that stores the root dimension suffix numbers 
 *                  when the DAP4 user-defined dimension name is dim1, dim2 dim4 etc.
 *                  under the root group. The vector will be <1,2,4,..> in the above example.
 *                  This parameter is necessary to handle the DAP4 dimension name clashing
 *                  since the dimension names generated by fileout netCDF module is always 
 *                  dim1, dim2 etc when a dimension doesn't have a name. DAP4 allows this
 *                  case. So the suffix number vector is used to make the dimension name generated
 *                  by this module to skip the name created with the numbers stored in the vector.
 *                  The dimension name clashing is then avoided. For example, if 'dim1' is a user-defined
 *                  dimension name, so the module generated dimension names(if any) will start from
 *                  'dim2', then 'dim3' etc.
 *                  This parameter is only used when the incoming object is DAP4 DMR.
 *
 * @returns The FONc object created via the DAP object
 * @throws BESInternalError if the DAP object is not an expected type
 */
FONcBaseType *
FONcUtils::convert(BaseType *v, 
                   const string &ncdf_version, 
                   const bool is_classic_model, 
                   map<string,int>&fdimname_to_id,
                   vector<int>&rds_nums)
{
    FONcBaseType *b = nullptr;

    // We need to handle netCDF-4 enhanced differently. More datatypes are supported.
    bool is_netcdf4_enhanced = false;
    if(ncdf_version == RETURN_AS_NETCDF4 && is_classic_model == false)
        is_netcdf4_enhanced = true;
    
    bool unsigned_promote = true;

    switch (v->type()) {
    case dods_str_c:
    case dods_url_c:
        b = new FONcStr(v);
        break;
    case dods_uint8_c:
    case dods_byte_c: {
        if(true == is_netcdf4_enhanced)
            b = new FONcUByte(v);
        else 
            b = new FONcShort(v,unsigned_promote);
        break;
    }
    case dods_int8_c: {
        b = new FONcInt8(v);
        break;
    }

    case dods_uint16_c: {
        if(true == is_netcdf4_enhanced)
            b = new FONcUShort(v); 
        else 
            b = new FONcInt(v,unsigned_promote);
        break;
    }
    case dods_int16_c:
        b = new FONcShort(v);
        break;
    case dods_uint32_c: {
        if(true == is_netcdf4_enhanced)
            b = new FONcUInt(v); 
        else { 
            string err = (string) "file out netcdf, " + "classic model-doesn't support unsigned int.";
            err += " Please use netCDF-4 enhanced model instead.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        break;
    }
    case dods_int32_c:
        b = new FONcInt(v);
        break;
    case dods_uint64_c: {
        if(true == is_netcdf4_enhanced)
            b = new FONcUInt64(v); 
        else { 
            string err = (string) "file out netcdf, " + "classic model-doesn't support unsigned 64-bit int.";
            err += " Please use netCDF-4 enhanced model instead.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        break;
    }
    case dods_int64_c: {
        if(true == is_netcdf4_enhanced)
            b = new FONcInt64(v); 
        else {
            string err = (string) "file out netcdf, " + "classic model-doesn't support unsigned 64-bit int.";
            err += " Please use netCDF-4 enhanced model instead.";
            throw BESInternalError(err, __FILE__, __LINE__);
        }
        break;
    }
    case dods_float32_c:
        b = new FONcFloat(v);
        break;
    case dods_float64_c:
        b = new FONcDouble(v);
        break;
    case dods_grid_c:
        b = new FONcGrid(v);
        break;
    case dods_array_c:

        // This "if block" is only true for the netCDF-4 enhanced/DAP4 case.
        // fdimname_to_id is obtained in FONcTransform:transform_dap4_group_internal().
        if(fdimname_to_id.size()>0) {
            vector<int> dim_ids;
            vector<bool> use_d4_dim_ids;
            Array *t_a = dynamic_cast<Array *>(v);
            Array::Dim_iter di = t_a->dim_begin();
            Array::Dim_iter de = t_a->dim_end();
            // Here we want to check if this array has DAP4 dimension.  
            // If yes, we want to check if this DAP4 dimension is defined in the DAP4 group.
            // A DAP4 dimension fully_qualified name is used as a key.
            // Note: we also need to use a flag to mark if this dimension 
            // is defined by groups this var belongs to.
            // DAP4 doesn't require a variable's dimension to have a dimension name. 
            // When dim_id is 0, a dimension name will be created for this dimension.
            for (; di != de; di++) {

                D4Dimension * d4_dim = t_a->dimension_D4dim(di);
                if(d4_dim) {
                    BESDEBUG("fonc", "FONcArray() - constructor is dap4: dimension name is "<< d4_dim->name() <<endl);
                    if(fdimname_to_id.find(d4_dim->fully_qualified_name())!= fdimname_to_id.end()) {
                        int dim_id = fdimname_to_id[d4_dim->fully_qualified_name()];
                        dim_ids.push_back(dim_id);
                        use_d4_dim_ids.push_back(true);
                    }
                    else {
                        dim_ids.push_back(0);
                        use_d4_dim_ids.push_back(false);
                    }
                }
                else {
                    dim_ids.push_back(0);
                    use_d4_dim_ids.push_back(false);
                }

            }
            b = new FONcArray(v,dim_ids,use_d4_dim_ids,rds_nums);

        }
        else {
            b = new FONcArray(v);
        }
        break;
    case dods_structure_c:
        b = new FONcStructure(v);
        break;
    case dods_sequence_c:
        b = new FONcSequence(v);
        break;
    default:
        string err = (string) "file out netcdf, unable to " + "write unknown variable type";
        throw BESInternalError(err, __FILE__, __LINE__);

    }
    // The following code may be combined with other related code. TODO: later.
    b->setVersion(ncdf_version);
    if(ncdf_version == RETURN_AS_NETCDF4) {
        if(is_classic_model)
            b->setNC4DataModel("NC4_CLASSIC_MODEL");
        else
            b->setNC4DataModel("NC4_ENHANCED");
    }
    return b;
}

/** @brief handle any netcdf errors
 *
 * Looks up the netcdf error message associated with the provided netcdf
 * return value and throws an exception with that information appended to
 * the provided error message.
 *
 * @note Modified: This used to test the value of stax and return without
 * doing anything if stax == NC_NOERR. This should not be called if there
 * is no error.
 *
 * @param stax A netcdf return value. Should be any value other than NC_NOERR
 * @param err A provided error message to begin the error message with
 * @param file The source code file name where the error was generated
 * @param line The source code line number where the error was generated
 * @return Never returns
 *
 * @throws BESError if the return value represents a netcdf error
 */
void FONcUtils::handle_error(int stax, const string &err, const string &file, int line)
{
    assert(stax != NC_NOERR);   // This should not be called for NOERR

    throw BESInternalError(err + string(": ") + nc_strerror(stax), file, line);
}

