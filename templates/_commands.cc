// OPENDAP_TYPE_commands.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "BESInitList.h"
#include "BESInitOrder.h"
#include "BESCommand.h"
#include "BESLog.h"
#include "BESResponseNames.h"
#include "OPENDAP_CLASSResponseNames.h"

static bool
OPENDAP_CLASSCmdInit(int, char**)
{
    if( BESLog::TheLog()->is_verbose() )
	(*BESLog::TheLog()) << "Initializing OPENDAP_CLASS Commands:" << endl ;

    string cmd_name ;

    return true ;
}

static bool
OPENDAP_CLASSCmdTerm(void)
{
    return true ;
}

FUNINITQUIT( OPENDAP_CLASSCmdInit, OPENDAP_CLASSCmdTerm, BESMODULE_INIT ) ;

// $Log: OPENDAP_TYPE_commands.cc,v $
