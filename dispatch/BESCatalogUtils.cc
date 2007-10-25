// BESCatalogUtils.cc

#include "sys/types.h"
#include "sys/stat.h"
#include "dirent.h"
#include "stdio.h"

#include "BESCatalogUtils.h"
#include "TheBESKeys.h"
#include "BESException.h"
#include "GNURegex.h"
#include "Error.h"

map<string, BESCatalogUtils *> BESCatalogUtils::_instances ;

BESCatalogUtils::
BESCatalogUtils( const string &n )
{
    string key = "BES.Catalog." + n + ".RootDirectory" ;
    bool found = false ;
    _root_dir = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( !found || _root_dir == "" )
    {
	string s = key + " not defined in key file" ;
	throw BESException( s, __FILE__, __LINE__ ) ;
    }
    DIR *dip = opendir( _root_dir.c_str() ) ;
    if( dip == NULL )
    {
	string serr = "BESCatalogDirectory - root directory "
	              + _root_dir + " does not exist" ;
	throw BESException( serr, __FILE__, __LINE__ ) ;
    }
    closedir( dip ) ;

    key = (string)"BES.Catalog." + n + ".Exclude" ;
    string e_str = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( found && e_str != "" && e_str != ";" )
    {
	build_list( _exclude, e_str ) ;
    }

    key = (string)"BES.Catalog." + n + ".Include" ;
    string i_str = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( found && i_str != "" && i_str != ";" )
    {
	build_list( _include, i_str ) ;
    }

    key = "BES.Catalog." + n + ".TypeMatch" ;
    string curr_str = TheBESKeys::TheKeys()->get_key( key, found ) ;
    if( curr_str == "" )
    {
	string s = key + " not defined in key file" ;
	throw BESException( s, __FILE__, __LINE__ ) ;
    }

    string::size_type str_begin = 0 ;
    string::size_type str_end = curr_str.length() ;
    string::size_type semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = curr_str.find( ";", str_begin ) ;
	if( semi == string::npos )
	{
	    string s = (string)"Catalog type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    throw BESException( s, __FILE__, __LINE__ ) ;
	}
	else
	{
	    string a_pair = curr_str.substr( str_begin, semi-str_begin ) ;
	    str_begin = semi+1 ;
	    if( semi == str_end-1 )
	    {
		done = true ;
	    }

	    string::size_type col = a_pair.find( ":" ) ;
	    if( col == string::npos )
	    {
		string s = (string)"Catalog type match malformed, no colon, "
			   + "looking for type:regexp;[type:regexp;]" ;
		throw BESException( s, __FILE__, __LINE__ ) ;
	    }
	    else
	    {
		type_reg newval ;
		newval.type = a_pair.substr( 0, col ) ;
		newval.reg = a_pair.substr( col+1, a_pair.length()-col ) ;
		_match_list.push_back( newval ) ;
	    }
	}
    }
}

void
BESCatalogUtils::build_list( list<string> &theList, const string &listStr )
{
    string::size_type str_begin = 0 ;
    string::size_type str_end = listStr.length() ;
    string::size_type semi = 0 ;
    bool done = false ;
    while( done == false )
    {
	semi = listStr.find( ";", str_begin ) ;
	if( semi == string::npos )
	{
	    string s = (string)"Catalog type match malformed, no semicolon, "
		       "looking for type:regexp;[type:regexp;]" ;
	    throw BESException( s, __FILE__, __LINE__ ) ;
	}
	else
	{
	    string a_member = listStr.substr( str_begin, semi-str_begin ) ;
	    str_begin = semi+1 ;
	    if( semi == str_end-1 )
	    {
		done = true ;
	    }
	    if( a_member != "" ) theList.push_back( a_member ) ;
	}
    }
}

bool
BESCatalogUtils::include( const string &inQuestion ) const
{
    bool toInclude = false ;

    // First check the file against the include list. If the file should be
    // included then check the exclude list to see if there are exceptions
    // to the include list.
    if( _include.size() == 0 )
    {
	toInclude = true ;
    }
    else
    {
	list<string>::const_iterator i_iter = _include.begin() ;
	list<string>::const_iterator i_end = _include.end() ;
	for( ; i_iter != i_end; i_iter++ )
	{
	    string reg = *i_iter ;
	    try
	    {
		// must match exactly, meaing result is = to length of string
		// in question
		Regex reg_expr( reg.c_str() ) ;
		if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) == inQuestion.length() )
		{
		    toInclude = true ;
		}
	    }
	    catch( Error &e )
	    {
		string serr = (string)"Unable to get catalog information, "
		              + "malformed Catalog Include parameter "
			      + "in bes configuration file around " 
			      + reg + ": " + e.get_error_message() ;
		throw BESException( serr, __FILE__, __LINE__ ) ;
	    }
	}
    }

    if( toInclude == true )
    {
	if( exclude( inQuestion ) )
	{
	    toInclude = false ;
	}
    }

    return toInclude ;
}

bool
BESCatalogUtils::exclude( const string &inQuestion ) const
{
    list<string>::const_iterator e_iter = _exclude.begin() ;
    list<string>::const_iterator e_end = _exclude.end() ;
    for( ; e_iter != e_end; e_iter++ )
    {
	string reg = *e_iter ;
	try
	{
	    Regex reg_expr( reg.c_str() ) ;
	    if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) == inQuestion.length() )
	    {
		return true ;
	    }
	}
	catch( Error &e )
	{
	    string serr = (string)"Unable to get catalog information, "
			  + "malformed Catalog Exclude parameter " 
			  + "in bes configuration file around " 
			  + reg + ": " + e.get_error_message() ;
	    throw BESException( serr, __FILE__, __LINE__ ) ;
	}
    }
    return false ;
}

BESCatalogUtils::match_citer
BESCatalogUtils::match_list_begin() const
{
    return _match_list.begin() ;
}

BESCatalogUtils::match_citer
BESCatalogUtils::match_list_end() const
{
    return _match_list.end() ;
}

void
BESCatalogUtils::dump( ostream &strm ) const
{
    strm << BESIndent::LMarg << "BESCatalogUtils::dump - ("
			     << (void *)this << ")" << endl ;
    BESIndent::Indent() ;

    strm << BESIndent::LMarg << "root directory: " << _root_dir << endl ;

    if( _include.size() )
    {
	strm << BESIndent::LMarg << "include list:" << endl ;
	BESIndent::Indent() ;
	list<string>::const_iterator i_iter = _include.begin() ;
	list<string>::const_iterator i_end = _include.end() ;
	for( ; i_iter != i_end; i_iter++ )
	{
	    strm << BESIndent::LMarg << *i_iter << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "include list: empty" << endl ;
    }

    if( _exclude.size() )
    {
	strm << BESIndent::LMarg << "exclude list:" << endl ;
	BESIndent::Indent() ;
	list<string>::const_iterator e_iter = _exclude.begin() ;
	list<string>::const_iterator e_end = _exclude.end() ;
	for( ; e_iter != e_end; e_iter++ )
	{
	    strm << BESIndent::LMarg << *e_iter << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "exclude list: empty" << endl ;
    }

    if( _match_list.size() )
    {
	strm << BESIndent::LMarg << "type matches:" << endl ;
	BESIndent::Indent() ;
	BESCatalogUtils::match_citer i = _match_list.begin() ;
	BESCatalogUtils::match_citer ie = _match_list.end() ;
	for( ; i != ie; i++ )
	{
	    type_reg match = (*i) ;
	    strm << BESIndent::LMarg << match.type << " : "
				     << match.reg << endl ;
	}
	BESIndent::UnIndent() ;
    }
    else
    {
	strm << BESIndent::LMarg << "    type matches: empty" << endl ;
    }

    BESIndent::UnIndent() ;
}

const BESCatalogUtils *
BESCatalogUtils::Utils( const string &cat_name )
{
    BESCatalogUtils *utils = BESCatalogUtils::_instances[cat_name] ;
    if( !utils )
    {
	utils = new BESCatalogUtils( cat_name );
	BESCatalogUtils::_instances[cat_name] = utils ;
    }
    return utils ;
}

