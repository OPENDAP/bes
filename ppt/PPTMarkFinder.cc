// PPTMarkFinder.cc

#include "PPTMarkFinder.h"
#include "PPTException.h"

PPTMarkFinder::PPTMarkFinder( unsigned char *mark, int markLength )
    : _markIndex( 0 ),
      _markLength( markLength )
{
    // Lets get sure we will not overrun the buffer
    if (markLength > PPTMarkFinder_Buffer_Size)
        throw PPTException( "mark given will overrun internal buffer.",
                            __FILE__, __LINE__ ) ;
    memcpy((void*) _mark, (void*) mark, (markLength * sizeof(unsigned char)) );
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

