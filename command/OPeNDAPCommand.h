// OPeNDAPCommand.h

#ifndef A_OPeNDAPCommand_h
#define A_OPeNDAPCommand_h 1

#include <string>
#include <map>

using std::string ;
using std::map ;
using std::less ;
using std::allocator ;

#include "DODSDataHandlerInterface.h"

class DODSResponseHandler ;
class DODSTokenizer ;

class OPeNDAPCommand
{
private:
    static map< string, OPeNDAPCommand *, less< string >, allocator< string > > cmd_list ;
    typedef map< string, OPeNDAPCommand *, less< string >, allocator< string > >::iterator cmd_iter ;
protected:
    string				_cmd ;
public:
    					OPeNDAPCommand( const string &cmd )
					    : _cmd( cmd ) {}
    virtual				~OPeNDAPCommand() {}
    virtual DODSResponseHandler *	parse_request( DODSTokenizer &tokenizer,
					  DODSDataHandlerInterface &dhi ) = 0 ;

    static OPeNDAPCommand *		TermCommand ;
    static void				add_command( const string &cmd_str,
                                                     OPeNDAPCommand *cmd ) ;
    static OPeNDAPCommand *		rem_command( const string &cmd_str ) ;
    static OPeNDAPCommand *		find_command( const string &cmd_str ) ;
} ;

#endif // A_OPeNDAPCommand_h

