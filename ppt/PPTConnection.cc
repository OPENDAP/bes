// PPTConnection.cc

// 2005 Copyright University Corporation for Atmospheric Research

#include "PPTConnection.h"
#include "PPTProtocol.h"
#include "Socket.h"

void
PPTConnection::send( const string &buffer )
{
    if( buffer != "" )
    {
	writeBuffer( buffer ) ;
    }
    writeBuffer( PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION ) ;
}

void
PPTConnection::sendExit()
{
    writeBuffer( PPTProtocol::PPT_EXIT_NOW ) ;
}

void
PPTConnection::writeBuffer( const string &buffer )
{
    _mySock->send( buffer, 0, buffer.length() ) ;
}
    
bool
PPTConnection::receive( ostream *strm )
{
    bool isDone = false ;
    ostream *use_strm = _out ;
    if( strm )
	use_strm = strm ;

    bool done = false ;
    while( done == false )
    {
	char *inBuff = new char[4097] ;
	int bytesRead = readBuffer( inBuff ) ;
	if( bytesRead != 0 )
	{
	    int exitlen = PPTProtocol::PPT_EXIT_NOW.length() ;
	    if( !strncmp( inBuff, PPTProtocol::PPT_EXIT_NOW.c_str(), exitlen ) )
	    {
		done = true ;
		isDone = true ;
	    }
	    else
	    {
		int termlen =
			PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION.length() ;
		int writeBytes = bytesRead ;
		if( bytesRead >= termlen )
		{
		    string inEnd ;
		    for( int j = 0; j < termlen; j++ )
			inEnd += inBuff[(bytesRead - termlen) + j] ;
		    if( inEnd == PPTProtocol::PPT_COMPLETE_DATA_TRANSMITION )
		    {
			done = true ;
			writeBytes = bytesRead-termlen ;
		    }
		}
		for( int j = 0; j < writeBytes; j++ )
		{
		    if( use_strm )
		    {
			(*use_strm) << inBuff[j] ;
		    }
		}
	    }
	}
	else
	{
	    done = true ;
	}
	delete [] inBuff ;
    }
    return isDone ;
}

int
PPTConnection::readBuffer( char *inBuff )
{
    return _mySock->receive( inBuff, 4096 ) ;
}

// $Log: PPTConnection.cc,v $
