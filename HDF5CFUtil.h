// This file is part of the hdf5_handler implementing for the CF-compliant
// Copyright (c) 2011-2016 The HDF Group, Inc. and OPeNDAP, Inc.
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
// You can contact The HDF Group, Inc. at 1800 South Oak Street,
// Suite 203, Champaign, IL 61820  

/////////////////////////////////////////////////////////////////////////////
/// \file HDF5CFUtil.h
/// \brief This file includes several helper functions for translating HDF5 to CF-compliant
///
/// \author Kent Yang <myang6@hdfgroup.org>
///
/// Copyright (C) 2011-2016 The HDF Group
///
/// All rights reserved.

#ifndef _HDF5CFUtil_H
#define _HDF5CFUtil_H
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <set>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>
#include <cerrno>
#include "hdf5.h"
#include "HE5Grid.h"

// We create this intermediate enum H5DataType in order to totally separate the 
// creating of DAS and DDS from any HDF5 API calls. When mapping to DAP, only this
// intermediate H5DataType will be used to map to the corresponding DAP datatype.
// Here H5UNSUPTYPE includes H5T_TIME, H5T_BITFIELD, H5T_OPAQUE,H5T_ENUM, H5T_VLEN.
// For CF option, H5REFERENCE, H5COMPOUND, H5ARRAY will not be supported. We leave them
// here for future merging of default option and CF option. Currently DAP2 doesn't 
// support 64-bit integer. We still list int64 bit types since we find ACOSL2S or OCO2L1B have this
// datatype and we need to provide a special mapping for this datatype. 
// H5UCHAR also needs a special mapping. Similiarly other unsupported types may need to 
// have special mappings in the future. So the following enum type may be extended 
// according to the future need. The idea is that all the necessary special mappings should
// be handled in the HDF5CF name space. 
// The DDS and DAS generation modules should not use any HDF5 APIs.
enum H5DataType
{H5FSTRING, H5FLOAT32,H5CHAR,H5UCHAR,H5INT16,H5UINT16,
 H5INT32,H5UINT32,H5INT64,H5UINT64,H5FLOAT64,H5VSTRING,
 H5REFERENCE,H5COMPOUND,H5ARRAY,H5UNSUPTYPE};

enum CVType { CV_EXIST,CV_LAT_MISS,CV_LON_MISS,CV_NONLATLON_MISS,CV_FILLINDEX,CV_MODIFY,CV_SPECIAL,CV_UNSUPPORTED};

// Angle Conversion Codes 
#define HE5_HDFE_RAD_DEG      0
#define HE5_HDFE_DEG_RAD      1
#define HE5_HDFE_DMS_DEG      2
#define HE5_HDFE_DEG_DMS      3
#define HE5_HDFE_RAD_DMS      4
#define HE5_HDFE_DMS_RAD      5


using namespace std;

struct Name_Size_2Pairs {
    std::string name1;
    std::string name2;
    hsize_t size1;
    hsize_t size2;
    int  rank;
};

struct HDF5CFUtil {

               static bool use_data_mem_cache(H5DataType h5type,CVType cvtype,const string & varpath);

               static size_t H5_numeric_atomic_type_size(H5DataType h5type);
               /// Map HDF5 Datatype to the intermediate H5DAPtype for the future use.
               static H5DataType H5type_to_H5DAPtype(hid_t h5_type_id);

               /// Trim the string with many NULL terms or garbage characters to simply a string
               /// with a NULL terminator. This method will not handle the NULL PAD case.
               static std::string trim_string(hid_t dtypeid,const std::string s, int num_sect, size_t section_size, std::vector<size_t>& sect_newsize);

               static std::string obtain_string_after_lastslash(const std::string s);
               static std::string obtain_string_before_lastslash(const std::string & s);
               static std::string remove_substrings(std::string str, const std::string &s);
               static bool cf_strict_support_type(H5DataType dtype); 
               static bool cf_dap2_support_numeric_type(H5DataType dtype); 

               // Obtain the unique name for the clashed names and save it to set namelist.
               static void gen_unique_name(std::string &str, std::set<std::string>&namelist,int&clash_index);

               /// From a string separated by a separator to a list of string,
               /// for example, split "ab,c" to {"ab","c"}
               static void Split (const char *s, int len, char sep,
                                  std::vector < std::string > &names);

               /// Assume sz is Null terminated string.
               static void Split (const char *sz, char sep,
                            std::vector < std::string > &names);

               static void Split_helper(vector<string>&tokens, const string &text,const char sep);

               // Parse GPM Level 3 GridHeaders
               //static void parser_trmm_v7_gridheader(int& latsize, int&lonsize, float& lat_start, float& lon_start, bool &sw_origin, bool & cr_reg);
               static void parser_gpm_l3_gridheader(const std:: vector<char>&value, int& latsize, int&lonsize, 
                                                    float& lat_start, float& lon_start, float& lat_res, float& lon_res, bool check_reg_orig);

               static void close_fileid(hid_t,bool);

               // wrap function of Unix read to a buffer. Memory for the buffer should be allocated.
               static ssize_t read_buffer_from_file(int fd,void*buf,size_t);
               static std::string obtain_cache_fname(const std::string & fprefix, const std::string & fname, const std::string &vname);
               
               static int GDij2ll(int projcode, int zonecode, double projparm[],
        int spherecode, int xdimsize, int ydimsize,
        double upleftpt[], double lowrightpt[],
        int npnts, int row[], int col[],
        double longitude[], double latitude[], int pixcen, int pixcnr);

               //static size_t INDEX_nD_TO_1D (const std::vector < size_t > &dims,
               //                           const std::vector < size_t > &pos);

#if 0
               template<typename T>  int subset(
                                                const T input[],
                                                int rank,
                                                vector<int> & dim,
                                                int start[],
                                                int stride[],
                                                int edge[],
                                                std::vector<T> *poutput,
                                                vector<int>& pos,
                                                int index);
#endif
} ;

static inline struct flock *lock(int type) {
    static struct flock lock;
    lock.l_type = type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return &lock;
}

static inline string get_errno() {
        char *s_err = strerror(errno);
        if (s_err)
                return s_err;
        else
                return "Unknown error.";
}


//size_t INDEX_nD_TO_1D (const std::vector < size_t > &dims,
 //                                const std::vector < size_t > &pos);

#if 0
{
    //
    //  int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
    //  int b[10][2]; // &b[1][2] == b + (20*1 + 2);
    // 
    if(dims.size () != pos.size ())
        throw InternalErr(__FILE__,__LINE__,"dimension error in INDEX_nD_TO_1D routine.");       
    size_t sum = 0;
    size_t  start = 1;

    for (size_t p = 0; p < pos.size (); p++) {
        size_t m = 1;

        for (size_t j = start; j < dims.size (); j++)
            m *= dims[j];
        sum += m * pos[p];
        start++;
    }
    return sum;
}
#endif
        int GDij2ll(int projcode, int zonecode, double projparm[],
        int spherecode, int xdimsize, int ydimsize,
        double upleftpt[], double lowrightpt[],
        int npnts, int row[], int col[],
        double longitude[], double latitude[], EOS5GridPRType pixcen, EOS5GridOriginType pixcnr);

//extern int inv_init(int insys, int inzone, double *inparm, int indatum, char *fn27, char *fn83, int *iflg, int (*inv_trans[])(double, double, double*, double*));

//extern int for_init(int outsys, int outzone, double *outparm, int outdatum, char *fn27, char *fn83, int *iflg, int (*for_trans[])(double, double, double *, double *));
       double HE5_EHconvAng(double inAngle, int code);
#endif
