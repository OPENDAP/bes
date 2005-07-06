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

/** @brief parses the request 'show process;'
 *
 * The syntax for a request handled by this response handler is 'show
 * process;'. The keywords 'show' and 'process' have already been
 * parsed, which is how we got to the parse method. This method makes sure
 * that the command is terminated by a semicolon and that there is no more
 * text after the keyword 'process'.
 *
 * @param tokenizer holds on to the list of tokens to be parsed
 * @param dhi structure that holds request and response information
 * @throws DODSParserException if there is a problem parsing the request
 * @see DODSTokenizer
 * @see _DODSDataHandlerInterface
 */
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

/** @brief executes the command 'show process;' by returning the process id of
 * the server process
 *
 * This response handler knows how to retrieve the process id for the server
 * and stores it in a DODSTextInfo informational response object.
 *
 * @param dhi structure that holds request and response information
 * @throws DODSResponseException if there is a problem building the
 * response object
 * @see _DODSDataHandlerInterface
 * @see DODSTextInfo
 */
void
ProcIdResponseHandler::execute( DODSDataHandlerInterface &dhi )
{
    DODSTextInfo *info = new DODSTextInfo( dhi.transmit_protocol == "HTTP" ) ;
    _response = info ;
    char mypid[12] ;
    fastpidconverter( getpid(), mypid, 10 ) ;
    info->add_data( (string)mypid + "\n" ) ;
}

/** @brief transmit the response object built by the execute command
 * using the specified transmitter object
 *
 * If a response object was built then transmit it as text.
 *
 * @param transmitter object that knows how to transmit specific basic types
 * @param dhi structure that holds the request and response information
 * @see DODSResponseObject
 * @see DODSTransmitter
 * @see _DODSDataHandlerInterface
 */
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
