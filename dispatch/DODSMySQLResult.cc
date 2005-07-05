// DODSMySQLResult.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include "DODSMySQLResult.h"

DODSMySQLResult::DODSMySQLResult( const int &n, const int &f )
    : _nrows( n ),
      _nfields( f )
{
    _matrix=new matrix ;
    _matrix->reserve( _nrows ) ;
    row r ;
    r.reserve( _nfields ) ;
    for( register int j=0; j<_nrows; j++ )
	_matrix->push_back( r ) ;
}

DODSMySQLResult::~DODSMySQLResult()
{
    delete _matrix;
}

void
DODSMySQLResult::set_field( const char *s )
{
    string st = s ;
    (*_matrix)[_row_position].push_back( st ) ;
}

string
DODSMySQLResult::get_field()
{
    return (*_matrix)[_row_position][_field_position];
}

bool
DODSMySQLResult::first_field()
{
    if( _nfields > 0 )
    {
	_field_position = 0 ;
	return true ;
    }
    return false ;
}

bool
DODSMySQLResult::next_field()
{
    if( ++_field_position < _nfields )
	return true ;
    return false ;
}

bool
DODSMySQLResult::next_row()
{
    if( ++_row_position < _nrows )
	return true ;
    return false ;
}

bool
DODSMySQLResult::first_row()
{
    if( _nrows > 0 )
    {
	_row_position = 0 ;
	return true ;
    }
    return false ;
}

// $Log: DODSMySQLResult.cc,v $
// Revision 1.2  2004/09/09 17:17:12  pwest
// Added copywrite information
//
// Revision 1.1  2004/06/30 20:16:24  pwest
// dods dispatch code, can be used for apache modules or simple cgi script
// invocation or opendap daemon. Built during cedar server development.
//
