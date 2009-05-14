////////////////////////////////////////////////////////////////////////////////
/// \file h5_handler.cc
/// \brief main program source.
///
/// \author Muqun Yang (ymuqun@ncsa.uiuc.edu)
/// \author Hyo-Kyung Lee <hyoklee@hdfgroup.org>
/// 
/// Copyright (C) 2007  The HDF Group
///
/// Copyright (C) 1999  National Center for Supercomputing Applications.
///             All rights reserved.
////////////////////////////////////////////////////////////////////////////////
#include "h5_handler.h"

/// The version of this handler.
const static string cgi_version = PACKAGE_VERSION;

/// An external object that handles NASA EOS HDF5 files for grid generation 
/// and meta data parsing.
extern H5EOS eos;

///////////////////////////////////////////////////////////////////////////////
/// \fn main(int argc, char *argv[])
/// main function for HDF5 data handler.
/// This function processes options and generates the corresponding DAP outputs requested by the user.
/// @param argc number of arguments
/// @param argv command line arguments for options
/// @return 0 if success
/// @return 1 if error
///////////////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
    DBG(cerr << "Starting the hdf5 handler." << endl);

    try {
        DODSFilter df(argc, argv);
        if (df.get_cgi_version() == "")
            df.set_cgi_version(cgi_version);

        // Handle the version info as a special case since there's no need to
        // open the hdf5 file. This makes it possible to move the file open
        // and close code out of the individual case statements.
        // jhrg 02/04/04 
        if (df.get_response() == DODSFilter::Version_Response) {
            df.send_version_info();
            return 0;
        }

        hid_t file1 = get_fileid(df.get_dataset_name().c_str());

        if (file1 < 0)
            throw Error(no_such_file, string("Could not open hdf5 file: ")
                        + df.get_dataset_name());

        // Check if it is an HDF-EOS5 file.
        DBG(cerr << "checking HDF-EOS5 file" << endl);
        if (eos.check_eos(file1)) {
            DBG(cerr << "An HDF-EOS5 file is detected" << endl);
            eos.set_dimension_array();
        } else {
            DBG(cerr << "An HDF-EOS5 file is not detected" << endl);
        }

        switch (df.get_response()) { // One of DAS, DDS, DODS, DDX, Version request

        case DODSFilter::DAS_Response:{
            DAS das;
            find_gloattr(file1, das);
            depth_first(file1, "/", das); // Traverse the HDF5 groups

            df.send_das(das);

            break;
        }

        case DODSFilter::DDS_Response:{
            DAS das;
            DDS dds(NULL);
            ConstraintEvaluator ce;

            depth_first(file1, "/", dds,
                        df.get_dataset_name().c_str());

            DBG(cerr << ">df.send_dds()" << endl);
            df.send_dds(dds, ce, true);
            break;
        }

        case DODSFilter::DataDDS_Response:{


            DDS dds(NULL);
            ConstraintEvaluator ce;
            DAS das;

            depth_first(file1, "/", dds,
                        df.get_dataset_name().c_str());

            df.send_data(dds, ce, cout);
            break;
        }

        case DODSFilter::DDX_Response:{
            DDS dds(NULL);
            ConstraintEvaluator ce;
            DAS das;

            depth_first(file1, "/", dds,
                        df.get_dataset_name().c_str());
            find_gloattr(file1, das);
            depth_first(file1, "/", das);
            dds.transfer_attributes(&das);
            df.send_ddx(dds, ce, cout);
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

// $Log$
