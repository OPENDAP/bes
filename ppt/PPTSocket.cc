
// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.


#include <iostream>

using std::cerr ;
using std::cout ;
using std::endl ;

#include "PPTConfig.h"

#include "PPTSocket.h"
#include "PPTException.h"

extern "C" { extern int errno; }

PPTSocket::PPTSocket( int i )
    : _socket( i ),
      _set_from( false )
{
}

PPTSocket::~PPTSocket()
{
}

extern int debug_server;

PPTSocket&
PPTSocket::operator<<( const PPTBuffer &b ) throw( PPTException )
{
    if(debug_server)
	{
	    cout<<"\n---------------- PPTSocket write trace begin --------------\n"
		<<"(PID) "<<getpid()<<": writing in socket "<<b.valid<<" bytes.\n"
		<<"---------------- PPTSocket trace end ----------------"
		<<endl;
	}
    //Lets get sure that the string is small enough to fit in the buffer
    if (write(_socket,b.data, b.valid)==-1)
	{
	    string error("socket failure, writing on stream socket");
	    const char* error_info=strerror(errno);
	    if (error_info)
		error+=" "+(string)error_info;
	    throw PPTException(__FILE__,__LINE__,error);
	}
    return *this;
}


void PPTSocket::operator>>(PPTBuffer &b)  throw (PPTException)
{
    int rval=0;
    if ((rval=read(_socket, b.data, b.get_size()))>0)
	{
	    b.valid=rval;
	    if(debug_server)
		{
		    cout<<"\n---------------- PPTSocket read trace begin --------------\n"
			<<"(PID) "<<getpid()<<": reading from  socket "<<b.valid<<" bytes.\n"
			<<"---------------- PPTSocket trace end ----------------"
			<<endl;
		}
	}
    else
	{
	    string error("socket failure, reading on stream socket");
	    const char* error_info=strerror(errno);
	    if (error_info)
		error+=" "+(string)error_info;
	    throw PPTException(__FILE__,__LINE__,error);
	}
}

