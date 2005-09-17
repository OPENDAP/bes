// OPeNDAPCommand.cc

#include "OPeNDAPCommand.h"
#include "OPeNDAPTermCommand.h"

OPeNDAPCommand *OPeNDAPCommand::TermCommand = new OPeNDAPTermCommand( "term" ) ;
map< string, OPeNDAPCommand * > OPeNDAPCommand::cmd_list ;

void
OPeNDAPCommand::add_command( const string &cmd_str, OPeNDAPCommand *cmd )
{
    OPeNDAPCommand::cmd_list[cmd_str] = cmd ;
}

OPeNDAPCommand *
OPeNDAPCommand::rem_command( const string &cmd_str )
{
    OPeNDAPCommand *cmd = NULL ;

    OPeNDAPCommand::cmd_iter iter = OPeNDAPCommand::cmd_list.find( cmd_str ) ;
    if( iter != OPeNDAPCommand::cmd_list.end() )
    {
	cmd = (*iter).second ;
	OPeNDAPCommand::cmd_list.erase( iter ) ;
    }
    return cmd ;
}

OPeNDAPCommand *
OPeNDAPCommand::find_command( const string &cmd_str )
{
    return OPeNDAPCommand::cmd_list[cmd_str] ;
}

