// DODSRequesthandlerList.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSReporterList.h"
#include "DODSReporter.h"

DODSReporterList *DODSReporterList::_instance = 0 ;

DODSReporterList::DODSReporterList()
{
}

DODSReporterList::~DODSReporterList()
{
    DODSReporter *reporter = 0 ;
    DODSReporterList::Reporter_iter i = _reporter_list.begin() ;
    for( ; i != _reporter_list.end(); i++ )
    {
	reporter = (*i).second ;
	if( reporter ) delete reporter ;
	_reporter_list.erase( i ) ;
    }
}

bool
DODSReporterList::add_reporter( string reporter_name,
			      DODSReporter *reporter_object )
{
    if( find_reporter( reporter_name ) == 0 )
    {
	_reporter_list[reporter_name] = reporter_object ;
	return true ;
    }
    return false ;
}

DODSReporter *
DODSReporterList::remove_reporter( string reporter_name )
{
    DODSReporter *ret = 0 ;
    DODSReporterList::Reporter_iter i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	ret = (*i).second;
	_reporter_list.erase( i ) ;
    }
    return ret ;
}

DODSReporter *
DODSReporterList::find_reporter( string reporter_name )
{
    DODSReporterList::Reporter_citer i ;
    i = _reporter_list.find( reporter_name ) ;
    if( i != _reporter_list.end() )
    {
	return (*i).second;
    }
    return 0 ;
}

void
DODSReporterList::report( const DODSDataHandlerInterface &dhi )
{
    DODSReporter *reporter = 0 ;
    DODSReporterList::Reporter_iter i = _reporter_list.begin() ;
    for( ; i != _reporter_list.end(); i++ )
    {
	reporter = (*i).second ;
	if( reporter ) reporter->report( dhi ) ;
    }
}

DODSReporterList *
DODSReporterList::TheList()
{
    if( _instance == 0 )
    {
	_instance = new DODSReporterList ;
    }
    return _instance ;
}

// $Log: DODSReporterList.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
