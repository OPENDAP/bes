// opendap_commands.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <iostream>

using std::endl ;

#include "DODSInitList.h"
#include "DODSInitOrder.h"
#include "DODSResponseNames.h"

#include "DODSLog.h"

#include "OPeNDAPGetCommand.h"
#include "OPeNDAPSetCommand.h"
#include "OPeNDAPShowCommand.h"
#include "OPeNDAPDefineCommand.h"
#include "OPeNDAPDeleteCommand.h"

static bool
DODSCommandInit(int, char**) {
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Initializing DODS Commands:" << endl;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << GET_RESPONSE << " command" << endl;
    OPeNDAPCommand *cmd = new OPeNDAPGetCommand( GET_RESPONSE ) ;
    OPeNDAPCommand::add_command( GET_RESPONSE, cmd ) ;

    string cmd_name = string( GET_RESPONSE ) + "." + DAS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DDS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DDX_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( GET_RESPONSE ) + "." + DATA_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SHOW_RESPONSE << " command" << endl;
    cmd = new OPeNDAPShowCommand( SHOW_RESPONSE ) ;
    OPeNDAPCommand::add_command( SHOW_RESPONSE, cmd ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + HELP_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + PROCESS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + CONTAINERS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + DEFINITIONS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + KEYS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + VERS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( SHOW_RESPONSE ) + "." + STATUS_RESPONSE ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DEFINE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPDefineCommand( DEFINE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DEFINE_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << SET_RESPONSE << " command" << endl;
    cmd = new OPeNDAPSetCommand( SET_RESPONSE ) ;
    OPeNDAPCommand::add_command( SET_RESPONSE, cmd ) ;

    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << DELETE_RESPONSE << " command" << endl;
    cmd = new OPeNDAPShowCommand( DELETE_RESPONSE ) ;
    OPeNDAPCommand::add_command( DELETE_RESPONSE, cmd ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_CONTAINER ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_DEFINITION ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    cmd_name = string( DELETE_RESPONSE ) + "." + DELETE_DEFINITIONS ;
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "    adding " << cmd_name << " command" << endl;
    OPeNDAPCommand::add_command( cmd_name, OPeNDAPCommand::TermCommand ) ;

    return true;
}

static bool
DODSCommandTerm( void )
{
    if( DODSLog::TheLog()->is_verbose() )
	(*DODSLog::TheLog()) << "Removing DODS Commands:" << endl;

    OPeNDAPCommand *cmd = OPeNDAPCommand::rem_command( GET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SHOW_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( DEFINE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( SET_RESPONSE ) ;
    if( cmd ) delete cmd ;

    cmd = OPeNDAPCommand::rem_command( DELETE_RESPONSE ) ;
    if( cmd ) delete cmd ;

    return true;
}

FUNINITQUIT(DODSCommandInit, DODSCommandTerm, DODSMODULE_INIT);

// $Log: dods_module.cc,v $
// Revision 1.7  2005/04/19 17:53:34  pwest
// added keys handler for show keys command
//
// Revision 1.6  2005/03/17 19:26:22  pwest
// added delete command to delete a specific container, a specific definition, or all definitions
//
// Revision 1.5  2005/03/15 19:55:36  pwest
// show containers and show definitions
//
// Revision 1.4  2005/02/09 19:46:55  pwest
// added basic transmitter and http transmitter to return manager
//
// Revision 1.3  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
