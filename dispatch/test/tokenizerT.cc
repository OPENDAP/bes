// tokenizerT.C

#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "tokenizerT.h"
#include "DODSTokenizer.h"
#include "DODSException.h"

int tokenizerT::
run(void) {
    cout << endl << "*****************************************" << endl;
    cout << "Entered tokenizerT::run" << endl;
    int retVal = 0;

    DODSTokenizer t ;
    t.tokenize( "define d1 as nc1 with nc1.constraint=\"a,b\",nc1.attributes=\"attr1,attr2\";" ) ;
    t.dump_tokens() ;
    string my_token ;

    try
    {
	my_token = t.get_first_token() ;
	if( my_token == "define" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "d1" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "as" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "nc1" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "with" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "nc1.constraint=" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	    unsigned int type = 0 ;
	    string name = t.parse_container_name( my_token, type ) ;
	    if( name == "nc1" )
	    {
		cout << "container name = " << name << ", good" << endl ;
		if( type == 1 )
		{
		    cout << "type = " << type << ", good" << endl ;
		}
		else
		{
		    cerr << "type = " << type << ", bad things man" << endl ;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "container name = " << name << ", bad things man" << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "\"a,b\"" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	    string no_quotes = t.remove_quotes( my_token ) ;
	    if( no_quotes == "a,b" )
	    {
		cout << "no_quotes = " << no_quotes << ", good" << endl ;
	    }
	    else
	    {
		cerr << "no_quotes = " << no_quotes << ", bad things man" << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "," )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "nc1.attributes=" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	    unsigned int type = 0 ;
	    string name = t.parse_container_name( my_token, type ) ;
	    if( name == "nc1" )
	    {
		cout << "container name = " << name << ", good" << endl ;
		if( type == 2 )
		{
		    cout << "type = " << type << ", good" << endl ;
		}
		else
		{
		    cerr << "type = " << type << ", bad things man" << endl ;
		    return 1 ;
		}
	    }
	    else
	    {
		cerr << "container name = " << name << ", bad things man" << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == "\"attr1,attr2\"" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	    string no_quotes = t.remove_quotes( my_token ) ;
	    if( no_quotes == "attr1,attr2" )
	    {
		cout << "no_quotes = " << no_quotes << ", good" << endl ;
	    }
	    else
	    {
		cerr << "no_quotes = " << no_quotes << ", bad things man" << endl ;
		return 1 ;
	    }
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}

	my_token = t.get_next_token() ;
	if( my_token == ";" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}
    }
    catch( DODSException &e )
    {
	cerr << "caught exception, bad things man" << endl ;
	cerr << e.get_error_description() << endl ;
    }

    try
    {
	cout << "trying to get next token" << endl ;
	my_token = t.get_next_token() ;
	cout << "done trying to get next token" << endl ;
	cerr << "found a token " << my_token << ", shouldn't have" << endl ;
	return 1 ;
    }
    catch( DODSException &e )
    {
	cout << "no more tokens, good" << endl ;
	cout << e.get_error_description() << endl ;
    }

    try
    {
	my_token = t.get_first_token() ;
	if( my_token == "define" )
	{
	    cout << "token = " << my_token << ", good" << endl ;
	}
	else
	{
	    cerr << "token = " << my_token << ", bad things man" << endl ;
	    return 1 ;
	}
    }
    catch( DODSException &e )
    {
	cerr << "caught exception, bad things man" << endl ;
	cerr << e.get_error_description() << endl ;
    }

    try
    {
	t.parse_error( "err1" ) ;
	cerr << "no exception thrown, bad things man" << endl ;
	return 1 ;
    }
    catch( DODSException &e )
    {
	cout << "caught exception, good" << endl ;
	cout << e.get_error_description() << endl ;
    }

    cout << endl << "*****************************************" << endl;
    cout << "Returning from tokenizerT::run" << endl;

    return retVal;
}

int
main(int argC, char **argV) {
    Application *app = new tokenizerT();
    return app->main(argC, argV);
}

