// DODS.h

// 2004 Copyright University Corporation for Atmospheric Research

#ifndef DODS_h_
#define DODS_h_ 1

#include "DODSDataHandlerInterface.h"

class DODSException ;
class DODSTransmitter ;

/** @brief Entry point into OPeNDAP, building responses to given requests.

    DODS is an abstract class providing the entry point into the retrieval
    of information using the OPeNDAP system. There are eight steps to
    retrieving a response to a given request:

    <OL>
    <LI>initialize the OPeNDAP environment</LI>
    <LI>authenticate the user attempting to retrieve the information if
    authentication is needed</LI>
    <LI>validate the incoming information to make sure that all information
    is available to perform the query</LI>
    <LI>build the request plan to retrieve the information. A response can
    be generated for multiple files using multiple server types (cedar, cdf,
    netcdf, etc...)</LI>
    <LI>execute the request plan and build the response object</LI>
    <LI>transmit the response object</LI>
    <LI>log the status of the request</LI>
    <LI>send out report information that can be reported on by any number of
    reporters registered with the system.</LI>
    </OL>

    The way in which the response is generated is as follows.
    A DODSResponseHandler is found that knows how to build the requested
    response object. The built-in response handlers are for the
    response objects das, dds, ddx, data, help, version. These response
    handlers are added to a response handler list during initialization.
    Additional response handlers can be added to this list. For example,
    in Cedar, response handlers are registered to build flat, tab, info,
    and stream responses.

    To build the response objects a user can make many requests. For
    example, a das object can be built using as many different files as is
    requested, say for file1,file2,file3,file4. And each of these files
    could be of a different data type. For example, file1 and file3 could
    be cedar files, file2 could be cdf file and file4 could be a netcdf
    file.

    The structure that holds all of the requested information is the
    DODSDataHandlerInterface. It holds on to a list of containers, each of
    which has the data type (cedar, cdf, nph, etc...) and the file to be
    read. The DODSDataHandlerInterface is built in the build request method.

    The response handlers know how to build the requested response object.
    For each container in the DODSDataHandlerInterface find the
    request handler (DODSRequestHandler) for the data type. Each request
    handler registers functions that know how to build a certain type of
    response (DAS, DDS, etc...). Find that function and invoke it. So, for
    example, there is a CedarRequestHandler class that registers functions
    that knows how to build the different response objects from cedar files.

    Once the response object is filled it is transmitted using a specified
    DODSTransmitter.

    The status is then logged (default is to not log any status. It is up
    to derived classes of DODS to implement the log_status method.)

    The request and status are then reported. The default action is to
    pass off the reporting to TheReporterList, which has a list of
    registered reporters and passes off the information to each of those
    reporters. For example, if the Cedar project wants to report on any
    cedar access then it can register a reporter with TheReporterList and if
    OPeNDAP wants to keep track of usage then it can register a reporter
    that will report/log all requests.

    @see DODSGlobalInit
    @see DODSKeys
    @see DODSResponseHandler 
    @see DODSRequestHandler 
    @see DODSTransmitter 
    @see DODSLog
    @see DODSReporter 
 */
class DODS
{
protected:
    DODSDataHandlerInterface	_dhi ;
    DODSTransmitter		*_transmitter ;

    virtual int			exception_manager(DODSException &e);
    virtual void		authenticate();
    virtual void		initialize();
    virtual void		validate_data_request();
    virtual void		build_data_request_plan();
    virtual void		execute_data_request_plan();
    virtual void		invoke_aggregation();
    virtual void		transmit_data();
    virtual void		log_status();
    virtual void		report_request();
    virtual void		clean();

    				DODS();
    virtual			~DODS();
public:
    virtual int			execute_request();
};

#endif // DODS_h_

// $Log: DODS.h,v $
// Revision 1.4  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.3  2004/12/15 17:39:03  pwest
// Added doxygen comments
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
