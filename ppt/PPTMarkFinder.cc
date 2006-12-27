// PPTMarkFinder.cc

#include "PPTMarkFinder.h"

PPTMarkFinder::PPTMarkFinder( unsigned char *mark, int markLength )
    : _markIndex( 0 ),
      _markLength( markLength )
{
    for( int i = 0; i < markLength; i++ )
    {
	_mark[i] = mark[i] ;
    }
}

bool
PPTMarkFinder::markCheck( unsigned char b )
{
    if( _mark[_markIndex] == b )
    {
	_markIndex++;
	if( _markIndex == _markLength )
	{
	    _markIndex = 0;
	    return true ;
	}
    }
    else
    {
	_markIndex = 0 ;
    }

    return false ;
}

