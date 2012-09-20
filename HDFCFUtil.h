/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps,clear memories and separating namelist
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) 2010 The HDF Group
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

// Subset of latitude and longitude to follow the parameters from the DAP expression constraint
#if 0
template < class T >
void HDFCFUtil::LatLon2DSubset (T * outlatlon,
                                                                                                                  int majordim,
                                                                                                                  int minordim,
                                                                                                                  T * latlon,
                                                                                                                  int32 * offset,
                                                                                                                  int32 * count,
                                                                                                                  int32 * step)
{

//          float64 templatlon[majordim][minordim];
        T (*templatlonptr)[majordim][minordim] = (typeof templatlonptr) latlon;
     int i, j, k;

#if 0
    for (int jjj = 0; jjj < majordim; jjj++)
        for (int kkk = 0; kkk < minordim; kkk++)
            cerr << "templatlon " << jjj << " " << kkk << " " <<
                templatlon[i][j] << endl;
#endif
    // do subsetting
    // Find the correct index
    int dim0count = count[0];
    int dim1count = count[1];
    int dim0index[dim0count], dim1index[dim1count];

    for (i = 0; i < count[0]; i++)      // count[0] is the least changing dimension 
        dim0index[i] = offset[0] + i * step[0];


    for (j = 0; j < count[1]; j++)
        dim1index[j] = offset[1] + j * step[1];

    // Now assign the subsetting data
    k = 0;

    for (i = 0; i < count[0]; i++) {
        for (j = 0; j < count[1]; j++) {
            outlatlon[k] = (*templatlonptr)[dim0index[i]][dim1index[j]];
            k++;

        }
    }

}
#endif

#endif
