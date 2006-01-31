
#include <H5Gpublic.h>
#include <H5Fpublic.h>
#include <H5Ipublic.h>
// Did not work with C++. 
// #include <H5Ppublic.h>
#include <H5Tpublic.h>
#include <H5Spublic.h>
#include <H5Apublic.h>
#include <H5public.h>

#include "cgi_util.h" 
#include "DDS.h"
#include "DODSFilter.h"
#include "common.h"

#include "HDF5Int32.h"
#include "HDF5UInt32.h"
#include "HDF5UInt16.h"
#include "HDF5Int16.h"
#include "HDF5Byte.h"
#include "HDF5Array.h"
#include "HDF5Str.h"
#include "HDF5Float32.h"
#include "HDF5Float64.h"
#include "HDF5Grid.h"
#if 0
#include "h5util.h"
#endif

bool depth_first(hid_t, char *, DDS &, const char *); 
void read_objects(DDS &dds, const string &varname, const string& filename);
                                                            
static const char STRING[]="String";
static const char BYTE[]="Byte";
static const char INT32[]="Int32";
static const char INT16[]="Int16";
static const char FLOAT64[]="Float64";
static const char FLOAT32[]="Float32";
static const char UINT16[]="UInt16";
static const char UINT32[]="UInt32";
static const char INT_ELSE[]="Int_else";
static const char FLOAT_ELSE[]="Float_else";

















