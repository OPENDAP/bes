/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps,clear memories and separating namelist
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////
#ifndef HDFCFUTIL_H
#define HDFCFUTIL_H
#include <stdlib.h>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <assert.h>
#include <iomanip>

#include "mfhdf.h"
#include "hdf.h"

#include <TheBESKeys.h>
#include <BESUtil.h>
#include "escaping.h" // for escattr

#define MAX_NON_SCALE_SPECIAL_VALUE 65535
#define MIN_NON_SCALE_SPECIAL_VALUE 65500

using namespace std;
using namespace libdap;

struct dimmap_entry
{
        string geodim;
        string datadim;
        int32 offset, inc;
};


struct HDFCFUtil
{

        /// Check the BES key. 
        static bool check_beskeys(const string key);

        /// From a string separated by a separator to a list of string,
        /// for example, split "ab,c" to {"ab","c"}
        static void Split (const char *s, int len, char sep,
                                std::vector < std::string > &names);

        /// Assume sz is Null terminated string.
        static void Split (const char *sz, char sep,
                            std::vector < std::string > &names);

        /// Clear memory
	static void ClearMem (int32 * offset32, int32 * count32, int32 * step32,
						  int *offset, int *count, int *step);
	static void ClearMem2 (int32 * offset32, int32 * count32, int32 * step32);
	static void ClearMem3 (int *offset, int *count, int *step);

        /// This is a safer way to insert and update map item than map[key]=val style string assignment.
        /// Otherwise, the local testsuite at The HDF Group will fail for HDF-EOS2 data
        ///  under iMac machine platform.
        static bool insert_map(std::map<std::string,std::string>& m, string key, string val);

       /// Change special characters to "_"
       static string get_CF_string(string s);

       /// Obtain the unique name for the clashed names and save it to set namelist.
       static void gen_unique_name(string &str,set<string>& namelist, int&clash_index);

       static void Handle_NameClashing(vector<string>&newobjnamelist);
       static void Handle_NameClashing(vector<string>&newobjnamelist,set<string>&objnameset);

       static string print_attr(int32, int, void*);
       static string print_type(int32);

       static bool is_special_value(int32 dtype,float fillvalue, float realvalue);

        // Subsetting the 2-D fields
       template <typename T> static void LatLon2DSubset (T* outlatlon, int ydim, int xdim,
                                                 T* latlon, int32 * offset, int32 * count,
                                                 int32 * step);

};

inline int32
INDEX_nD_TO_1D (const std::vector < int32 > &dims,
                                const std::vector < int32 > &pos)
{
        /*
           int a[10][20][30];  // & a[1][2][3] == a + (20*30+1 + 30*2 + 1 *3);
           int b[10][2]; // &b[1][2] == b + (20*1 + 2);
         */
        assert (dims.size () == pos.size ());
        int32 sum = 0;
        int32 start = 1;

        for (unsigned int p = 0; p < pos.size (); p++) {
                int32 m = 1;

                for (unsigned int j = start; j < dims.size (); j++)
                        m *= dims[j];
                sum += m * pos[p];
                start++;
        }
        return sum;
}


#endif
