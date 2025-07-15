/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the Vdata fields from NASA HDF4 data products.
// Each Vdata will be decomposed into individual Vdata fields.
// Each field will be mapped to A DAP variable.

//  Authors:   Kent Yang <myang6@hdfgroup.org>
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#include "HDFSPArray_VDField.h"
#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include "hdf.h"
#include "mfhdf.h"
#include <BESInternalError.h>
#include <BESDebug.h>
#include "HDFCFUtil.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;
#define SIGNED_BYTE_TO_INT32 1


bool
HDFSPArray_VDField::read ()
{

    BESDEBUG("h4","Coming to HDFSPArray_VDField read "<<endl);
    if (length() == 0)
        return true; 

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();

    // Declaration of offset,count and step
    vector<int>offset;
    offset.resize(rank);
    vector<int>count;
    count.resize(rank);
    vector<int>step;
    step.resize(rank);

    // Obtain offset,step and count from the client expression constraint
    int nelms = format_constraint(offset.data(),step.data(),count.data());

    int32 file_id = -1;

    if(true == check_pass_fileid_key) 
        file_id = fileid;

    else {
        // Open the file
        file_id = Hopen (filename.c_str (), DFACC_READ, 0);
        if (file_id < 0) {
            string msg = "File " + filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }

    // Start the Vdata interface
    int32 vdata_id = 0;
    if (Vstart (file_id) < 0) {
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
        string msg = "This file cannot be open.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Attach the vdata
    vdata_id = VSattach (file_id, vdref, "r");
    if (vdata_id == -1) {
        Vend (file_id);
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
        string msg = "Vdata cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    try {
        int32 r = -1;

        // Seek the position of the starting point
        if (VSseek (vdata_id, (int32) offset[0]) == -1) {
            string msg = "VSseek failed at " + to_string(offset[0]) + ".";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        // Prepare the vdata field
        if (VSsetfields (vdata_id, fdname.c_str ()) == -1) {
            string msg = "VSsetfields failed with the name " + fdname +".";
            throw BESInternalError(msg,__FILE__,__LINE__);
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
                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                FULL_INTERLACE);

                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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
                set_value ((dods_byte *) val.data(), nelms);
#else
                vector<int32>newval;
                newval.resize(nelms);

                for (int counter = 0; counter < nelms; counter++)
                    newval[counter] = (int32) (val[counter]);

                set_value ((dods_int32 *) newval.data(), nelms);

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

                r = VSread (vdata_id, orival.data(), 1+(count[0] -1)* step[0], FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_byte *) val.data(), nelms);
            }

                break;

            case DFNT_INT16:
            {
                vector<int16>val;
                val.resize(nelms);
                vector<int16>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_int16 *) val.data(), nelms);
            }
                break;

            case DFNT_UINT16:

            {
                vector<uint16>val;
                val.resize(nelms);

                vector<uint16>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_uint16 *) val.data(), nelms);
            }
                break;
            case DFNT_INT32:
            {
                vector<int32>val;
                val.resize(nelms);
                vector<int32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_int32 *) val.data(), nelms);
            }
                break;

            case DFNT_UINT32:
            {
    
                vector<uint32>val;
                val.resize(nelms);

                vector<uint32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_uint32 *) val.data(), nelms);
            }
                break;
            case DFNT_FLOAT32:
            {
                vector<float32>val;
                val.resize(nelms);
                vector<float32>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_float32 *) val.data(), nelms);
            }
                break;
            case DFNT_FLOAT64:
            {

                vector<float64>val;
                val.resize(nelms);

                vector<float64>orival;
                orival.resize(vdfelms);

                r = VSread (vdata_id, (uint8 *) orival.data(), 1+(count[0] -1)* step[0],
                        FULL_INTERLACE);
                if (r == -1) {
                    string msg = "VSread failed.";
                    throw BESInternalError(msg,__FILE__,__LINE__);
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

                set_value ((dods_float64 *) val.data(), nelms);
            }
                break;
            default:
                throw BESInternalError ("Unsupported data type.",__FILE__, __LINE__);
        }

        if (VSdetach (vdata_id) == -1) {
            string msg = "VSdetach failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }

        if (Vend (file_id) == -1) {
            string msg ="VSdetach failed.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
        HDFCFUtil::close_fileid(-1,file_id,-1,-1,check_pass_fileid_key);
   }
   catch(...) {
        VSdetach(vdata_id);
        Vend(file_id);
        HDFCFUtil::close_fileid(-1,fileid,-1,-1,check_pass_fileid_key);
        throw;

   }

    return true;
}

// Standard way of DAP handlers to pass the coordinates of the subsetted region to the handlers
// Return the number of elements to read. 
int
HDFSPArray_VDField::format_constraint (int *offset, int *step, int *count)
{
    int nels = 1;
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
    }

    return nels;
}


