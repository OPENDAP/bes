// mysql_persistence.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSContainerPersistenceMySQL.h"
#include "ThePersistenceList.h"
#include "TheDODSLog.h"

static bool
MySQLPersistenceInit(int, char**) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Adding MySQL Persistence" << endl;
    DODSContainerPersistenceMySQL *cpf =
	    new DODSContainerPersistenceMySQL( "MySQL" ) ;
    ThePersistenceList->add_persistence( cpf ) ;
    return true;
}

static bool
MySQLPersistenceTerm(void) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Removing MySQL Persistence" << endl;
    ThePersistenceList->rem_persistence( "MySQL" ) ;
    return true ;
}

FUNINITQUIT( MySQLPersistenceInit, MySQLPersistenceTerm, PERSISTENCE_INIT ) ;

// $Log: mysql_persistence.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
