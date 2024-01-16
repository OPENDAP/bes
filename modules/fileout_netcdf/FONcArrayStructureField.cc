// FONcArrayStructureField.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>
#include <libdap/Array.h>
#include <libdap/Structure.h>
#include <libdap/Byte.h>
#include <libdap/Int8.h>
#include <libdap/Int16.h>
#include <libdap/Int32.h>
#include <libdap/Int64.h>
#include <libdap/UInt16.h>
#include <libdap/UInt32.h>
#include <libdap/UInt64.h>
#include <libdap/Float32.h>
#include <libdap/Float64.h>
#include <libdap/util.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include "FONcArrayStructureField.h"
#include "FONcDim.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

vector<FONcDim *> FONcArrayStructureField::SDimensions;
/** @brief Constructor for FOncInt that takes a DAP Int32 or UInt32
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Int32 or UInt32 instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be an int32 or uint32
 * @throws BESInternalError if the BaseType is not an Int32 or UInt32
 */
FONcArrayStructureField::FONcArrayStructureField( BaseType *b, Array* a)
    : FONcBaseType()
{
    // We only support one-layer of simple int/float array or scalar fields inside an array of structure now.
    Type b_data_type = b->type();
    Type supported_atomic_data_type = b_data_type;
    if(b_data_type == libdap::dods_array_c)
        supported_atomic_data_type = b->var()->type();
    if (!is_simple_type(supported_atomic_data_type) || supported_atomic_data_type == dods_str_c
        || supported_atomic_data_type == dods_url_c
        || supported_atomic_data_type == dods_enum_c || supported_atomic_data_type ==dods_opaque_c) {
        string s = "File out netcdf, only support one-layer of simple int/float fields inside an array of structure.";
        throw BESInternalError(s, __FILE__, __LINE__);
    }

    d_a = dynamic_cast<Array *>(a);
    if (!d_a) {
        string s = "File out netcdf, FONcArrayStructField was passed a variable that is not a DAP Array";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    d_varname = b->name();
    if (b_data_type == libdap::dods_array_c) {
        d_array_type = FONcUtils::get_nc_type(b->var(), isNetCDF4_ENHANCED());
        d_array_type_size = (b->var()->width_ll())/(b->var()->length_ll());
    }
    else {
        d_array_type = FONcUtils::get_nc_type(b, isNetCDF4_ENHANCED());
        d_array_type_size = (b->width_ll())/(b->length_ll());
    }
    // Need to retrieve dimension information here.
    // In this version, we only try to get the dimension size right.
    Array::Dim_iter di = d_a->dim_begin();
    Array::Dim_iter de = d_a->dim_end();
    for (; di != de; di++) {
        int64_t size = d_a->dimension_size_ll(di, true);
        struct_dim_sizes.push_back(size);
        total_nelements *= size;
        FONcDim *use_dim = find_sdim(d_a->dimension_name(di), size);
        struct_dims.push_back(use_dim);
    }
    if (b_data_type == libdap::dods_array_c) {
        auto db_a = dynamic_cast<Array *>(b);
        if (!db_a){
            string s = "File out netcdf, FONcArrayStructField was passed a variable that is not a DAP Array";
            throw BESInternalError(s, __FILE__, __LINE__);
        }
        Array::Dim_iter b_di = db_a->dim_begin();
        Array::Dim_iter b_de = db_a->dim_end();
        for (; b_di != b_de; b_di++) {
            int64_t size = d_a->dimension_size_ll(b_di, true);
            struct_dim_sizes.push_back(size);
            total_nelements *= size;
            field_nelements *= size;
            FONcDim *use_dim = find_sdim(db_a->dimension_name(b_di), size);
            struct_dims.push_back(use_dim);
        }
    }

}

/** @brief Destructor that cleans up the instance
 *
 * The DAP Int32 or UInt32 instance does not belong to the FONcByte
 * instance, so it is not deleted.
 */
FONcArrayStructureField::~FONcArrayStructureField()
{
}

void FONcArrayStructureField::convert(vector<string> embed, bool _dap4, bool is_dap4_group){
    var_name = FONcUtils::gen_name(embed,d_varname, d_orig_varname);

}
void
FONcArrayStructureField::convert_asf( std::vector<std::string> embed) {


}

/** @brief define the DAP Int32 or UInt32 in the netcdf file
 *
 * The definition actually takes place in FONcBaseType. This function
 * adds the attributes for the instance as well as an attribute if
 * the name of the variable had to be modified.
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * Int32 or UInt32
 */
void
FONcArrayStructureField::define( int ncid )
{
    BESDEBUG("fonc", "FONcArray::define() - defining array '" << d_varname << "'" << endl);

    if (!d_defined) {

        BESDEBUG("fonc", "FONcArray::define() - defining array of structure field: " << d_varname << "'" << endl);
        vector<FONcDim *>::iterator i = struct_dims.begin();
        vector<FONcDim *>::iterator e = struct_dims.end();
        for (; i != e; i++) {
            FONcDim *fd = *i;
            fd->define_struct(ncid);
            d_dim_ids.push_back(fd->dimid());
            BESDEBUG("fonc", "FONcArray::define() - dim_id: " << fd->dimid() << " size:" << fd->size() << endl);
        }

        int stax = nc_def_var(ncid, var_name.c_str(), d_array_type, (int)(struct_dims.size()), d_dim_ids.data(), &d_varid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - Failed to define variable " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        stax = nc_def_var_fill(ncid, d_varid, NC_NOFILL, NULL );
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to clear fill value for " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }
}

/** @brief Write the int out to the netcdf file
 *
 * Once the int is defined, the value of the int can be written out
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the value
 */
void
FONcArrayStructureField::write( int ncid )
{
    BESDEBUG( "fonc", "FONcArrayStructureField::write for var " << d_varname << endl ) ;

    vector<char> data_buf;
    data_buf.resize(total_nelements*d_array_type_size);
    char* data_buf_ptr = data_buf.data();
    vector<BaseType*> compound_buf = d_a->get_compound_buf();
    for (unsigned i= 0; i<d_a->length_ll();i++) {
        BaseType *cb = compound_buf[i];
        if (cb->type()!=libdap::dods_structure_c){
            throw BESInternalError("Fileout netcdf: This is not array of structure", __FILE__, __LINE__);
        }
        Structure* structure_elem = dynamic_cast<Structure *>(cb);
        if (!structure_elem)
            throw BESInternalError("Fileout netcdf: This is not array of structure", __FILE__, __LINE__);
        //loop through the structure

        Constructor::Vars_iter vi = structure_elem->var_begin();
        Constructor::Vars_iter ve = structure_elem->var_end();
        for (; vi != ve; vi++) {
            BaseType *bt = *vi;
            if (bt->send_p()) {

                if (bt->name() == d_varname) {
                    if (bt->type() == libdap::dods_array_c) {
                        Array *memb_array = dynamic_cast<Array *>(bt);
                        if (!memb_array)
                            throw BESInternalError("Fileout netcdf: This structure member is not an array", __FILE__, __LINE__);
                        char *buf = memb_array->get_buf();
                        size_t memb_array_size = memb_array->width_ll();
                        // fill in the data_buf.
                        memcpy(data_buf_ptr, buf, memb_array_size);
                        BESDEBUG("fonc","memb_array_length is "<<memb_array->length_ll()<<endl);
                        BESDEBUG("fonc","memb_array_type is "<<memb_array->width_ll()<<endl);
                        BESDEBUG("fonc","memb_array_size is "<<memb_array_size<<endl);
                        data_buf_ptr +=memb_array_size;

                    } else {// Need to switch to different data types
                        obtain_scalar_data(data_buf_ptr,bt);
                        data_buf_ptr += d_array_type_size;
                    }
                }
            }
        }
    }
    int stax = nc_put_var(ncid, d_varid, (void*)data_buf.data());
    if (stax != NC_NOERR) {
        string err = "fileout.netcdf - cannot write the array of structure members " + d_varname;
        FONcUtils::handle_error(stax , err, __FILE__, __LINE__);
    }
}

void FONcArrayStructureField::obtain_scalar_data(char *data_buf_ptr, BaseType *b) {

    switch (b->type()) {

        case dods_uint8_c:
        case dods_byte_c: {
            auto byte_var = dynamic_cast<Byte *>(b);
            uint8_t byte_value = byte_var->value();
            memcpy(data_buf_ptr,(void*)&byte_value,d_array_type_size);
            break;
        }
        case dods_int8_c: {
            auto int8_var = dynamic_cast<Int8 *>(b);
            int8_t int8_value = int8_var->value();
            memcpy(data_buf_ptr,(void*)&int8_value,d_array_type_size);
            break;
        }

        case dods_uint16_c: {
            auto uint16_var = dynamic_cast<UInt16 *>(b);
            uint16_t uint16_value = uint16_var->value();
            memcpy(data_buf_ptr,(void*)&uint16_value,d_array_type_size);
            break;
        }
        case dods_int16_c: {
            auto int16_var = dynamic_cast<Int16 *>(b);
            int16_t int16_value = int16_var->value();
            memcpy(data_buf_ptr, (void *) &int16_value, d_array_type_size);
            break;
        }
        case dods_uint32_c: {
            auto uint32_var = dynamic_cast<UInt32 *>(b);
            uint32_t uint32_value = uint32_var->value();
            memcpy(data_buf_ptr,(void*)&uint32_value,d_array_type_size);
            break;
        }
        case dods_int32_c: {
            auto int32_var = dynamic_cast<Int32 *>(b);
            int32_t int32_value = int32_var->value();
            memcpy(data_buf_ptr, (void *) &int32_value, d_array_type_size);
            break;
        }
        case dods_uint64_c: {
            auto uint64_var = dynamic_cast<UInt64 *>(b);
            uint64_t uint64_value = uint64_var->value();
            memcpy(data_buf_ptr,(void*)&uint64_value,d_array_type_size);
            break;
        }
        case dods_int64_c: {
            auto int64_var = dynamic_cast<Int64 *>(b);
            int64_t int64_value = int64_var->value();
            memcpy(data_buf_ptr,(void*)&int64_value,d_array_type_size);
            break;
        }
        case dods_float32_c: {
            auto float32_var = dynamic_cast<Float32 *>(b);
            float float32_value = float32_var->value();
            memcpy(data_buf_ptr,(void*)&float32_value,d_array_type_size);
            break;
        }
        case dods_float64_c: {
            auto float64_var = dynamic_cast<Float64 *>(b);
            double float64_value = float64_var->value();
            memcpy(data_buf_ptr,(void*)&float64_value,d_array_type_size);
            break;
        }
        default:
            string err = (string) "file out netcdf structure array: Only support int/float types";
            throw BESInternalError(err, __FILE__, __LINE__);
    }

}
/** @brief returns the name of the DAP Int32 or UInt32
 *
 * @returns The name of the DAP Int32 or UInt32
 */
string
FONcArrayStructureField::name()
{
    return var_name ;
}

/** @brief returns the netcdf type of the DAP object
 *
 * @returns The nc_type of NC_INT
 */
nc_type
FONcArrayStructureField::type()
{
    return d_array_type;
}

FONcDim *
FONcArrayStructureField::find_sdim(const string &name, int64_t size) {

    FONcDim *ret_dim = nullptr;
    vector<FONcDim *>::iterator i = FONcArrayStructureField::SDimensions.begin();
    vector<FONcDim *>::iterator e = FONcArrayStructureField::SDimensions.end();
    for (; i != e && !ret_dim; i++) {
        if (!((*i)->name().empty()) && ((*i)->name() == name)) {
            if ((*i)->size() == size) {
                ret_dim = (*i);
            }
            else {
                string err = "fileout_netcdf: dimension found with the same name, but different size";
                throw BESInternalError(err, __FILE__, __LINE__);
            }
        }
    }

    if (!ret_dim) {
        ret_dim = new FONcDim(name, size);
        FONcArrayStructureField::SDimensions.push_back(ret_dim);
    }
    else {
        ret_dim->struct_incref();
    }

    return ret_dim;
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void
FONcArrayStructureField::dump( ostream &strm ) const
{
#if 0
    strm << BESIndent::LMarg << "FONcArrayStructureField::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;
    strm << BESIndent::LMarg << "name = " << _bt->name()  << endl ;
    BESIndent::UnIndent() ;
#endif
}

