/////////////////////////////////////////////////////////////////////////////
// This file is part of the hdf4 data handler for the OPeNDAP data server.
// It retrieves the HDF-EOS2 swath or grid DFNT_CHAR 1D array field and
// then send to DAP as DAP string for the CF option.
//  Authors:   Kent Yang <myang6@hdfgroup.org>  
// Copyright (c) The HDF Group
/////////////////////////////////////////////////////////////////////////////

#ifdef USE_HDFEOS2_LIB
#include "config.h"
#include "config_hdf.h"

#include <iostream>
#include <sstream>
#include <cassert>
#include <libdap/debug.h>
#include <BESInternalError.h>
#include <BESDebug.h>
#include <BESLog.h>

#include "HDFCFUtil.h"
#include "HDFEOS2CFStr.h"
#include "HDF4RequestHandler.h"

using namespace std;
using namespace libdap;

HDFEOS2CFStr::HDFEOS2CFStr(const int gsfd,
                        const std::string &filename,
                        const std::string &objname,
                        const std::string &varname,
                        const std::string &varnewname,
                        int grid_or_swath)
              :Str(varnewname,filename),
               gsfd(gsfd),
               filename(filename),
               objname(objname),
               varname(varname),
               grid_or_swath(grid_or_swath)
{
}

HDFEOS2CFStr::~HDFEOS2CFStr() = default;

BaseType *HDFEOS2CFStr::ptr_duplicate()
{
    return new HDFEOS2CFStr(*this);
}

bool
HDFEOS2CFStr::read ()
{

    BESDEBUG("h4","Coming to HDFEOS2CFStr read "<<endl);

    bool check_pass_fileid_key = HDF4RequestHandler::get_pass_fileid();


    int32 (*openfunc) (char *, intn);
    intn (*closefunc) (int32);
    int32 (*attachfunc) (int32, char *);
    intn (*detachfunc) (int32);
    intn (*fieldinfofunc) (int32, char *, int32 *, int32 *, int32 *, char *);
    intn (*readfieldfunc) (int32, char *, int32 *, int32 *, int32 *, void *);


    // Define function pointers to handle the swath
    if (grid_or_swath == 0) {
        openfunc = GDopen;
        closefunc = GDclose;
        attachfunc = GDattach;
        detachfunc = GDdetach;
        fieldinfofunc = GDfieldinfo;
        readfieldfunc = GDreadfield;
 
    }
    else {
        openfunc = SWopen;
        closefunc = SWclose;
        attachfunc = SWattach;
        detachfunc = SWdetach;
        fieldinfofunc = SWfieldinfo;
        readfieldfunc = SWreadfield;
    }

    int32 gfid = -1;
    if (false == check_pass_fileid_key) {
        // Obtain the EOS object ID(either grid or swath)
        gfid = openfunc (const_cast < char *>(filename.c_str ()), DFACC_READ);
        if (gfid < 0) {
            string msg =  "File " +  filename + " cannot be open.";
            throw BESInternalError(msg,__FILE__,__LINE__);
        }
    }
    else
        gfid = gsfd;

    int32 gsid = attachfunc (gfid, const_cast < char *>(objname.c_str ()));
    if (gsid < 0) {
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "Grid/Swath " + objname + " cannot be attached.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    // Initialize the temp. returned value.
    intn r = 0;
    int32 tmp_rank = 0;
    char tmp_dimlist[1024];
    int32 tmp_dims[1];
    int32 field_dtype = 0;

    r = fieldinfofunc (gsid, const_cast < char *>(varname.c_str ()),
                &tmp_rank, tmp_dims, &field_dtype, tmp_dimlist);
    if (r != 0) {
        detachfunc(gsid);
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "Field " + varname + " information cannot be obtained.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }


    vector<int32>offset32;
    offset32.resize(1);
    vector<int32>count32;
    count32.resize(1);
    vector<int32>step32;
    step32.resize(1);
    offset32[0] = 0;
    count32[0]  = tmp_dims[0];
    step32[0]   = 1;

    vector<char>val;
    val.resize(count32[0]);

    r = readfieldfunc(gsid,const_cast<char*>(varname.c_str()),
                       offset32.data(), step32.data(), count32.data(), val.data());

    if (r != 0) {
        detachfunc(gsid);
        if (false == check_pass_fileid_key)
            closefunc(gfid);
        string msg = "Swath or grid readdata failed.";
        throw BESInternalError(msg,__FILE__,__LINE__);
    }

    string final_str(val.begin(),val.end());
    set_value(final_str);
    detachfunc(gsid);
    if (false == check_pass_fileid_key)
        closefunc(gfid);
    return false;
}


#endif
