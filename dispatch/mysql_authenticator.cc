// mysql_authenticator.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSMySQLAuthenticate.h"
#include "TheDODSAuthenticator.h"
#include "TheDODSLog.h"

DODSAuthenticate *TheDODSAuthenticator = 0;

static bool
TheDODSAuthenticatorInit(int, char**) {
    if( TheDODSLog->is_verbose() )
	(*TheDODSLog) << "Using MySQL Authentication" << endl;
    TheDODSAuthenticator = new DODSMySQLAuthenticate ;
    return true;
}

static bool
TheDODSAuthenticatorTerm(void) {
    if( TheDODSAuthenticator ) 
    {
	if( TheDODSLog->is_verbose() )
	    (*TheDODSLog) << "Cleaning up MySQL Authentication" << endl;
	delete (DODSMySQLAuthenticate *)TheDODSAuthenticator ;
    }
    TheDODSAuthenticator = 0 ;
    return true ;
}

FUNINITQUIT( TheDODSAuthenticatorInit, TheDODSAuthenticatorTerm, THEDODSAUTHENTICATOR_INIT ) ;

// $Log: mysql_authenticator.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
