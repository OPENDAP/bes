// DODSConstraintFuncs.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSConstraintFuncs.h"
#include "OPeNDAPDataNames.h"

string
DODSConstraintFuncs::pre_to_post_constraint( const string &name,
					     const string &pre_constraint )
{
    string str = pre_constraint ;
    string new_name = name ;
    new_name.append( "." ) ;
    if( str != "" )
    {
	str.insert( 0, new_name ) ;
	int pos = 0 ;
	pos = str.find( ',', pos ) ;
	while( pos != -1 )
	{
	    pos++ ;
	    str.insert( pos, new_name ) ;
	    pos = str.find( ',', pos ) ;
	}
    }

    return str ;
}

void
DODSConstraintFuncs::post_append( DODSDataHandlerInterface &dhi )
{
    if( dhi.container && dhi.container->get_constraint() != "" )
    {
	string to_append =
	    pre_to_post_constraint( dhi.container->get_symbolic_name(),
				    dhi.container->get_constraint() ) ;
	string constraint = dhi.data[POST_CONSTRAINT] ;
	if( constraint != "" )
	    constraint += "," ;
	constraint.append( to_append ) ;
	dhi.data[POST_CONSTRAINT] = constraint ;
    }
}

// $Log: DODSConstraintFuncs.cc,v $
// Revision 1.3  2005/02/02 20:12:18  pwest
// bug appending constraints to post_constraint
//
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
