// HDFRequestHandler.cc

#include <iostream>
#include <fstream>

using std::ifstream ;
using std::cerr ;
using std::endl ;

#ihdflude "DAS.h"
#ihdflude "DDS.h"

#ihdflude "HDFRequestHandler.h"
#ihdflude "DODSResponseHandler.h"
#ihdflude "DODSResponseNames.h"
#ihdflude "DODSConstraintFuncs.h"
#ihdflude "DODSInfo.h"
#ihdflude "TheDODSKeys.h"
#ihdflude "DODSResponseException.h"

extern void read_variables(DAS &das, const string &filename) throw (Error);
extern void read_descriptors(DDS &dds, const string &filename)  throw (Error);

HDFRequestHandler::HDFRequestHandler( string name )
    : DODSRequestHandler( name )
{
    add_handler( DAS_RESPONSE, HDFRequestHandler::hdf_build_das ) ;
    add_handler( DDS_RESPONSE, HDFRequestHandler::hdf_build_dds ) ;
    add_handler( DATA_RESPONSE, HDFRequestHandler::hdf_build_data ) ;
    add_handler( HELP_RESPONSE, HDFRequestHandler::hdf_build_help ) ;
    add_handler( VERS_RESPONSE, HDFRequestHandler::hdf_build_version ) ;
}

HDFRequestHandler::~HDFRequestHandler()
{
}

bool
HDFRequestHandler::hdf_build_das( DODSDataHandlerInterface &dhi )
{
    DAS *das = (DAS *)dhi.response_handler->get_response_object() ;

    read_das( *das, "/tmp/", dhi.container->get_real_name() );

    return true ;
}

bool
HDFRequestHandler::hdf_build_dds( DODSDataHandlerInterface &dhi )
{
    DDS *dds = (DDS *)dhi.response_handler->get_response_object() ;

    read_dds( *dds, "/tmp/", dhi.container->get_real_name() );
    dds->parse_constraint( dhi.container->get_constraint() );

    return true ;
}

bool
HDFRequestHandler::hdf_build_data( DODSDataHandlerInterface &dhi )
{
    dds->filename( dhi.container->get_real_name() );
    read_dds( *dds, "/tmp/", dhi.container->get_real_name() );
    dhi.post_constraint = dhi.container->get_constraint();

    return true ;
}

bool
HDFRequestHandler::hdf_build_help( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = (DODSInfo *)dhi.response_handler->get_response_object() ;
    info->add_data( (string)"No help for netCDF handler.\n" ) ;

#if 0
    info->add_data( (string)"cdf-dods help: " + hdf_version() + "\n" ) ;
    bool found = false ;
    string key = (string)"HDF.Help." + dhi.transmit_protocol ;
    string file = TheDODSKeys->get_key( key, found ) ;
    if( found == false )
    {
     info->add_data( "no help information available for cdf-dods\n" ) ;
    }
    else
    {
        ifstream ifs( file.c_str() ) ;
 if( !ifs )
     {
          info->add_data( "cdf-dods help file not found, help information not available\n" ) ;
       }
      else
   {
          char line[4096] ;
      while( !ifs.eof() )
            {
          ifs.getline( line, 4096 ) ;
            if( !ifs.eof() )
               {
                  info->add_data( line ) ;
               info->add_data( "\n" ) ;
           }
          }
      ifs.close() ;
      }
    }
#endif

    return true ;
}

bool
HDFRequestHandler::nc_build_version( DODSDataHandlerInterface &dhi )
{
    DODSInfo *info = (DODSInfo *)dhi.response_handler->get_response_object() ;
    info->add_data( (string)"    0.9\n" ) ;
    info->add_data( (string)"    libnc-dods 0.9\n" ) ;
#if 0
    info->add_data( (string)"    " + nc_version() + "\n" ) ;
    info->add_data( (string)"        libcdf2.7\n" ) ;
#endif
    return true ;
}

