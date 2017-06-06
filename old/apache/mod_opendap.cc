// mod_opendap.cc

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
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <unistd.h>
#include <iostream>

using std::cerr ;
using std::endl ;
using std::cout ;
using std::flush ;

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_main.h"
#include "util_script.h"
#include "util_md5.h"

#include "BESDataRequestInterface.h"
#include "BESApacheWrapper.h"

char * ltoa(long val, char *buf,int base)
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
    buf = ltoa ( r.quot, buf, base);

  /* output last digit */

  *buf++ = "0123456789abcdefghijklmnopqrstuvwxyz"[(int)r.rem];
  *buf   = '\0';
  return buf;
}


static int util_read(request_rec *r, const char **rbuf)
{
    int rc = OK;

    if ((rc = ap_setup_client_block(r, REQUEST_CHUNKED_ERROR)))
    {
	return rc;
    }

    if (ap_should_client_block(r))
    {
	char argsbuffer[HUGE_STRING_LEN];
	int rsize, len_read, rpos=0;
	long length = r->remaining;
	*rbuf = (char*) ap_pcalloc(r->pool, length + 1);

	ap_hard_timeout("util_read", r);

	while((len_read = ap_get_client_block(r, argsbuffer, sizeof(argsbuffer))) > 0)
	{
	    ap_reset_timeout(r);
	    if ((rpos + len_read) > length)
	    {
		rsize = length - rpos;
	    }
	    else
	    {
		rsize = len_read;
	    }
	    memcpy((char*)*rbuf + rpos, argsbuffer, rsize);
	    rpos += rsize;
	}

	ap_kill_timeout(r);
    }
    return rc;
}

static int
header_trace( void *data, const char *key, const char *val )
{
    request_rec *r = (request_rec *)data ;
    cerr << "Header Field '" << key << "' = '" << val << "'" << endl ;
    return TRUE ;
}

static void
list_headers( request_rec *r )
{
    ap_table_do( header_trace, r, r->headers_in, NULL ) ;
}

static int opendap_handler(request_rec *r)
{
    char port_number_buffer[80];
    dup2(r->connection->client->fd,STDOUT_FILENO);
    BESDataRequestInterface rq;

    // BEGIN Initialize all data request elements correctly to a null pointer
    rq.server_name=0;
    rq.server_address=0;
    rq.server_protocol=0;
    rq.server_port=0;
    rq.script_name=0;
    rq.user_address=0;
    rq.user_agent=0;
    rq.request=0;
    // END Initialize all the data request elements correctly to a null pointer

    rq.server_name=ap_get_server_name(r);
    rq.server_address="jose";
    rq.server_protocol=r->protocol;
    ltoa(ap_get_server_port(r), port_number_buffer, 10);
    rq.server_port=port_number_buffer;
    rq.script_name=r->uri;
    rq.user_address=r->connection->remote_ip;
    rq.user_agent = ap_table_get(r->headers_in, "User-Agent");

    const char* m_method = r->method;
    if (!m_method)
    {
	cerr << "mod_opendap: Fatal, Cannot load request method" << endl;
	return SERVER_ERROR;
    }

    BESApacheWrapper wrapper;
    if ( strcmp(m_method, "GET") == 0 )
    {
	if(r->parsed_uri.query)
	{
	    wrapper.process_request(r->parsed_uri.query);
	    rq.cookie=wrapper.process_user(r->parsed_uri.query);
	    rq.token=wrapper.process_token(r->parsed_uri.query);
	}
	else
	{
	    rq.request=0;
	    rq.cookie=0;
	    rq.token=0;
	}
    }
    else if (strcmp(m_method, "POST") == 0 )
    {
	const char *post_data=0;
	util_read(r, &post_data);
	wrapper.process_request(post_data);
	rq.cookie=wrapper.process_user(post_data);
	rq.token=wrapper.process_token(r->parsed_uri.query);
    }
    else
    {
	rq.request=0;
	rq.cookie=0;
	rq.token=0;
    }

    // These two lines will print out the header information to the error
    // log
    // list_headers( r ) ;
    // exit( 0 ) ;

    if( !rq.cookie || !strcmp(rq.cookie,"") )
    {
	rq.cookie = ap_table_get( r->headers_in, "Cookie" ) ;
    }

    int status = 0 ;
    rq.request = wrapper.get_first_request() ;
    while( rq.request && status == 0 )
    {
	status = wrapper.call_BES(rq);
	rq.request = wrapper.get_next_request() ;
    }

    // always flush the socket at the end...
    // and since stdout is now the tcp/ip socket for this connection
    cout.flush() ;

    // exit instead of returning
    exit( 0 ) ;

    return OK;
}

/* Make the name of the content handler known to Apache */
static handler_rec opendap_handlers[] =
{
    {"opendap-handler", opendap_handler},
    {NULL}
};

/* Tell Apache what phases of the transaction we handle */
module MODULE_VAR_EXPORT opendap_module =
{
    STANDARD_MODULE_STUFF,
    NULL,               /* module initializer                 */
    NULL,               /* per-directory config creator       */
    NULL,               /* dir config merger                  */
    NULL,               /* server config creator              */
    NULL,               /* server config merger               */
    NULL,               /* command table                      */
    opendap_handlers,      /* [7]  content handlers              */
    NULL,               /* [2]  URI-to-filename translation   */
    NULL,               /* [5]  check/validate user_id        */
    NULL,               /* [6]  check user_id is valid *here* */
    NULL,               /* [4]  check access by host address  */
    NULL,               /* [7]  MIME type checker/setter      */
    NULL,               /* [8]  fixups                        */
    NULL,               /* [9]  logger                        */
    NULL,               /* [3]  header parser                 */
    NULL,               /* process initialization             */
    NULL,               /* process exit/cleanup               */
    NULL                /* [1]  post read_request handling    */
};

