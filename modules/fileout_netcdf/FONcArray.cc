// FONcArray.cc

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
//      Kent Yang <myang6@hdfgroup.org> (for DAP4/netCDF-4 enhancement)

#include <sstream>
#include <algorithm>
#include <cstring>

#include <netcdf.h>

#include <libdap/Array.h>
#include <libdap/AttrTable.h>
#include <libdap/D4Attributes.h>

#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESUtil.h>

#include "FONcRequestHandler.h" // For access to the handler's keys
#include "FONcArray.h"
#include "FONcDim.h"
#include "FONcGrid.h"
#include "FONcMap.h"
#include "FONcUtils.h"
#include "FONcAttributes.h"

using namespace libdap;

// This controls whether variables' data values are deleted as soon
// as they are written (except for DAP2 Grid Maps, which may be shared).
#define CLEAR_LOCAL_DATA 1
#define STRING_ARRAY_OPT 1

vector<FONcDim *> FONcArray::Dimensions;

const int MAX_CHUNK_SIZE = 1024;

// Set general total maximum chunk sizes to 1M(1024x1024)
const int GENERAL_MAX_CHUNK_SIZES = 1048576; 

// set normal 1-D maximum chunk sizes to 64K(64*1024)
const int NORMAL_1D_MAX_CHUNK_SIZES = 65536;

/** @brief Constructor for FONcArray that takes a DAP Array
 *
 * This constructor takes a DAP BaseType and makes sure that it is a DAP
 * Array instance. If not, it throws an exception
 *
 * @param b A DAP BaseType that should be an array
 * @throws BESInternalError if the BaseType is not an Array
 */
FONcArray::FONcArray(BaseType *b) : FONcBaseType() {
    d_a = dynamic_cast<Array *>(b);
    if (!d_a) {
        string s = "File out netcdf, FONcArray was passed a variable that is not a DAP Array";
        throw BESInternalError(s, __FILE__, __LINE__);
    }

    for (unsigned int i = 0; i < d_a->dimensions(); i++)
        use_d4_dim_ids.push_back(false);
}

FONcArray::FONcArray(BaseType *b, const vector<int> &fd4_dim_ids, const vector<bool> &fuse_d4_dim_ids,
                     const vector<int> &rds_nums) : FONcBaseType() {
    d_a = dynamic_cast<Array *>(b);
    if (!d_a) {
        string s = "File out netcdf, FONcArray was passed a variable that is not a DAP Array";
        throw BESInternalError(s, __FILE__, __LINE__);
    }
    if (d_a->is_dap4()) {
        BESDEBUG("fonc", "FONcArray() - constructor is dap4 " << endl);
        d4_dim_ids = fd4_dim_ids;
        use_d4_dim_ids = fuse_d4_dim_ids;
        d4_def_dim = true;
        d4_rds_nums = rds_nums;
    }
}

/** @brief Destructor that cleans up the array
 *
 * The destructor cleans up by removing the array dimensions from it's
 * list. Since the dimensions can be shared by other arrays, FONcDim
 * uses reference counting, so the instances aren't actually deleted
 * here, but their reference count is decremented
 *
 * The DAP Array instance does not belong to the FONcArray instance, so
 * it is not deleted.
 */
FONcArray::~FONcArray() {
    for (auto &dim: d_dims) {
        dim->decref();
    }

    for (auto &map: d_grid_maps) {
        map->decref();
    }
}

/** @brief Converts the DAP Array to a FONcArray
 *
 * Does this by converting the name to a valid netcdf variable name,
 * creating FONcDim instances for each of the dimensions of the array,
 * or finding a shared dimension in the global list of dimensions.
 *
 * Also keeps track of any single dimensional arrays where the name of
 * the array is the same as the dimension name, as these could be maps
 * for grids that might be defined.
 *
 * @param embed A list of strings for each name of parent structures or
 * grids
 * @throws BESInternalError if there is a problem converting the Array
 */
void FONcArray::convert(vector<string> embed, bool _dap4, bool is_dap4_group) {
    FONcBaseType::convert(embed, _dap4, is_dap4_group);

    d_varname = FONcUtils::gen_name(embed, d_varname, d_orig_varname);

    BESDEBUG("fonc", "FONcArray::convert() - converting array " << d_varname << endl);

    d_array_type = FONcUtils::get_nc_type(d_a->var(), isNetCDF4_ENHANCED());

    if(d_array_type == NC_NAT) {

        string err = "fileout_netcdf: The datatype of this variable '" + d_varname;
        err += "' is not supported. It is very possible that you try to obtain ";
        err += "a netCDF file that follows the netCDF classic model. ";
        err += "The unsigned 32-bit integer and signed/unsigned 64-bit integer ";
        err += "are not supported by the netCDF classic model. Downloading this file as the netCDF-4 file that ";
        err += "follows the netCDF enhanced model should solve the problem.";

        throw BESInternalError(err, __FILE__, __LINE__);
    }

#if !NDEBUG
    if (d4_dim_ids.size() > 0) {
        BESDEBUG("fonc", "FONcArray::convert() - d4_dim_ids size is " << d4_dim_ids.size() << endl);
    }
#endif

    d_ndims = d_a->dimensions();
    d_actual_ndims = d_ndims; //replace this with _a->dimensions(); below TODO
    if (d_array_type == NC_CHAR) {
        // if we have an array of strings then we need to add the string length
        // dimension, so add one more to ndims
        d_ndims++;
    }

    // See HYRAX-805. When assigning values using [], set the size using resize
    // not reserve. THe reserve method works to optimize push_back(), but apparently
    // does not work with []. jhrg 8/3/18
    d_dim_ids.resize(d_ndims);
    d_dim_sizes.resize(d_ndims);

    
    Array::Dim_iter di = d_a->dim_begin();
    Array::Dim_iter de = d_a->dim_end();
    int dimnum = 0;
    for (; di != de; di++) {
        int64_t size = d_a->dimension_size_ll(di, true);
        d_dim_sizes[dimnum] = size;
        d_nelements *= size;
        // Set COMPRESSION CHUNK SIZE for each dimension.
        // Make the chunk size reasonable for 1-D case.
        // For the 1-D case, the MAX_CHUNK_SIZE(1K) is too small if the array size is too big.
        // We increase the chunk size to 64K if the number of elements is >64K and less than  16M.
        // If the number of elements is >16M, we set the chunk size to 1M.
        // KY 2023-01-31
        if (d_a->dimensions() == 1) {
            if (size < NORMAL_1D_MAX_CHUNK_SIZES)
                d_chunksizes.push_back(size <= MAX_CHUNK_SIZE ? size : MAX_CHUNK_SIZE);
            else if ( size >= NORMAL_1D_MAX_CHUNK_SIZES && size <=GENERAL_MAX_CHUNK_SIZES*16)
                d_chunksizes.push_back(NORMAL_1D_MAX_CHUNK_SIZES);
            else 
                d_chunksizes.push_back(GENERAL_MAX_CHUNK_SIZES);
        }
        // For the 2-D case, we keep the original way to set the chunk size as MAX_CHUNK_SIZE for each dimension.
        // This turns out to be good enough for most NASA files so far. KY 2023-01-31
        else if (d_a->dimensions() ==2) 
            d_chunksizes.push_back(size <= MAX_CHUNK_SIZE ? size : MAX_CHUNK_SIZE);
        // We found an array that has 3-D 365x8075*7814 elements. This will make the chunk size 365*1024*1024, which
        // is too big and will cause potential bad performance for the application that uses the generated
        // netcdf file. So reduce the chunk size when the similar case occurs. 
        // This will be handled in a separate for-loop below. KY 2023-01-25

        BESDEBUG("fonc", "FONcArray::convert() - dim num: " << dimnum << ", dim size: " << size << endl);
        BESDEBUG("fonc", "FONcArray::convert() - dim name: " << d_a->dimension_name(di) << endl);

        // If this dimension is a D4 dimension defined in its group, just obtain the dimension ID.
        if (true == d4_def_dim && use_d4_dim_ids[dimnum] == true) {
            d_dim_ids[dimnum] = d4_dim_ids[dimnum];
            BESDEBUG("fonc", "FONcArray::convert() - has dap4 group" << endl);

        }
        else {
            // See if this dimension has already been defined. If it has the
            // same name and same size as another dimension, then it is a
            // shared dimension. Create it only once and share the FONcDim
            int ds_num = FONcDim::DimNameNum + 1;
            while (find(d4_rds_nums.begin(), d4_rds_nums.end(), ds_num) != d4_rds_nums.end()) {
            // Note: the following #if 0 #endif block is only for future development.
            //       Don't delete or change it for debugging.
#if 0
                // This may be an optimization for rare cases. May do this when performance issue hurts
                //d4_rds_nums_visited.push_back(ds_num);
#endif
                // Now the following line ensure this dimension name dimds_num(ds_num is a number)
                // is NOT created for the dimension that doesn't have a name in DAP4.
                ds_num++;
            }
            FONcDim::DimNameNum = ds_num - 1;

            FONcDim *use_dim = find_dim(embed, d_a->dimension_name(di), size);
            d_dims.push_back(use_dim);
        }

        dimnum++;
    }

    // We found an array that has 3-D 365x8075*7814 elements. This will make the chunk size 365*1024*1024, which
    // is too big and will cause potential bad performance for the application that uses the generated
    // netcdf file. So reduce the chunk size when the similar case occurs.
    // The following rules handle the cases described above.
    // Given there may be a very large size array and also the chunk size cannot too big,
    // we modify the maximum chunk size according to the array size if necessary. 
    // The idea is we don't want to have too many chunks and we don't want to have the chunk size too big.
    // So we do the following. 
    // 1. We start from 1M(1024x1024), if the number of chunks for the array is >64, we increase the chunk size to be 2048x2048.
    // 2. We can increase the chunk size to 16M(4096x4096) if the number of chunks is still >64.
    // 3. The maximum chunk size is 16M(4096x4096) no matter how big the array size is.
    // 4. We can increase the maximum chunk size to a bigger number in the future if necessary.
    //    We can also change the maximum number of chunks.
    // KY 2023-01-26

    // The above algorithm works well to reduce the number of chunks for a large array size as well as having a decent
    // access time. However,
    // it may increase the final file size when the fastest changing dimension size increases from 1024 to 2048 or 4096.
    // So to find a good balance between the file size and the access time for NASA files,
    // we fix the sizes of two fastest changing dimensions to be <=1024x1024. We will increase the higher chunk dimension size 
    // from 1 to 2 or higher if the number of higher dimension size is >512. Why 512? Because we find the higher dimension
    // may be time and one year is 365 days. We want to still keep the one day per slice if possible. 512 is not too big. 
    // The chunk overhead is not that big.
    // KY 2023-01-29
    // One more optimiziation: if the sizes of two fastest changing dimensions are much less than 1M(1024x1024),given
    // that the higher dimension size may be large, we allow the chunk size grow to 1M and then if we still find the number of
    // higher dimension size is >512, we then grow the chunk size gradually. This will make the bigger array size not hold
    // too many chunks.

    // Chunk size is not set when the number of dimension is >2. It is set here.
    // When the number of higher than 2 dimension sizes is <max_num_higher_chunks(512 now), we start increasing the higher chunk dimension size. 
    // KY 2023-01-29
    if (d_a->dimensions() >2) {

        // The chunk sizes of the two fastest changing dimensions are fixed(not exceeding the MAX_CHUNK_SIZE). So set them first.
        size_t two_fastest_chunk_dim_sizes=1;
        auto d_chunksize_it = d_chunksizes.begin();

        for (size_t i = d_a->dimensions();i>d_a->dimensions()-2; i--) {

            size_t size = d_dim_sizes[i-1];
            BESDEBUG("fonc", "FONcArray::CHUNK - dim size backward: " << size << endl);
            d_chunksize_it = d_chunksizes.insert(d_chunksize_it, size <= MAX_CHUNK_SIZE ? size : MAX_CHUNK_SIZE);
            two_fastest_chunk_dim_sizes *=d_chunksizes[0];
        }
        BESDEBUG("fonc", "FONcArray::CHUNK - two fastest dimchunk_sizes " << two_fastest_chunk_dim_sizes << endl);

        // Set the chunk sizes for the rest dimensions if the array size is not too big.
        // Calculate the dimension index when the total chunk sizes exceed 1024x1024.
        // Without doing this, an extreme case is that a 511x1x1 array will set chunk size to 1x1x1, we don't want this to happen. 
 
        size_t rest_dim_stop_index = d_a->dimensions()-2;

        if (two_fastest_chunk_dim_sizes <= GENERAL_MAX_CHUNK_SIZES) {

            size_t total_chunk_size_so_far = two_fastest_chunk_dim_sizes;

            for (int i = d_a->dimensions()-2;i>0; i--) {

                size_t size = d_dim_sizes[i-1];
                size_t chunk_size_candidate =((size<=MAX_CHUNK_SIZE)?size:MAX_CHUNK_SIZE); 

                if (total_chunk_size_so_far * chunk_size_candidate <= GENERAL_MAX_CHUNK_SIZES) {
                    total_chunk_size_so_far *=chunk_size_candidate;
                    d_chunksize_it = d_chunksizes.insert(d_chunksize_it,chunk_size_candidate);
                    // If i =1, we reach the first dimension. Set the dim stop index to 0.
                    if (i == 1)
                        rest_dim_stop_index = 0;
                }
                else {
                    rest_dim_stop_index = i;
                    break;
                }
            }
            BESDEBUG("fonc", "FONcArray::CHUNK - total_chunk_size_so_far: " << total_chunk_size_so_far << endl);
        } 
        BESDEBUG("fonc", "FONcArray::CHUNK - rest_dim_stop_index: " << rest_dim_stop_index << endl);
            

        // Now if our chunk size is already 1M and there are too many chunks in this big array. 
        // We may increase the chunk size. 512 is the current maximum number of chunks.

        size_t higher_dimension_size = 1;
        size_t total_higher_dim_chunk_size = 1;
        int max_num_higher_chunks = 512;

        for (size_t i = 0; i<rest_dim_stop_index; i++) 
            higher_dimension_size *= d_dim_sizes[i];

        if (higher_dimension_size > (size_t)max_num_higher_chunks)
            total_higher_dim_chunk_size = higher_dimension_size/max_num_higher_chunks;
        
        size_t left_higher_dim_chunk_size = total_higher_dim_chunk_size;
        
        // We have to walk the dimension backward to make sure the fastest changing dimension chunk sizes set properly.
        for (size_t i = rest_dim_stop_index;i>0; i--) {

            size_t size = d_dim_sizes[i-1];

            BESDEBUG("fonc", "FONcArray::CHUNK - left_higher_dim_chunk_size " << left_higher_dim_chunk_size << endl);
            if (size < left_higher_dim_chunk_size) {
                d_chunksize_it = d_chunksizes.insert(d_chunksize_it,size);
                // The if block is not necessary, just make the sonar cloud happy
                if (size !=0) 
                    left_higher_dim_chunk_size = left_higher_dim_chunk_size/size;
            }
            else {
                d_chunksize_it = d_chunksizes.insert(d_chunksize_it,left_higher_dim_chunk_size);
                // Set the left higher dim chunk size to 1 since all the left hgher chunk dimension size(if any) should be 1.
                left_higher_dim_chunk_size = 1;
            }
        }
    }

#ifndef NDEBUG
    for( const auto &chunk_size:d_chunksizes) 
        BESDEBUG("fonc", "FONcArray::CHUNK - chunk_size final: " <<chunk_size << endl);
#endif

    // if this array is a string array, then add the length dimension
    if (d_array_type == NC_CHAR) {
        // Calling intern_data() here is part of the 'streaming' refactoring.
        // For the other types, the call can go in the write() implementations,
        // but because strings in netCDF are arrays of char, a string array
        // must have an added dimension (so a 1d string array becomes a 2d char
        // array). To build the netCDF file, we need to know the dimensions of
        // the array when the file is defined, not when the data are written.
        // To know the size of the extra dimension used to hold the chars, we
        // need to look at all the strings and find the biggest one. Thus, in
        // order to define the variable for the netCDF file, we need to read
        // string data long before we actually write it out. Kind of a drag,
        // but not the end of the world. jhrg 5/18/21

        // For NC_CHAR, the module needs to call the intern_data().  
        // This routine is called in the convert(). So it should not called in define().
        if (d_is_dap4)
            d_a->intern_data();
        else
            d_a->intern_data(*get_eval(), *get_dds());

        // get the data from the dap array
        int array_length = d_a->length();
#if 0
        d_str_data.reserve(array_length);
        d_a->value(d_str_data);

        // determine the max length of the strings
        size_t max_length = 0;
        for (int i = 0; i < array_length; i++) {
            if (d_str_data[i].size() > max_length) {
                max_length = d_str_data[i].size();
            }
        }
        max_length++;
#endif
        size_t max_length = 0;
        for (int i = 0; i < array_length; i++) {
            if (d_a->get_str()[i].size() > max_length) {
                max_length = d_a->get_str()[i].size();
            }
        }
        max_length++;

        vector<string> empty_embed;
        string lendim_name;
        if (is_dap4_group == true) {
            // Here is a quick implementation. 
            // We just append the DimNameNum(globally defined)
            // and then increase the number by 1.
            ostringstream dim_suffix_strm;
            dim_suffix_strm << "_len" << FONcDim::DimNameNum + 1;
            FONcDim::DimNameNum++;
            lendim_name = d_varname + dim_suffix_strm.str();

        }
        else
            lendim_name = d_varname + "_len";


        FONcDim *use_dim = find_dim(empty_embed, lendim_name, max_length, true);
        // Added static_cast to suppress warning. 12.27.2011 jhrg
        if (use_dim->size() < static_cast<int>(max_length)) {
            use_dim->update_size(max_length);
        }

        d_dim_sizes[d_ndims - 1] = use_dim->size();
        d_dim_ids[d_ndims - 1] = use_dim->dimid();

        //DAP4 dimension ID is false.
        use_d4_dim_ids.push_back(false);
        d_dims.push_back(use_dim);

        // Adding this fixes the bug reported by GSFC where arrays of strings
        // caused the handler to throw an error stating that 'Bad chunk sizes'
        // were used. When the dimension of the string array was extended (because
        // strings become char arrays in netcdf3/4), the numbers of dimensions
        // in 'chunksizes' was not bumped up. The code below in convert() that
        // set the chunk sizes then tried to access data that had never been set.
        // jhrg 11/25/15
        d_chunksizes.push_back(max_length <= MAX_CHUNK_SIZE ? max_length : MAX_CHUNK_SIZE);
    }

    // If this array has a single dimension, and the name of the array
    // and the name of that dimension are the same, then this array
    // might be used as a map for a grid defined elsewhere.
    // Notice: DAP4 doesn't have Grid and the d_dont_use_it=true causes some
    // variables not written to the  netCDF-4 file with group hierarchy.
    // So need to have the if check. KY 2021-06-21
    if(d_is_dap4 == false) {
        if (!FONcGrid::InGrid && d_actual_ndims == 1 && d_a->name() == d_a->dimension_name(d_a->dim_begin())) {
            // is it already in there?
            const FONcMap *map = FONcGrid::InMaps(d_a);
            if (!map) {
                // This memory is/was leaked. jhrg 8/28/13
               auto new_map = new FONcMap(this);
                d_grid_maps.push_back(new_map);        // save it here so we can free it later. jhrg 8/28/13
                FONcGrid::Maps.push_back(new_map);
            }
            else {
                d_dont_use_it = true;
            }
        }
    }

    BESDEBUG("fonc", "FONcArray::convert() - done converting array " << d_varname << endl);
}

/** @brief Find a possible shared dimension in the global list
 *
 * If a dimension has the same name and size as another, then it is
 * considered a shared dimension. All dimensions for this DataDDS are
 * stored in a global list.
 *
 * @param name Name of the dimension to find
 * @param size Size of the dimension to find
 * @returns either a new instance of FONcDim, or a shared FONcDim
 * instance
 * @throws BESInternalError if the name of a dimension is the same, but
 * the size is different
 */
FONcDim *
FONcArray::find_dim(const vector<string> &embed, const string &name, int64_t size, bool ignore_size) {
    string oname;
    string ename = FONcUtils::gen_name(embed, name, oname);
    FONcDim *ret_dim = nullptr;
    vector<FONcDim *>::iterator i = FONcArray::Dimensions.begin();
    vector<FONcDim *>::iterator e = FONcArray::Dimensions.end();
    for (; i != e && !ret_dim; i++) {
        if (!((*i)->name().empty()) && ((*i)->name() == name)) {
            if (ignore_size) {
                ret_dim = (*i);
            }
            else if ((*i)->size() == size) {
                ret_dim = (*i);
            }
            else {
                if (embed.size() > 0) {
                    vector<string> tmp;
                    return find_dim(tmp, ename, size);
                }
                string err = "fileout_netcdf: dimension found with the same name, but different size";
                throw BESInternalError(err, __FILE__, __LINE__);
            }
        }
    }

    if (!ret_dim) {
        ret_dim = new FONcDim(name, size);
        FONcArray::Dimensions.push_back(ret_dim);
    }
    else {
        ret_dim->incref();
    }

    return ret_dim;
}

/** @brief define the DAP Array in the netcdf file
 *
 * This includes creating the dimensions, if they haven't already been
 * created, and then defining the array itself. Once the array is
 * defined, all of the attributes are written out.
 *
 * If the Array is an array of strings, an additional dimension is
 * created to represent the maximum length of the strings so that the
 * array can be written out as text
 *
 * @param ncid The id of the NetCDF file
 * @throws BESInternalError if there is a problem defining the
 * dimensions or variable
 */
void FONcArray::define(int ncid) {
    BESDEBUG("fonc", "FONcArray::define() - defining array '" << d_varname << "'" << endl);

    if (!d_defined && !d_dont_use_it) {

        BESDEBUG("fonc", "FONcArray::define() - defining array ' defined already: " << d_varname << "'" << endl);

        // Note: the following #if 0 #endif block is only for future development.
        //       Don't delete or change it for debugging.
#if 0
        if(d4_dim_ids.size() >0) {
           if(d_array_type == NC_CHAR) {
               if(d_dims.size() == 1) {
                   FONcDim *fd = *(d_dims.begin());
                   fd->define(ncid);
                   d_dim_ids[d_ndims-1] = fd->dimid();

               }
               else {

               }
           }
        }
        else {
#endif
        // If not defined DAP4 dimensions(mostly DAP2 or DAP4 no groups)
        if (false == d4_def_dim) {
            vector<FONcDim *>::iterator i = d_dims.begin();
            vector<FONcDim *>::iterator e = d_dims.end();
            int dimnum = 0;
            for (; i != e; i++) {
                FONcDim *fd = *i;
                fd->define(ncid);
                d_dim_ids[dimnum] = fd->dimid();
                BESDEBUG("fonc", "FONcArray::define() - dim_id: " << fd->dimid() << " size:" << fd->size() << endl);
                dimnum++;
            }
        }
        else {// Maybe some dimensions are not DAP4 dimensions, will still generate those dimensions.
            int j = 0;
            for (unsigned int i = 0; i < use_d4_dim_ids.size(); i++) {
                if (use_d4_dim_ids[i] == false) {
                    FONcDim *fd = d_dims[j];
                    fd->define(ncid);
                    d_dim_ids[i] = fd->dimid();
                    j++;
                }
            }
        }

        int stax = nc_def_var(ncid, d_varname.c_str(), d_array_type, d_ndims, d_dim_ids.data(), &d_varid);
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - Failed to define variable " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        stax = nc_def_var_fill(ncid, d_varid, NC_NOFILL, NULL );
        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - " + "Failed to clear fill value for " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        // Check if the direct IO flag is set. Note intern_data() is called in define().
        if (d_array_type != NC_CHAR) {
            if (d_is_dap4)
                d_a->intern_data();
            else    
                d_a->intern_data(*get_eval(), *get_dds());
        }

        bool d_io_flag = d_a->get_dio_flag();

#ifndef NBEBUG
        BESDEBUG("fonc", "d_io_flag: "<<d_io_flag<<endl);
        
        if (d_io_flag) {

            Array::var_storage_info dmrpp_vs_info = d_a->get_var_storage_info();

            BESDEBUG("fonc", "filters: "<<dmrpp_vs_info.filter<<endl);
            for (const auto& def_lev:dmrpp_vs_info.deflate_levels) 
                BESDEBUG("fonc", "deflate level: "<<def_lev<<endl);
    
            for (unsigned int i = 0; i < dmrpp_vs_info.chunk_dims.size(); i++) 
                BESDEBUG("fonc", "chunk_dim["<<i<<"]: "<<dmrpp_vs_info.chunk_dims[i]<<endl);
    
            for (unsigned int i = 0; i<dmrpp_vs_info.var_chunk_info.size(); i++) {
                BESDEBUG("fonc", "chunk index: "<<i<<" filter mask "<<dmrpp_vs_info.var_chunk_info[i].filter_mask<<endl);
                BESDEBUG("fonc", "chunk index: "<<i<<" chunk_direct_io_offset "<<dmrpp_vs_info.var_chunk_info[i].chunk_direct_io_offset<<endl);
                BESDEBUG("fonc", "chunk index: "<<i<<" chunk_buffer_size "<<dmrpp_vs_info.var_chunk_info[i].chunk_buffer_size<<endl);
                
                BESDEBUG("fonc", "chunk index: "<<i<<" coordinates are  "<<endl);
                for (unsigned int j = 0; j<dmrpp_vs_info.var_chunk_info[i].chunk_coords.size(); j++)
                    BESDEBUG("fonc", "coordinate index: "<<j<<" value "<<dmrpp_vs_info.var_chunk_info[i].chunk_coords[j]<<endl);
            }
        }

        BESDEBUG("fonc", "FONcArray::define() netcdf-4 version is " << d_ncVersion << endl);

#endif

        if (isNetCDF4()) {
            BESDEBUG("fonc", "FONcArray::define() Working netcdf-4 branch " << endl);

            if (d_io_flag) {
                // Use the direct chunk IO settings.
                define_dio_filters(ncid, d_varid);     
            }
            else {
                if (FONcRequestHandler::chunk_size == 0)
                    // I have no idea if chunksizes is needed in this case.
                    stax = nc_def_var_chunking(ncid, d_varid, NC_CONTIGUOUS, d_chunksizes.data());
                else
                    stax = nc_def_var_chunking(ncid, d_varid, NC_CHUNKED, d_chunksizes.data());
    
                if (stax != NC_NOERR) {
                    string err = "fileout.netcdf - Failed to define chunking for variable " + d_varname;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }
    
                // TODO Make this more adaptable to the Array's data type. Find out when it's
                //  best to use shuffle, et c. jhrg 7/22/18
                // The following code provides a way how to use shuffle. KY 11/2/23
                if (FONcRequestHandler::use_compression) {
    
                    int shuffle = 0;
                    // For integer, if the type size is >= 2, turn on the shuffle key always.
                    // For other types, turn off the shuffle key by default.
                    if (NC_SHORT == d_array_type || NC_USHORT == d_array_type || NC_INT == d_array_type ||
                        NC_UINT == d_array_type || NC_INT64 == d_array_type || NC_UINT64 == d_array_type ||
                        FONcRequestHandler::use_shuffle)                
                        shuffle = 1;
                    
                    int deflate = 1;
                    int deflate_level = 4;
                    stax = nc_def_var_deflate(ncid, d_varid, shuffle, deflate, deflate_level);
    
                    if (stax != NC_NOERR) {
                        string err = (string) "fileout.netcdf - Failed to define compression (deflate) level for variable "
                                     + d_varname;
                        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                    }
                }
            }
        }

        // Largely revised the fillvalue check code and add the check for the DAP4 case. KY 2021-05-10
        if (d_is_dap4) {
            D4Attributes *d4_attrs = d_a->attributes();
            updateD4AttrType(d4_attrs, d_array_type);
        }
        else {
            AttrTable &attrs = d_a->get_attr_table();
            updateAttrType(attrs, d_array_type);
        }

        BESDEBUG("fonc", "FONcArray::define() - Adding attributes " << endl);
        FONcAttributes::add_variable_attributes(ncid, d_varid, d_a, isNetCDF4_ENHANCED(), d_is_dap4);
        FONcAttributes::add_original_name(ncid, d_varid, d_varname, d_orig_varname);
        d_defined = true;
    }
    else {
        if (d_defined) {
            BESDEBUG("fonc", "FONcArray::define() - variable " << d_varname << " is already defined" << endl);
        }
        if (d_dont_use_it) {
            BESDEBUG("fonc", "FONcArray::define() - variable " << d_varname << " is not being used" << endl);
        }
    }

    BESDEBUG("fonc", "FONcArray::define() - done defining array '" << d_varname << "'" << endl);
}

/**
 * @brief Private method to reduce code duplication in FONcArray::write(int ncid).
 * @tparam T Write an array of this C++ type
 * @param ncid The ID of the open netCDF file.
 */
void FONcArray::write_nc_variable(int ncid, nc_type var_type) {

// To support direct IO, intern_data() is moved to the define mode, Comment out for the time being.
#if 0
    if (d_is_dap4)
        d_a->intern_data();
    else
        d_a->intern_data(*get_eval(), *get_dds());
#endif

    // Check if we can use direct IO.
    bool d_io_flag = d_a->get_dio_flag();

    if (d_io_flag) {
        // direct IO operation.
        write_direct_io_data(ncid,d_varid);
        return;
    }

    int stax;

    switch (var_type) {
        case NC_UBYTE:
            stax = nc_put_var_uchar(ncid, d_varid, reinterpret_cast<unsigned char *>(d_a->get_buf()));
            break;
        case NC_BYTE:
            stax = nc_put_var_schar(ncid, d_varid, reinterpret_cast<signed char *>(d_a->get_buf()));
            break;
        case NC_SHORT:
            stax = nc_put_var_short(ncid, d_varid, reinterpret_cast<short *>(d_a->get_buf()));
            break;
        case NC_INT:
            stax = nc_put_var_int(ncid, d_varid, reinterpret_cast<int *>(d_a->get_buf()));
            break;
        case NC_INT64:
            stax = nc_put_var_longlong(ncid, d_varid, reinterpret_cast<long long *>(d_a->get_buf()));
            break;
        case NC_FLOAT:
            stax = nc_put_var_float(ncid, d_varid, reinterpret_cast<float *>(d_a->get_buf()));
            break;
        case NC_DOUBLE:
            stax = nc_put_var_double(ncid, d_varid, reinterpret_cast<double *>(d_a->get_buf()));
            break;
        case NC_USHORT:
            stax = nc_put_var_ushort(ncid, d_varid, reinterpret_cast<unsigned short *>(d_a->get_buf()));
            break;
        case NC_UINT:
            stax = nc_put_var_uint(ncid, d_varid, reinterpret_cast<unsigned int *>(d_a->get_buf()));
            break;
        case NC_UINT64:
            stax = nc_put_var_ulonglong(ncid, d_varid, reinterpret_cast<unsigned long long *>(d_a->get_buf()));
            break;

        default:
            throw BESInternalError("Failed to transform array of unknown type in file out netcdf (1)",
                                         __FILE__, __LINE__);
    }

    if (stax != NC_NOERR) {
        string err = "fileout.netcdf - Failed to create array of " + d_a->var()->type_name() + " for " + d_varname;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }

    // This frees the local storage. jhrg 5/14/21
#if CLEAR_LOCAL_DATA
    if (!FONcGrid::InMaps(d_a))
        d_a->clear_local_data();
#endif
}

/**
 * @brief Are all of the strings the same length?
 *
 * This function also handles a special case when
 * the vector size is 0. If this happens,
 * the return value is false since no string is
 * compared. KY 2022-10-31
 *
 * @param the_array_strings
 * @return True if the strings are the same length
 */
bool FONcArray::equal_length(vector<string> &the_strings)
{
    if (the_strings.empty()==true)
        return false;
    else {
        size_t length = the_strings[0].size();
        if ( std::all_of(the_strings.begin()+1, the_strings.end(),
                         [length](string &s){return s.size() == length;}) )
            return true;
        else
            return false;
    }
}

/**
 * @brief Write the array out to the netcdf file
 *
 * Once the array is defined, the values of the array can be written out
 * as well.
 *
 * @param ncid The id of the netcdf file
 * @throws BESInternalError if there is a problem writing the values out
 * to the netcdf file
 */
void FONcArray::write(int ncid) {
    BESDEBUG("fonc", "FONcArray::write() BEGIN  var: " << d_varname << "[" << d_nelements << "]" << endl);
    BESDEBUG("fonc", "FONcArray::write() BEGIN  var type: " << d_array_type << " " << endl);

    if (d_dont_use_it) {
        BESDEBUG("fonc", "FONcTransform::write not using variable " << d_varname << endl);
        return;
    }

    // Writing out array is complex. There are three cases:
    // 1. Arrays of NC_CHAR, which are written the same for both the netCDF
    // classic and enhanced data models;
    // 2. All the other types, written for the enhanced data model
    // 3. All the other types, written for the classic data model

    if (d_array_type == NC_CHAR) {
        // Note that String data are not read here but in FONcArray::convert() because
        // that code needs to know that actual length of the individual strings in the
        // array. jhrg 6/4/21

        // More info: In FONcArray::convert() for arrays of NC_CHAR, even though the
        // libdap::Array variable has M dimension (e.g., 2) d_ndims will be M+1. The
        // additional dimension is there because each character is actually a string,
        // so the code needs to store both the character and a null terminator. This
        // might be a mistake in the data model - using String for CHAR might not be
        // the best plan. Right now, it's what we have. jhrg 10/3/22

        // Can we optimize for a special case where all strings are the same length?
        // jhrg 10/3/22
        if (equal_length(d_a->get_str())) {
#if STRING_ARRAY_OPT
            write_equal_length_string_array(ncid);
#else
            write_string_array(ncid);
#endif
        }
        else {
            write_string_array(ncid);
        }
    }
    else if (isNetCDF4_ENHANCED()) {
        // If we support the netCDF-4 enhanced model, the unsigned integer
        // can be directly mapped to the netcdf-4 unsigned integer.
        write_for_nc4_types(ncid);
    }
    else {
        write_for_nc3_types(ncid);
    }

    BESDEBUG("fonc", "FONcArray::write() END  var: " << d_varname << "[" << d_nelements << "]" << endl);
}

void FONcArray::write_for_nc3_types(int ncid) {
    Type element_type = d_a->var()->type();
    // create array to hold data hyperslab
    switch (d_array_type) {
        case NC_BYTE:
        case NC_FLOAT:
        case NC_DOUBLE:
            write_nc_variable(ncid, d_array_type);
            break;

        case NC_SHORT:
            // Given Byte/UInt8 will always be unsigned they must map
            // to a NetCDF type that will support unsigned bytes.  This
            // detects the original variable was of type Byte and typecasts
            // each data value to a short.
            if (element_type == dods_byte_c || element_type == dods_uint8_c) {
               // The data is retrieved in the define mode. So no need to do it here.
               // Comment out for the time being.
#if 0
                if (d_is_dap4)
                    d_a->intern_data();
                else
                    d_a->intern_data(*get_eval(), *get_dds());
#endif

                // There's no practical way to get rid of the value copy, be here we
                // read directly from libdap::Array object's memory.
                vector<short> data(d_nelements);
                for (size_t d_i = 0; d_i < d_nelements; d_i++)
                    data[d_i] = *(reinterpret_cast<unsigned char *>(d_a->get_buf()) + d_i);

                int stax = nc_put_var_short(ncid, d_varid, data.data());
                if (stax != NC_NOERR) {
                    string err = (string) "fileout.netcdf - Failed to create array of shorts for " + d_varname;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }

                // Once we've written an array, reclaim its space _unless_ it is a Grid map.
                // It might be shared and other code here expects it to be resident in memory.
                // jhrg 6/4/21
                if (!FONcGrid::InMaps(d_a))
                    d_a->clear_local_data();
            }
            else {
                write_nc_variable(ncid, NC_SHORT);
            }
            break;

        case NC_INT:
            // Added as a stop-gap measure to alert SAs and inform users of a misconfigured server.
            // jhrg 6/15/20
            if (element_type == dods_int64_c || element_type == dods_uint64_c) {
                // We should not be here. The server configuration is wrong since the netcdf classic
                // model is being used (either a netCDf3 response is requested OR a netCDF4 with the
                // classic model). Tell the user and the SA.
                string msg;
                if (FONcRequestHandler::classic_model == false) {
                    msg = "You asked for one or more 64-bit integer values returned using a netCDF3 file. "
                          "Try asking for netCDF4 enhanced and/or contact the server administrator.";
                }
                else {
                    msg = "You asked for one or more 64-bit integer values, but either returned using a netCDF3 file or "
                          "from a server that is configured to use the 'classic' netCDF data model with netCDF4. "
                          "Try netCDF4 and/or contact the server administrator.";
                }
                throw BESInternalError(msg, __FILE__, __LINE__);
            }

            if (element_type == dods_uint16_c) {

               // The data is retrieved in the define mode. So no need to do it here.
               // Comment out for the time being.
#if 0
                if (d_is_dap4)
                    d_a->intern_data();
                else
                    d_a->intern_data(*get_eval(), *get_dds());
#endif

                vector<int> data(d_nelements);
                for (size_t d_i = 0; d_i < d_nelements; d_i++)
                    data[d_i] = *(reinterpret_cast<unsigned short *>(d_a->get_buf()) + d_i);

                int stax = nc_put_var_int(ncid, d_varid, data.data());
                if (stax != NC_NOERR) {
                    string err = (string) "fileout.netcdf - Failed to create array of ints for " + d_varname;
                    FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
                }

                if (!FONcGrid::InMaps(d_a))
                    d_a->clear_local_data();
            }
            else {
                write_nc_variable(ncid, NC_INT);
            }
            break;

        default:
            throw BESInternalError("Failed to transform array of unknown type in file out netcdf (2)",
                                   __FILE__, __LINE__);
    }
}

/**
 * @note This may not work for 'ragged arrays' of strings.
 * @param ncid
 */
void FONcArray::write_string_array(int ncid) {
    vector<size_t> var_count(d_ndims);
    vector<size_t> var_start(d_ndims);
    int dim = 0;
    for (dim = 0; dim < d_ndims; dim++) {
        // the count for each of the dimensions will always be 1 except
        // for the string length dimension.
        // The size of the last dimension (var_count[d_ndims-1]) is set
        // separately for each element below. jhrg 10/3/22
        var_count[dim] = 1;

        // the start for each of the dimensions will start at 0. We will
        // bump this up in the while loop below
        var_start[dim] = 0;
    }

    auto const &d_a_str = d_a->get_str();
    for (size_t element = 0; element < d_nelements; element++) {
        var_count[d_ndims - 1] = d_a_str[element].size() + 1;
        var_start[d_ndims - 1] = 0;

        // write out the string
        int stax = nc_put_vara_text(ncid, d_varid, var_start.data(), var_count.data(),
                                    d_a_str[element].c_str());

        if (stax != NC_NOERR) {
            string err = (string) "fileout.netcdf - Failed to create array of strings for " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }

        // bump up the start.
        if (element + 1 < d_nelements) {
            bool done = false;
            dim = d_ndims - 2;
            while (!done) {
                var_start[dim] = var_start[dim] + 1;
                if (var_start[dim] == d_dim_sizes[dim]) {
                    var_start[dim] = 0;
                    dim--;
                }
                else {
                    done = true;
                }
            }
        }
    }

    d_a->get_str().clear();
}

/**
 * @brief Take an n-dim array of string and treat it as a n+1 dim array of string
 * Each string of the n+1 dim array is a single character. The strings
 * are C-style strings (null-terminated). The field d_ndims is already
 * set to n+1 when the server runs this method.
 * @param ncid
 */
void FONcArray::write_equal_length_string_array(int ncid) {
    vector<size_t> var_count(d_ndims);
    vector<size_t> var_start(d_ndims);
    // The flattened n-dim array as a vector of strings, row major order
    auto const &d_a_str = d_a->get_str();

    vector<char> text_data;
    text_data.reserve(d_a_str.size() * (d_a_str[0].size() + 1));
    for (auto &str: d_a_str) {
        for (auto c: str)
            text_data.emplace_back(c);
        text_data.emplace_back('\0');
    }

    for (int dim = 0; dim < d_ndims; dim++) {
        var_start[dim] = 0;
    }
    for (int dim = 0; dim < d_ndims; dim++) {
        var_count[dim] = d_dim_sizes[dim];
    }
    var_count[d_ndims - 1] = d_a_str[0].size() + 1;

    int stax = nc_put_vara_text(ncid, d_varid, var_start.data(), var_count.data(), text_data.data());

    if (stax != NC_NOERR) {
        string err = (string) "fileout.netcdf - Failed to create array of strings for " + d_varname;
        FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }

    d_a->get_str().clear();
}


/** @brief returns the name of the DAP Array
 *
 * @returns The name of the DAP Array
 */
string FONcArray::name() {
    return d_a->name();
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus instance data,
 * including private data for this instance, and dumps the list of
 * dimensions for this array.
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONcArray::dump(ostream &strm) const {
    strm << BESIndent::LMarg << "FONcArray::dump - (" << (void *) this << ")" << endl;
    BESIndent::Indent();
    strm << BESIndent::LMarg << "name = " << d_varname << endl;
    strm << BESIndent::LMarg << "ndims = " << d_ndims << endl;
    strm << BESIndent::LMarg << "actual ndims = " << d_actual_ndims << endl;
    strm << BESIndent::LMarg << "nelements = " << d_nelements << endl;
    if (d_dims.size()) {
        strm << BESIndent::LMarg << "dimensions:" << endl;
        BESIndent::Indent();
        vector<FONcDim *>::const_iterator i = d_dims.begin();
        vector<FONcDim *>::const_iterator e = d_dims.end();
        for (; i != e; i++) {
            (*i)->dump(strm);
        }
        BESIndent::UnIndent();
    }
    else {
        strm << BESIndent::LMarg << "dimensions: none" << endl;
    }
    BESIndent::UnIndent();
}

/** @brief write the data to netCDF-4 datatype
 *
 * The netCDF-4 supports unsigned 8-bit,16-bit,32-bit,64-bit integer
 * datatypes. So makes the exact mapping
 * when users specify the netCDF-4 enhanced output.
 * 
 *
 * @param ncid netCDF file ID 
 */
void FONcArray::write_for_nc4_types(int ncid) {

    d_is_dap4 = true;

    // create array to hold data hyperslab
    // DAP2 only supports unsigned BYTE. So here
    // we don't include NC_BYTE (the signed BYTE, the same
    // as 64-bit integer). KY 2020-03-20 
    // Actually 64-bit integer is supported.
    switch (d_array_type) {
        case NC_BYTE:
        case NC_UBYTE:
        case NC_SHORT:
        case NC_INT:
        case NC_INT64:
        case NC_FLOAT:
        case NC_DOUBLE:
        case NC_USHORT:
        case NC_UINT:
        case NC_UINT64:
            write_nc_variable(ncid, d_array_type);
            break;

        default:
            string err = (string) "Failed to transform array of unknown type in file out netcdf";
            throw BESInternalError(err, __FILE__, __LINE__);
    }
}

// Method to define filters of direct IO.
void FONcArray::define_dio_filters(int ncid, int d_varid) {

    Array::var_storage_info dmrpp_vs_info = d_a->get_var_storage_info();

    BESDEBUG("fonc", "filters: "<<dmrpp_vs_info.filter<<endl);

    string filters_string = dmrpp_vs_info.filter;
    
    bool has_fletcher_first = false;
    bool has_fletcher_last = false;
    bool has_shuffle = false;
    bool has_2deflates = false;
    bool has_1deflate = false;

    // Filter orders matter.
    obtain_dio_filters_order(filters_string, has_fletcher_first, has_fletcher_last, has_shuffle, has_2deflates, has_1deflate);

    int stax = 0;
    stax = nc_def_var_chunking_direct_write(ncid, d_varid, NC_CHUNKED, dmrpp_vs_info.chunk_dims.data());
    if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define direct_io chunking for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
    }

    // Allocate filters for the direct IO.
    allocate_dio_nc4_def_filters(ncid, d_varid, has_fletcher_first, has_fletcher_last, has_shuffle, has_2deflates, has_1deflate, dmrpp_vs_info.deflate_levels);


}

// Method that obtains the filter orders for the direct IO case.
void FONcArray::obtain_dio_filters_order(const string & filters, bool &has_fle_first, bool &has_fle_last, bool &has_shuffle, bool &has_2defs, bool &has_1def) {

    vector<string> filter_array = BESUtil::split(filters, ' ' );

    short num_defs = 0;
    for (unsigned int i = 0; i <filter_array.size(); i++) {
        if (filter_array[i] == "shuffle") 
            has_shuffle = true;
        else if (filter_array[i] == "deflate") 
            num_defs++;
        else if (filter_array[i] == "fletcher32") {
            if (i ==0)
                has_fle_first = true;
            else 
                has_fle_last = true;
        }
    }

    if (num_defs == 1)
        has_1def = true;
    else if (num_defs == 2)
        has_2defs = true;
    else if (num_defs >2)
        throw BESInternalError("Currently we don't support more than 2 deflate filters.", __FILE__, __LINE__);

}

// Method that allocate netCDF-4 filters for the direct IO case.
void FONcArray::allocate_dio_nc4_def_filters(int ncid, int d_varid, bool has_fle_first, bool has_fle_last, bool has_shuffle, 
                                         bool has_2defs, bool has_1def, const vector<unsigned int>& def_levs) {

    int stax = 0;

    if (has_1def && def_levs.size() !=1)
        throw BESInternalError("The size of def_levs should be 1 for one deflate filter", __FILE__, __LINE__);

    if (has_2defs && def_levs.size() !=2)
        throw BESInternalError("The size of def_levs should be 2 for two deflate filters", __FILE__, __LINE__);

    if (has_fle_first) {

        stax = nc_def_var_fletcher32(ncid, d_varid, 1);
        if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define the fletcher32 filter for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }

    if (has_shuffle) {
        if (has_1def) {
            stax = nc_def_var_deflate(ncid, d_varid, 1, 1, def_levs[0]);
            if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define the deflate and the shuffle filters for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
        else if (has_2defs) {
            stax = nc_def_var_two_deflates(ncid, d_varid, 1, 1, def_levs[0], def_levs[1]);
            if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define the two deflate filters and the shuffle filter for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
    }
    else {
        if (has_1def) {
            stax = nc_def_var_deflate(ncid, d_varid, 0, 1, def_levs[0]);
            if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define the deflate filter for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
        else if (has_2defs) {
            stax = nc_def_var_two_deflates(ncid, d_varid, 0, 1, def_levs[0], def_levs[1]);
            if (stax != NC_NOERR) {
                string err = "fileout.netcdf - Failed to define the two deflate filters for variable " + d_varname;
                FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
            }
        }
    }

    if (has_fle_last) {
        stax = nc_def_var_fletcher32(ncid, d_varid,1);   
        if (stax != NC_NOERR) {
            string err = "fileout.netcdf - Failed to define the filter fletcher32 for variable " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);
        }
    }

}

// Write data for the direct IO case.
void FONcArray::write_direct_io_data(int ncid, int d_varid) {

    char dummy_buffer[1];

    // The following call doesn't write any data but set up the necessary operations for sending data directly.
    int stax = nc_put_var(ncid, d_varid, dummy_buffer);
    if (stax != NC_NOERR) {
        string err = "fileout.netcdf - the direct IO version of nc_put_var error for variable " + d_varname;
        FONcUtils::handle_error(stax , err, __FILE__, __LINE__);
    }

    Array::var_storage_info dmrpp_vs_info = d_a->get_var_storage_info();

    for (unsigned int i = 0; i<dmrpp_vs_info.var_chunk_info.size(); i++) {

        Array::var_chunk_info_t vci = dmrpp_vs_info.var_chunk_info[i];

        // May use the vector to replace new[] later. 
        char *chunk_buf = new char[vci.chunk_buffer_size];
        memcpy (chunk_buf,d_a->get_buf()+vci.chunk_direct_io_offset,vci.chunk_buffer_size);

        stax = nc4_write_chunk(ncid, d_varid, vci.filter_mask, vci.chunk_coords.size(), (const size_t *)(vci.chunk_coords.data()),vci.chunk_buffer_size, chunk_buf);
        if (stax != NC_NOERR) {
            string err = "fileout.netcdf - nc4_write_chunk error for variable " + d_varname;
            FONcUtils::handle_error(stax, err, __FILE__, __LINE__);

        }

        delete[] chunk_buf;
    }
 
}


