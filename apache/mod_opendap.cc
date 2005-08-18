#include <unistd.h>
#include <iostream>

using std::cerr ;
using std::endl ;

#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_protocol.h"
#include "http_request.h"
#include "http_main.h"
#include "util_script.h"
#include "util_md5.h"

#include "DODSDataRequestInterface.h"
#include "DODSApacheWrapper.h"

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
    DODSDataRequestInterface rq;

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

    DODSApacheWrapper wrapper;
    const char* m_method = r->method;
    if (!m_method) 
    {
	cerr << "mod_opendap: Fatal, Can not load request method" << endl;
	return SERVER_ERROR;
    }

    if ( strcmp(m_method, "GET") == 0 ) 
    {
	if(r->parsed_uri.query)
	{
	    rq.request=wrapper.process_request(r->parsed_uri.query);
	    rq.cookie=wrapper.process_user(r->parsed_uri.query);
	}
	else
	{
	    rq.request=0;
	    rq.cookie=0;
	}
    }
    else if (strcmp(m_method, "POST") == 0 ) 
    {
	const char *post_data=0;
	util_read(r, &post_data);
	rq.request=wrapper.process_request(post_data);
	rq.cookie=wrapper.process_user(post_data);
    }
    else
    {
	rq.request=0;
	rq.cookie=0;
    }

    // These two lines will print out the header information to the error
    // log
    // list_headers( r ) ;
    // exit( 0 ) ;

    if( !rq.cookie || !strcmp(rq.cookie,"") )
    {
	rq.cookie = ap_table_get( r->headers_in, "Cookie" ) ;
    }

    wrapper.call_DODS(rq);

    // always flush the socket at the end...
    // and since stdout is now the tcp/ip socket for this connection
    fflush(stdout);

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

