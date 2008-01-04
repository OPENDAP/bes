// PPTStreamBuf.cc

#include <sys/types.h>
//#include <sys/uio.h>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <iostream>

using std::ostringstream ;
using std::hex ;
using std::setw ;
using std::setfill ;

#include "PPTStreamBuf.h"
#include "PPTProtocol.h"

PPTStreamBuf::PPTStreamBuf( int fd, unsigned bufsize )
    : d_bufsize( bufsize ),
      d_buffer( 0 ),
      count( 0 )
{
    open( fd, bufsize ) ;
}

PPTStreamBuf::~PPTStreamBuf()
{
    if(d_buffer)
    {
	sync() ;
	delete d_buffer ;
    }
}

void
PPTStreamBuf::open( int fd, unsigned bufsize )
{
    d_fd = fd ;
    d_bufsize = bufsize == 0 ? 1 : bufsize ;

    d_buffer = new char[d_bufsize] ;
    setp( d_buffer, d_buffer + d_bufsize ) ;
}
  
int
PPTStreamBuf::sync()
{
    if( pptr() > pbase() )
    {
	ostringstream strm ;
	strm << hex << setw( 4 ) << setfill( '0' ) << (unsigned int)(pptr() - pbase()) << "d" ;
	string tmp_str = strm.str() ;
	write( d_fd, tmp_str.c_str(), tmp_str.length() ) ;
	count += write( d_fd, d_buffer, pptr() - pbase() ) ;
	setp( d_buffer, d_buffer + d_bufsize ) ;
	// If something doesn't look right try using fsync
	fsync(d_fd);
    }
    return 0 ;
}

int
PPTStreamBuf::overflow( int c )
{
    sync() ;
    if( c != EOF )
    {
	*pptr() = static_cast<char>(c) ;
	pbump( 1 ) ;
    }
    return c ;
}

void
PPTStreamBuf::finish()
{
    sync() ;
    ostringstream strm ;
    ostringstream xstrm ;
    xstrm << "count=" << hex << setw( 8 ) << setfill( '0' ) << how_many() << ";" ;
    string xstr = xstrm.str() ;
    strm << hex << setw( 4 ) << setfill( '0' ) << (unsigned int)xstr.length() << "x" << xstr ;
    strm << hex << setw( 4 ) << setfill( '0' ) << (unsigned int)0 << "d" ;
    string tmp_str = strm.str() ;
    write( d_fd, tmp_str.c_str(), tmp_str.length() ) ;
    // If something doesn't look right try using fsync
    fsync(d_fd);
    count = 0 ;
}

