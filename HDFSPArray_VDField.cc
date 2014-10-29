/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the Vdata fields from NASA HDF4 data products.
// Each Vdata will be decomposed into individual Vdata fields.
// Each field will be mapped to A DAP variable.

//  Authors:   MuQun Yang <myang6@hdfgroup.org>
// Copyright (c) 2010-2012 The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArray_VDField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include "InternalErr.h"
#include <BESDebug.h>
#include "HDFCFUtil.h"

using namespace std;
#define SIGNED_BYTE_TO_INT32 1


bool
HDFSPArray_VDField::read ()
{

    BESDEBUG("h4","Coming to HDFSPArray_VDField read "<<endl);

    string check_pass_fileid_key_str="H4.EnablePassFileID";
    bool check_pass_fileid_key = false;
    check_pass_fileid_key = HDFCFUtil::check_beskeys(check_pass_fileid_key_str);

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(&offset[0],&step[0],&count[0]);

    int32 file_id = -1;

    if(true == check_pass_fileid_key) 
        file_id = fileid;

    else {
        // Open the file
        file_id = Hopen (filename.c_str (), DFACC_READ, 0);
        if (file_id < 0) {
            ostringstream eherr;
            eherr << "File " << filename.c_str () << " cannot be open.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
    }

    // Start the Vdata interface
    int32 vdata_id = 0;
    if (Vstart (file_id) < 0) {
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "This file cannot be open.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    // Attach the vdata
    vdata_id = VSattach (file_id, vdref, "r");
    if (vdata_id == -1) {
        Vend (file_id);
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
        ostringstream eherr;
        eherr << "Vdata cannot be attached.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }

    try {
        int32 r = -1;

        // Seek the position of the starting point
        if (VSseek (vdata_id, (int32) offset[0]) == -1) {
            ostringstream eherr;
            eherr << "VSseek failed at " << offset[0];
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, fdname.c_str ()) == -1) {
            ostringstream eherr;
            eherr << "VSsetfields failed with the name " << fdname;
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        int32 vdfelms = fdorder * count[0] * step[0];

        // Loop through each data type
        switch (dtype) {
            case DFNT_INT8:
            {
                vector<int8> val;
                val.resize(nelms);

                vector<int8>orival;
                orival.resize(vdfelms);

                // Read the data
                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                FULL_INTERLACE);

                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                // Obtain the subset portion of the data
                if (fdorder > 1) {
                   for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =
                            orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }


#ifndef SIGNED_BYTE_TO_INT32
                set_value ((dods_byte *) &val[0], nelms);
#else
                vector<int32>newval;
                newval.resize(nelms);

                for (int counter = 0; counter < nelms; counter++)
                    newval[counter] = (int32) (val[counter]);

                set_value ((dods_int32 *) &newval[0], nelms);

#endif
            }

                break;
            case DFNT_UINT8:
            case DFNT_UCHAR8:
            {

                vector<uint8>val;
                val.resize(nelms);
          
                vector<uint8>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, &orival[0], 1+(count[0] -1)* step[0], FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_byte *) &val[0], nelms);
            }

                break;

            case DFNT_INT16:
            {
                vector<int16>val;
                val.resize(nelms);
                vector<int16>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_int16 *) &val[0], nelms);
            }
                break;

            case DFNT_UINT16:

            {
                vector<uint16>val;
                val.resize(nelms);

                vector<uint16>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_uint16 *) &val[0], nelms);
            }
                break;
            case DFNT_INT32:
            {
                vector<int32>val;
                val.resize(nelms);
                vector<int32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_int32 *) &val[0], nelms);
            }
                break;

            case DFNT_UINT32:
            {
    
                vector<uint32>val;
                val.resize(nelms);

                vector<uint32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_uint32 *) &val[0], nelms);
            }
                break;
            case DFNT_FLOAT32:
            {
                vector<float32>val;
                val.resize(nelms);
                vector<float32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_float32 *) &val[0], nelms);
            }
                break;
            case DFNT_FLOAT64:
            {

                vector<float64>val;
                val.resize(nelms);

                vector<float64>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) &orival[0], 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    ostringstream eherr;
                    eherr << "VSread failed.";
                    throw InternalErr (__FILE__, __LINE__, eherr.str ());
                }

                if (fdorder > 1) {
                    for (int i = 0; i < count[0]; i++)
                        for (int j = 0; j < count[1]; j++)
                            val[i * count[1] + j] =	orival[i * fdorder * step[0] + offset[1] + j * step[1]];
                }
                else {
                    for (int i = 0; i < count[0]; i++)
                        val[i] = orival[i * step[0]];
                }

                set_value ((dods_float64 *) &val[0], nelms);
            }
                break;
            default:
                InternalErr (__FILE__, __LINE__, "unsupported data type.");
        }

        if (VSdetach (vdata_id) == -1) {
            ostringstream eherr;
            eherr << "VSdetach failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }

        if (Vend (file_id) == -1) {
            ostringstream eherr;

            eherr << "VSdetach failed.";
            throw InternalErr (__FILE__, __LINE__, eherr.str ());
        }
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
   }
   catch(...) {
        VSdetach(vdata_id);
        Vend(file_id);
        HDFCFUtil::close_fileid(-1,fileid,-1,-1,check_pass_fileid_key);
        throw;

   }

#if 0
    if (Hclose (file_id) == -1) {

        ostringstream eherr;

        eherr << "VSdetach failed.";
        throw InternalErr (__FILE__, __LINE__, eherr.str ());
    }
#endif

    return true;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFSPArray_VDField::format_constraint (int *offset, int *step, int *count)
{
    long nels = 1;
    int id = 0;

    Dim_iter p = dim_begin ();

    while (p != dim_end ()) {

        int start = dimension_start (p, true);
        int stride = dimension_stride (p, true);
        int stop = dimension_stop (p, true);


        // Check for illegical  constraint
        if (stride < 0 || start < 0 || stop < 0 || start > stop) {
            ostringstream oss;

            oss << "Array/Grid hyperslab indices are bad: [" << start <<
                   ":" << stride << ":" << stop << "]";
            throw Error (malformed_expr, oss.str ());
        }

        // Check for an empty constraint and use the whole dimension if so.
        if (start == 0 && stop == 0 && stride == 0) {
            start = dimension_start (p, false);
            stride = dimension_stride (p, false);
            stop = dimension_stop (p, false);
        }

        offset[id] = start;
        step[id] = stride;
        count[id] = ((stop - start) / stride) + 1; // count of elements
        nels *= count[id]; // total number of values for variable

        BESDEBUG ("h4",
                  "=format_constraint():"
                  << "id=" << id << " offset=" << offset[id]
                  << " step=" << step[id]
                  << " count=" << count[id]
                  << endl);

        id++;
        p++;
    }

    return nels;
}
