#ifndef HDFSPUTIL_H
#define HDFSPUTIL_H
#include <stdlib.h>
#include <string>
#include "mfhdf.h"
#include "hdf.h"
#include <vector>
#include <assert.h>
using namespace std;


struct HDFSPUtil
{
	static void ClearMem (int32 * offset32, int32 * count32, int32 * step32,
						  int *offset, int *count, int *step);
	static void ClearMem2 (int32 * offset32, int32 * count32, int32 * step32);
	static void ClearMem3 (int *offset, int *count, int *step);
};
#endif
