// OPENDAP_TYPE_commands.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "OPeNDAPCommand.h"
#include "DODSLog.h"
#include "DODSResponseNames.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSCmdInit(int, char**)
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing OPENDAP_CLASS Commands:" << endl ;

    string cmd_name ;

    return true ;
}

static bool
OPENDAP_CLASSCmdTerm(void)
{
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSCmdInit, OPENDAP_CLASSCmdTerm, DODSMODULE_INIT ) ;

// $Log: OPENDAP_TYPE_commands.cc,v $
