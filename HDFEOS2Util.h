/////////////////////////////////////////////////////////////////////////////
// Helper functions for handling dimension maps and clear memories
//
//  Authors:   MuQun Yang <myang6@hdfgroup.org> Eunsoo Seo
// Copyright (c) 2010 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifndef HDFEOS2UTIL_H
#define HDFEOS2UTIL_H
#include <stdlib.h>
#include <string>
#include "mfhdf.h"
#include "hdf.h"
#include "HdfEosDef.h"
#include <vector>
#include <assert.h>
using namespace std;

struct dimmap_entry
{
	std::string geodim;
	std::string datadim;
	int32 offset, inc;
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


struct HDFEOS2Util
{
	static void ClearMem (int32 * offset32, int32 * count32, int32 * step32,
						  int *offset, int *count, int *step);
};
#endif
