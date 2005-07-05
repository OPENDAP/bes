// ProcIdResponseHandler.cc

// 2004 Copyright University Corporation for Atmospheric Research

#include <unistd.h>

#include "ProcIdResponseHandler.h"
#include "DODSTextInfo.h"
#include "cgi_util.h"
#include "util.h"
#include "DODSParserException.h"
#include "DODSTokenizer.h"

ProcIdResponseHandler::ProcIdResponseHandler( string name )
    : DODSResponseHandler( name )
{
}

ProcIdResponseHandler::~ProcIdResponseHandler( )
{
}

void
ProcIdResponseHandler::parse( DODSTokenizer &tokenizer,
                           DODSDataHandlerInterface &dhi )
{
    string my_token = tokenizer.get_next_token() ;
    if( my_token != ";" )
    {
	tokenizer.parse_error( my_token + " not expected" ) ;
    }
}

char *
ProcIdResponseHandler::fastpidconverter(
      long val,                                 /* value to be converted */
      char *buf,                                /* output string         */
      int base)                                 /* conversion base       */
{
      ldiv_t r;                                 /* result of val / base  */

      if (base > 36 || base < 2)          /* no conversion if wrong base */
      {
            *buf = '\0';
            return buf;
      }
      if (val < 0)
            *buf++ = '-';
      r = ldiv (labs(val), base);

      /* output digits of val/base first */

      if (r.quot > 0)
            buf = fastpidconverter ( r.quot, buf, base);
      /* output last digit */

      *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem];
      *buf   = '\0';
      return buf;
}

void
ProcIdResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    char mypid[12] ;
    fastpidconverter( getpid(), mypid, 10 ) ;
    info->add_data( (string)mypid + "\n" ) ;
}

void
ProcIdResponseHandler::transmit( DODSTransmitter *transmitter,
                                  DODSDataHandlerInterface &dhi )
{
    if( _response )
    {
	DODSTextInfo *info = dynamic_cast<DODSTextInfo *>(_response) ;
	transmitter->send_text( *info, dhi ) ;
    }
}

DODSResponseHandler *
ProcIdResponseHandler::ProcIdResponseBuilder( string handler_name )
{
    return new ProcIdResponseHandler( handler_name ) ;
}

// $Log: ProcIdResponseHandler.cc,v $
// Revision 1.2  2005/03/15 19:58:35  pwest
// using DODSTokenizer to get first and next tokens
//
// Revision 1.1  2005/02/01 17:48:17  pwest
//
// integration of ESG into opendap
//
