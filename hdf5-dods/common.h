/*-------------------------------------------------------------------------
 *
 * Copyright (C) 2001   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/****************************************************************************

  Description: 

1. HDF5-DODS server

See HDF5-DODS server specification for detailed description.
 
This file is a header file that includes common constants and global
structures of the server. 

Author:  Kent Yang(ymuqun@ncsa.uiuc.edu)

*****************************************************************************/

#include <H5Ipublic.h>

#define DODS_MAX_RANK 30
#define SPACE1_NAME  "Space1"
#define SPACE1_RANK	1
#define SPACE1_DIM1	4
#define DODS_NAMELEN    1024
#define DODSGRID
#define STR_FLAG 1
#define STR_NOFLAG 0
#define HDF5_softlink "HDF5_softlink"
#define HDF5_OBJ_FULLPATH "HDF5_OBJ_FULLPATH"
typedef struct DS {
    char name[DODS_NAMELEN];
    hid_t dset;
    hid_t type;
    hid_t dataspace;
    int ndims;
    int size[DODS_MAX_RANK];
    hsize_t nelmts;
    size_t need;
} DS_t;

typedef struct DSattr {
    char name[DODS_NAMELEN];
    int type;
    int ndims;
    int size[DODS_MAX_RANK];
    hsize_t nelmts;
    size_t need;
} DSattr_t;
