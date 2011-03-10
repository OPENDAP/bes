// BESCatalogUtils.cc

// This file is part of bes, A C++ back-end server implementation framework
// for the OPeNDAP Data Access Protocol.

// Copyright (c) 2004-2009 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
// 
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301
 
// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include "config.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <iostream>
using std::cout ;
using std::endl ;

#include "BESCatalogUtils.h"
#include "TheBESKeys.h"
#include "BESInternalError.h"
#include "BESSyntaxUserError.h"
#include "BESNotFoundError.h"
#include "BESRegex.h"
#include "BESUtil.h"

map<string, BESCatalogUtils *> BESCatalogUtils::_instances ;

BESCatalogUtils::
BESCatalogUtils( const string &n )
    : _follow_syms( false )
{
    string key = "BES.Catalog." + n + ".RootDirectory" ;
    bool found = false ;
    TheBESKeys::TheKeys()->get_value( key, _root_dir, found ) ;
    if( !found || _root_dir == "" )
    {
	string s = key + " not defined in BES configuration file" ;
	throw BESSyntaxUserError( s, __FILE__, __LINE__ ) ;
    }
    DIR *dip = opendir( _root_dir.c_str() ) ;
    if( dip == NULL )
    {
	string serr = "BESCatalogDirectory - root directory "
	              + _root_dir + " does not exist" ;
	throw BESNotFoundError( serr, __FILE__, __LINE__ ) ;
    }
    closedir( dip ) ;

    found = false ;
    key = (string)"BES.Catalog." + n + ".Exclude" ;
    vector<string> vals ;
    TheBESKeys::TheKeys()->get_values( key, vals, found ) ;
    vector<string>::iterator ei = vals.begin() ;
    vector<string>::iterator ee = vals.end() ;
    for( ; ei != ee; ei++ )
    {
	string e_str = (*ei) ;
	if( !e_str.empty() && e_str != ";" )
	    BESUtil::explode( ';', e_str, _exclude ) ;
    }

    key = (string)"BES.Catalog." + n + ".Include" ;
    vals.clear() ;
    TheBESKeys::TheKeys()->get_values( key, vals, found ) ;
    vector<string>::iterator ii = vals.begin() ;
    vector<string>::iterator ie = vals.end() ;
    for( ; ii != ie; ii++ )
    {
	string i_str = (*ii) ;
	if( !i_str.empty() && i_str != ";" )
	    BESUtil::explode( ';', i_str, _include ) ;
    }

    key = "BES.Catalog." + n + ".TypeMatch" ;
    list<string> match_list ;
    vals.clear() ;
    TheBESKeys::TheKeys()->get_values( key, vals, found ) ;
    if( !found || vals.size() == 0 )
    {
	string s = key + " not defined in key file" ;
	throw BESInternalError( s, __FILE__, __LINE__ ) ;
    }
    vector<string>::iterator vi = vals.begin() ;
    vector<string>::iterator ve = vals.end() ;
    for( ; vi != ve; vi++ )
    {
	BESUtil::explode( ';', (*vi), match_list ) ;
    }

    list<string>::iterator mli = match_list.begin() ;
    list<string>::iterator mle = match_list.end() ;
    for( ; mli != mle; mli++ )
    {
	if( !((*mli).empty()) && *(mli) != ";" )
	{
	    list<string> amatch ;
	    BESUtil::explode( ':', (*mli), amatch ) ;
	    if( amatch.size() != 2 )
	    {
		string s = (string)"Catalog type match malformed, "
			   + "looking for type:regexp;[type:regexp;]" ;
		throw BESInternalError( s, __FILE__, __LINE__ ) ;
	    }
	    list<string>::iterator ami = amatch.begin() ;
	    type_reg newval ;
	    newval.type = (*ami) ;
	    ami++ ;
	    newval.reg = (*ami) ;
	    _match_list.push_back( newval ) ;
	}
    }

    key = (string)"BES.Catalog." + n + ".FollowSymLinks" ;
    string s_str ;
    TheBESKeys::TheKeys()->get_value( key, s_str, found ) ;
    s_str = BESUtil::lowercase( s_str ) ;
    if( s_str == "yes" || s_str == "on" || s_str == "true" )
    {
	_follow_syms = true ;
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
	    if( !reg.empty() )
	    {
		try
		{
		    // must match exactly, meaing result is = to length of string
		    // in question
		    BESRegex reg_expr( reg.c_str() ) ;
		    if( reg_expr.match( inQuestion.c_str(),
					inQuestion.length() ) ==
			static_cast<int>(inQuestion.length()) )
		    {
			toInclude = true ;
		    }
		}
		catch( BESError &e )
		{
		    string serr = (string)"Unable to get catalog information, "
				  + "malformed Catalog Include parameter "
				  + "in bes configuration file around " 
				  + reg + ": " + e.get_message() ;
		    throw BESInternalError( serr, __FILE__, __LINE__ ) ;
		}
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
	if( !reg.empty() )
	{
	    try
	    {
		BESRegex reg_expr( reg.c_str() ) ;
		if( reg_expr.match( inQuestion.c_str(), inQuestion.length() ) ==
		    static_cast<int>(inQuestion.length()) )
		{
		    return true ;
		}
	    }
	    catch( BESError &e )
	    {
		string serr = (string)"Unable to get catalog information, "
			      + "malformed Catalog Exclude parameter " 
			      + "in bes configuration file around " 
			      + reg + ": " + e.get_message() ;
		throw BESInternalError( serr, __FILE__, __LINE__ ) ;
	    }
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
	    if( !(*i_iter).empty() )
	    {
		strm << BESIndent::LMarg << *i_iter << endl ;
	    }
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
	    if( !(*e_iter).empty() )
	    {
		strm << BESIndent::LMarg << *e_iter << endl ;
	    }
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

    if( _follow_syms )
    {
	strm << BESIndent::LMarg << "    follow symbolic links: on" << endl ;
    }
    else
    {
	strm << BESIndent::LMarg << "    follow symbolic links: off" << endl ;
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

