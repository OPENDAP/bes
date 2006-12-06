/*-------------------------------------------------------------------------
 * Copyright (C) 1999	National Center for Supercomputing Applications.
 *			All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/* This is the HDF5-DAS which extracts DAS class descriptors converted from
   HDF5 attribute of an hdf5 data file. */

#include "config_hdf5.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include <iostream>
#include <string>

#include "debug.h"
#include "cgi_util.h"
#include "DAS.h"
#include "DDS.h"
#include "ConstraintEvaluator.h"
#include "HDF5TypeFactory.h"
#include "DODSFilter.h"
#include "InternalErr.h"
#include "h5das.h"
#include "h5dds.h"

extern "C" {
    hid_t get_fileid(const char *filename);
} const static string cgi_version = "3.0";

int main(int argc, char *argv[])
{
    DBG(cerr << "Starting the HDF server." << endl);

    try {
        DODSFilter df(argc, argv);
        if (df.get_cgi_version() == "")
            df.set_cgi_version(cgi_version);

        // Handle the version info as a special case since there's no need to
        // open the hdf5 file. This makes it possible to move the file open
        // and close code out of the individual case statements. 02/04/04
        // jhrg
        if (df.get_response() == DODSFilter::Version_Response) {
            df.send_version_info();
            return 0;
        }

        hid_t file1 = get_fileid(df.get_dataset_name().c_str());
        if (file1 < 0)
            throw Error(no_such_file, string("Could not open hdf5 file: ")
                        + df.get_dataset_name());

        switch (df.get_response()) {
        case DODSFilter::DAS_Response:{
                DAS das;
                find_gloattr(file1, das);
                depth_first(file1, "/", das,
                            df.get_dataset_name().c_str());
                df.send_das(das);
                break;
            }

        case DODSFilter::DDS_Response:{
                HDF5TypeFactory factory;
                DDS dds(&factory);
                ConstraintEvaluator ce;
                DAS das;

                depth_first(file1, "/", dds,
                            df.get_dataset_name().c_str());
                find_gloattr(file1, das);
                depth_first(file1, "/", das,
                            df.get_dataset_name().c_str());

                dds.transfer_attributes(&das);

                df.send_dds(dds, ce, true);
                break;
            }

        case DODSFilter::DataDDS_Response:{
                HDF5TypeFactory factory;
                DDS dds(&factory);
                ConstraintEvaluator ce;
                DAS das;

                depth_first(file1, "/", dds,
                            df.get_dataset_name().c_str());
                find_gloattr(file1, das);
                depth_first(file1, "/", das,
                            df.get_dataset_name().c_str());

                dds.transfer_attributes(&das);

                df.send_data(dds, ce, stdout);
                break;
            }

        case DODSFilter::DDX_Response:{
                HDF5TypeFactory factory;
                DDS dds(&factory);
                ConstraintEvaluator ce;
                DAS das;

                depth_first(file1, "/", dds,
                            df.get_dataset_name().c_str());
                find_gloattr(file1, das);
                depth_first(file1, "/", das,
                            df.get_dataset_name().c_str());

                dds.transfer_attributes(&das);

                df.send_ddx(dds, ce, stdout);
                break;
            }

        case DODSFilter::Version_Response:{
                df.send_version_info();

                break;
            }

        default:
            df.print_usage();   // Throws Error
        }

        if (H5Fclose(file1) < 0)
            throw Error(unknown_error,
                        string("Could not close the HDF5 file: ")
                        + df.get_dataset_name());

    }
    catch(Error & e) {
        string s;
        s = string("h5_handler: ") + e.get_error_message() + "\n";
        ErrMsgT(s);
        set_mime_text(stdout, dods_error, cgi_version);
        e.print(stdout);
        return 1;
    }
    catch(...) {
        string s("h5_handler: Unknown exception");
        ErrMsgT(s);
        Error e(unknown_error, s);
        set_mime_text(stdout, dods_error, cgi_version);
        e.print(stdout);
        return 1;
    }

    DBG(cerr << "HDF5 server exitied successfully." << endl);
    return 0;
}
