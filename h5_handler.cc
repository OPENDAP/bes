/// \file h5_handler.cc
/// \brief main program source.
///
/// Copyright (C) 2007	HDF Group, Inc.
///
/// Copyright (C) 1999	National Center for Supercomputing Applications.
///		All rights reserved.

#include "h5_handler.h"

const static string cgi_version = "3.0";
extern H5EOS eos;


/// \fn main(int argc, char *argv[])
/// main function for HDF5 data handler.
/// This function processes options and generates the corresponding DAP outputs requested by the user.
/// @param argc number of arguments
/// @param argv command line arguments for options
/// @return 0 if success
/// @return 1 if error
int main(int argc, char *argv[])
{
  DBG(cerr << "Starting the HDF handler." << endl);

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

    DBG(cerr << "checking EOS file" << endl);
    if(eos.check_eos(file1)){
      DBG(cerr << "eos file is detected" << endl);
      eos.set_dimension_array();
    }
    else{
      DBG(cerr << "eos file is not detected" << endl);
    }
    //#endif
    
    // More C++ style? How to use virtual function. <hyokyung 2007.02.20. 13:31:10>
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
      // DAS das;
      // Check if it is EOS file.

		
      depth_first(file1, "/", dds,
		  df.get_dataset_name().c_str());
      // find_gloattr(file1, das);
      DBG(cerr << ">dds.transfer_attributesr" << endl);		
      // dds.transfer_attributes(&das); // ? <hyokyung 2007.02.20. 13:31:49>
      DBG(cerr << ">df.send_dds()" << endl);				
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
      // find_gloattr(file1, das);
      // dds.transfer_attributes(&das);
      df.send_data(dds, ce, stdout); // ? <hyokyung 2007.02.20. 13:32:00>
      break;
    }

    case DODSFilter::DDX_Response:{ // What is DDX? <hyokyung 2007.02.20. 13:32:11>
      HDF5TypeFactory factory;
      DDS dds(&factory);
      ConstraintEvaluator ce;
      DAS das;

      depth_first(file1, "/", dds,
		  df.get_dataset_name().c_str());
      find_gloattr(file1, das);
      //depth_first(file1, "/", das,
      //            df.get_dataset_name().c_str());

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
    // May not be right? <hyokyung 2007.02.20. 13:32:35>
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
