// cgi_persistence.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSContainerPersistenceCGI.h"
#include "DODSContainerPersistenceList.h"
#include "DODSLog.h"

static bool
CGIPersistenceInit(int, char**) {
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Adding CGI Persistence" << endl;
    DODSContainerPersistenceCGI *cpf =
	    new DODSContainerPersistenceCGI( "CGI" ) ;
    DODSContainerPersistenceList::TheList()->add_persistence( cpf ) ;
    return true;
}

static bool
CGIPersistenceTerm(void) {
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing CGI Persistence" << endl;
    DODSContainerPersistenceList::TheList()->rem_persistence( "CGI" ) ;
    return true ;
}

FUNINITQUIT( CGIPersistenceInit, CGIPersistenceTerm, PERSISTENCE_INIT ) ;

// $Log: cgi_persistence.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
