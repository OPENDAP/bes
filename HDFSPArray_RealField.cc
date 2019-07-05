/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the real SDS field values for special HDF4 data products.
//  Authors:   MuQun Yang <myang6@hdfgroup.org>  Eunsoo Seo
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#include "HDFSPArray_RealField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "BESInternalError.h"
#include "HDFCFUtil.h"
#include "HDF4RequestHandler.h"

//#include "BESH4MCache.h"
#include "dodsutil.h"

using namespace std;
using namespace libdap;
#define SIGNED_BYTE_TO_INT32 1


bool
HDFSPArray_RealField::read ()
{

    BESDEBUG("h4","Coming to HDFSPArray_RealField read "<<endl);
    if(length() == 0)                                                                               
        return true; 
    
    // Declare offset, count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint (&offset[0], &step[0], &count[0]);

    // Cache
    // Check if a BES key H4.EnableDataCacheFile is true, if yes, we will check
    // if we can read the data from this file.
#if 0
    string check_data_cache_key = "H4.EnableDataCacheFile";
    bool enable_check_data_cache_key = false;
    enable_check_data_cache_key = HDFCFUtil::check_beskeys(check_data_cache_key);
#endif

    bool data_from_cache = false;
    bool data_to_cache = false;

    short dtype_size = HDFCFUtil::obtain_type_size(dtype);
    if(-1 == dtype_size) {
        string err_mesg = "Wrong data type size for the variable ";
        err_mesg += name();
        throw InternalErr(__FILE__,__LINE__,err_mesg);
    }

    string cache_fpath;
    //if(true == enable_check_data_cache_key) {
    if(true == HDF4RequestHandler::get_enable_data_cachefile()) {

        BESH4Cache *llcache = BESH4Cache::get_instance();

        // Here we have a sanity check for the cached parameters:Cached directory,file prefix and cached directory size.
        // Supposedly Hyrax BES cache feature should check this and the code exists. However, the
        // current hyrax 1.9.7 doesn't provide this feature. KY 2014-10-24
#if 0
        string bescachedir= llcache->getCacheDirFromConfig();
        string bescacheprefix = llcache->getCachePrefixFromConfig();
        unsigned int cachesize = llcache->getCacheSizeFromConfig();
#endif

        // Make it simple, the data cache parameters also shares with the HDF-EOS2 grid lat/lon cache
        string bescachedir = HDF4RequestHandler::get_cache_latlon_path();
        string bescacheprefix = HDF4RequestHandler::get_cache_latlon_prefix();
        long cachesize = HDF4RequestHandler::get_cache_latlon_size();

        if(("" == bescachedir)||(""==bescacheprefix)||(cachesize <=0))
            throw InternalErr (__FILE__, __LINE__, "Either the cached dir is empty or the prefix is NULL or the cache size is not set.");
        else {
            struct stat sb;
            if(stat(bescachedir.c_str(),&sb) !=0) {
                string err_mesg="The cached directory " + bescachedir;
                err_mesg = err_mesg + " doesn't exist.  ";
                throw InternalErr(__FILE__,__LINE__,err_mesg);
            }
            else { 
                if(true == S_ISDIR(sb.st_mode)) {
                        if(access(bescachedir.c_str(),R_OK|W_OK|X_OK) == -1) {
                            string err_mesg="The cached directory " + bescachedir;
                            err_mesg = err_mesg + " can NOT be read,written or executable.";
                            throw InternalErr(__FILE__,__LINE__,err_mesg);
                        }
                }
                else {
                        string err_mesg="The cached directory " + bescachedir;
                        err_mesg = err_mesg + " is not a directory.";
                        throw InternalErr(__FILE__,__LINE__,err_mesg);
                }
            }
        }

        //string cache_fname=HDFCFUtil::obtain_cache_fname(llcache->getCachePrefixFromConfig(),filename,name());
        string cache_fname=HDFCFUtil::obtain_cache_fname(bescacheprefix,filename,name());
        cache_fpath = bescachedir + "/"+ cache_fname;
        
        int total_elems = 1;
        for (unsigned int i = 0; i <dimsizes.size();i++)
            total_elems = total_elems*dimsizes[i];
        dtype_size = HDFCFUtil::obtain_type_size(dtype);
        if(-1 == dtype_size) {
            string err_mesg = "Wrong data type size for the variable ";
            err_mesg += name();
            throw InternalErr(__FILE__,__LINE__,err_mesg);
        }
        int expected_file_size = dtype_size *total_elems;
        int fd = 0;
        if( true == llcache->get_data_from_cache(cache_fpath, expected_file_size,fd)) {

            vector<int32>offset32;
            offset32.resize(offset.size());
            for (int i = 0; i< rank;i++)
                offset32[i] = offset[i];
            int offset_1st = INDEX_nD_TO_1D(dimsizes,offset32);          
            vector<int32>end;
            end.resize(rank);
            for (int i = 0; i < rank; i++)
                end[i] = offset[i] +(count[i]-1)*step[i];
            int offset_last = INDEX_nD_TO_1D(dimsizes,end);
//cerr<<"offset_1d is "<<offset_1st <<endl;
//cerr<<"offset_last is "<<offset_last <<endl;
            size_t total_read = dtype_size*(offset_last-offset_1st+1);
            
            off_t fpos = lseek(fd,dtype_size*offset_1st,SEEK_SET);
            if (-1 == fpos) {
                llcache->unlock_and_close(cache_fpath);
                llcache->purge_file(cache_fpath);
            }
 
            /// Read the data from the cache
            else 
                data_from_cache = obtain_cached_data(llcache,cache_fpath,fd, step,count,total_read,dtype_size);

        }

        if(true == data_from_cache)
            return true;
        else 
            data_to_cache = true;

     }

#if 0
    string check_pass_fileid_key_str="H4.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDFCFUtil::check_beskeys(check_pass_fileid_key_str);
#endif
    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Just declare offset,count and step in the int32 type.
    vector<int32>offset32;
    offset32.resize(rank);
    vector<int32>count32;
    count32.resize(rank);
    vector<int32>step32;
    step32.resize(rank);

    // Just obtain the offset,count and step in the datatype of int32.
    for (int i = 0; i < rank; i++) {
        offset32[i] = (int32) offset[i];
        count32[i] = (int32) count[i];
        step32[i] = (int32) step[i];
    }

    int32 sdid = -1;

    // Obtain SD ID.
    if(false == check_pass_fileid_key) {
        sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (sdid < 0) {
            ostringstream eherr;
            eherr << "File " << filename.c_str () << " cannot be open.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }
    else {
        // This code will make sure that the file ID is not passed when H4.EnableMetaDataCacheFile key is set.
        // We will wait to see if Hyrax core supports the cache and then make improvement. KY 2015-04-30
#if 0
        string check_enable_mcache_key="H4.EnableMetaDataCacheFile";
        bool turn_on_enable_mcache_key= false;
        turn_on_enable_mcache_key = HDFCFUtil::check_beskeys(check_enable_mcache_key);
#endif

        //if(true == turn_on_enable_mcache_key) {
        if(true == HDF4RequestHandler::get_enable_metadata_cachefile()) {

            sdid = SDstart (const_cast < char *>(filename.c_str ()), DFACC_READ);
            if (sdid < 0) {
                ostringstream eherr;
                eherr << "File " << filename.c_str () << " cannot be open.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }
            // Reset the pass file ID key to be false for resource clean-up.
            check_pass_fileid_key = false;

        }
        else 
            sdid = sdfd;
    }

    // Initialize  SDS ID. 
    int32 sdsid = 0;

    // Obtain the SDS index based on the input sds reference number.
    int32 sdsindex = SDreftoindex (sdid, (int32) fieldref);
    if (sdsindex == -1) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "SDS index " << sdsindex << " is not right.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Obtain this SDS ID.
    sdsid = SDselect (sdid, sdsindex);
    if (sdsid < 0) {
        HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "SDselect failed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Initialize the temp. returned value.
    int32 r = 0;

    // Loop through all the possible SDS datatype and then read the data.
    switch (dtype) {

        case DFNT_INT8:
        {
            vector<char>buf;
            buf.resize(nelms);
            r = SDreaddata (sdsid, &offset32[0], &step32[0], &count32[0], &buf[0]);
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "SDreaddata failed.";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

#ifndef SIGNED_BYTE_TO_INT32
            val2buf(&buf[0]);
            set_read_p(true);
#else
            vector<int32>newval;
            newval.resize(nelms);
            for (int counter = 0; counter < nelms; counter++)
                newval[counter] = (int32) (buf[counter]);

            set_value ((dods_int32 *) &newval[0], nelms);
#endif
             if(true == data_to_cache) {
                try {
                    write_data_to_cache(sdsid,cache_fpath,dtype_size,buf,nelms);
                }
                catch(...) {
                    SDendaccess (sdsid);
                    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                    ostringstream eherr;
                    eherr << "write data to cache failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
                }
            }
        }
            break;

        // Will keep the following comments until we formally decide not to support these old products.
        // --------------------------------------------------------
        // HANDLE this later if necessary. KY 2010-8-17
        //if(sptype == TRMML2_V6) 
        // Find scale_factor, add_offset attributes
        // create a new val to float32, remember to change int16 at HDFSP.cc to float32
        // if(sptype == OBPGL3) 16-bit unsigned integer 
        // Find slope and intercept, 
        // CZCS: also find base. data = base**((slope*l3m_data)+intercept)
        // Others : slope*data+intercept
        // OBPGL2: 16-bit signed integer, (8-bit signed integer or 32-bit signed integer)
        // If slope = 1 and intercept = 0 no need to do conversion
        // ---------------------------------------------------------

        case DFNT_UINT8:
        case DFNT_UCHAR8:
        case DFNT_INT16:
        case DFNT_UINT16:
        case DFNT_INT32:
        case DFNT_UINT32:
        case DFNT_FLOAT32:
        case DFNT_FLOAT64:
        {
            vector<char>buf;
            buf.resize(nelms*dtype_size);

            r = SDreaddata (sdsid, &offset32[0], &step32[0], &count32[0], &buf[0]);
            if (r != 0) {
                SDendaccess (sdsid);
                HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                ostringstream eherr;
                eherr << "SDreaddata failed";
                throw InternalErr (__FILE__, __LINE__, eherr.str ());
            }

            val2buf(&buf[0]);
            set_read_p(true);

            // write data to cache if cache is set.
            if(true == data_to_cache) {
                try {
                    write_data_to_cache(sdsid,cache_fpath,dtype_size,buf,nelms);
                }
                catch(...) {
                    SDendaccess (sdsid);
                    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
                    ostringstream eherr;
                    eherr << "write data to cache failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
 
                }
            }
        }
            break;
        default:
            SDendaccess (sdsid);
            HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);
            throw InternalErr (__FILE__, __LINE__, "unsupported data type.");
    }

    // Close the SDS interface
    r = SDendaccess (sdsid);
    if (r != 0) {
        ostringstream eherr;
        eherr << "SDendaccess failed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
    HDFCFUtil::close_fileid(sdid,-1,-1,-1,check_pass_fileid_key);

#if 0
    // Close the SD interface
    r = SDend (sdid);
    if (r != 0) {
        ostringstream eherr;
        eherr << "SDend failed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
#endif

    return true;
}

bool HDFSPArray_RealField::obtain_cached_data(BESH4Cache *llcache,const string & cache_fpath, int fd,vector<int> &cd_step, vector<int>&cd_count,size_t total_read,short dtype_size) {

    ssize_t ret_read_val = -1;
    vector<char>buf;
    //char* buf;
    //buf = malloc(total_read);
    buf.resize(total_read);
    ret_read_val = HDFCFUtil::read_buffer_from_file(fd,(void*)&buf[0],total_read);
    llcache->unlock_and_close(cache_fpath);
    if((-1 == ret_read_val) || (ret_read_val != (ssize_t)total_read)) {
        llcache->purge_file(cache_fpath);
        return false;
    }
    else {    
        unsigned int nele_to_read = 1;
        for(int i = 0; i<rank;i++) 
            nele_to_read *=cd_count[i];

        if(nele_to_read == (total_read/dtype_size)) {
//cerr<<"use the buffer" <<endl;
            val2buf(&buf[0]);
            set_read_p(true);
        }
        else { //  Need to re-assemble the buffer according to different datatype

            vector<int>cd_start(rank,0);
            vector<int32>cd_pos(rank,0);
            int nelms_to_send = 1;
            for(int i = 0; i <rank; i++)
                nelms_to_send = nelms_to_send*cd_count[i];
            switch (dtype) {

                case DFNT_INT8:
                {

#ifndef SIGNED_BYTE_TO_INT32
                     vector<int8>total_val;
                     total_val.resize(total_read/dtype_size);
                     memcpy(&total_val[0],(void*)&buf[0],total_read);

                     vector<int8>final_val;
                     subset<int8>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                     set_value((dods_byte*)&final_val[0],nelms_to_send);

#else
                     vector<int32>total_val2;
                     total_val2.resize(total_read/dtype_size);
                     memcpy(&total_val2[0],(void*)&buf[0],total_read);

                     vector<int32>final_val2;
                     subset<int32>(
                                      &total_val2[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val2,
                                      cd_pos,
                                      0
                                     );

                     set_value((dods_int32*)&final_val2[0],nelms_to_send);

#endif
                }

                    break;
                case DFNT_UINT8:
                case DFNT_UCHAR8:
                {
                    vector<uint8>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<uint8>final_val;
                    subset<uint8>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_byte *) &final_val[0], nelms_to_send);
                }
                    break;

                case DFNT_INT16:
                {
                    vector<int16>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<int16>final_val;
                    subset<int16>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_int16 *) &final_val[0], nelms_to_send);
                }
                    break;

                case DFNT_UINT16:
                {
                    vector<uint16>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<uint16>final_val;
                    subset<uint16>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_uint16 *) &final_val[0], nelms_to_send);
                }
                    break;

                case DFNT_INT32:
                {
                    vector<int32>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<int32>final_val;
                    subset<int32>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_int32 *) &final_val[0], nelms_to_send);
                }
                    break;

                case DFNT_UINT32:
                {
                    vector<uint32>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<uint32>final_val;
                    subset<uint32>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_uint32 *) &final_val[0], nelms_to_send);
                }
                    break;

                case DFNT_FLOAT32:
                {
                    vector<float32>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<float32>final_val;
                    subset<float32>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );


                    set_value ((dods_float32 *) &final_val[0], nelms_to_send);
                }
                    break;
                case DFNT_FLOAT64:
                {
                    vector<float64>total_val;
                    total_val.resize(total_read/dtype_size);
                    memcpy(&total_val[0],(void*)&buf[0],total_read);

                    vector<float64>final_val;
                    subset<float64>(
                                      &total_val[0],
                                      rank,
                                      dimsizes,
                                      cd_start,
                                      cd_step,
                                      cd_count,
                                      &final_val,
                                      cd_pos,
                                      0
                                     );

                    set_value ((dods_float64 *) &final_val[0], nelms_to_send);
                }
                    break;
                default:
                    throw InternalErr (__FILE__, __LINE__, "unsupported data type.");

            }// end switch(dtype)
        }// end else (stride is not 1)
        return true;
    }// end else(full_read = true)
}

void 
HDFSPArray_RealField::write_data_to_cache(int32 sdsid, const string& cache_fpath,short dtype_size,const vector<char> &buf, int nelms) {

    BESH4Cache *llcache = BESH4Cache::get_instance();
    vector<int32>woffset32(rank,0);
    vector<int32>wstep32(rank,1);
    vector<int32>wcount32;
    wcount32.resize(rank);
    int total_nelem = 1;
    for(int i = 0; i <rank; i++){
        wcount32[i] = (int32)dimsizes[i];
        total_nelem = total_nelem*dimsizes[i];
    }
    vector<char>val;
    if(DFNT_INT8 == dtype) {

#ifndef SIGNED_BYTE_TO_INT32
        if(total_nelem == nelms) 
            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&buf[0]);
        else {
            val.resize(dtype_size*total_nelem);
            if(SDreaddata (sdsid, &woffset32[0], &wstep32[0], &wcount32[0], &val[0])<0) 
                throw InternalErr (__FILE__, __LINE__, "Cannot read the whole SDS for cache.");
            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&val[0]);
        }
 
#else 
        vector<int>newval;
        newval.resize(total_nelem);
        if(total_nelem == nelms) {
            for (int i = 0; i < total_nelem;i++)
                newval[i] = (int)buf[i];
            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&newval[0]);
        }
        else {
            vector<char>val2;
            val2.resize(total_nelem);
            if(SDreaddata (sdsid, &woffset32[0], &wstep32[0], &wcount32[0], &val2[0])<0)
                throw InternalErr (__FILE__, __LINE__, "Cannot read the whole SDS for cache.");
            for (int i = 0; i < total_nelem;i++)
                newval[i] = (int)val2[i];
            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&newval[0]);
       }
#endif
    }
    else {
        if(total_nelem == nelms) {
            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&buf[0]);
        }
        else {
            val.resize(dtype_size*total_nelem);
            if(SDreaddata (sdsid, &woffset32[0], &wstep32[0], &wcount32[0], &val[0])<0) 
                throw InternalErr (__FILE__, __LINE__, "Cannot read the whole SDS for cache.");

            llcache->write_cached_data2(cache_fpath,dtype_size*total_nelem,(const void*)&val[0]);
        }
    }
}
// Standard way to pass the coordinates of the subsetted region from the client to the handlers
// Returns the number of elements 
int
HDFSPArray_RealField::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();
    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);

        // Check for illegal  constraint
        if (start > stop) {
            ostringstream oss;
            oss << "Array/Grid hyperslab start point "<< start <<
                   " is greater than stop point " <<  stop <<".";
            throw Error(malformed_expr, oss.str());
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1;      // count of elements
        nels *= count[id];              // total number of values for variable

        BESDEBUG ("h4",
                         "=format_constraint():"
                         << "id=" << id << " offset=" << offset[id]
                         << " step=" << step[id]
                         << " count=" << count[id]
                         << endl);

        id++;
        p++;
    }// while (p != dim_end ())

    return nels;
}


//! Getting a subset of a variable
//
//      \param input Input variable
//       \param dim dimension info of the input
//       \param start start indexes of each dim
//       \param stride stride of each dim
//       \param edge count of each dim
//       \param poutput output variable
//      \parrm index dimension index
//       \return 0 if successful. -1 otherwise.
//
template<typename T> int
HDFSPArray_RealField::subset(
    const T input[],
    int sf_rank,
    vector<int32> & dim,
    vector<int> & start,
    vector<int> & stride,
    vector<int> & edge,
    std::vector<T> *poutput,
    vector<int32>& pos,
    int index)
{
    for(int k=0; k<edge[index]; k++)
    {
        pos[index] = start[index] + k*stride[index];
        if(index+1<sf_rank)
            subset(input, sf_rank, dim, start, stride, edge, poutput,pos,index+1);
        if(index==sf_rank-1)
        {
            poutput->push_back(input[INDEX_nD_TO_1D( dim, pos)]);
        }
    } // end of for
    return 0;
} // end of template<typename T> static int subset
