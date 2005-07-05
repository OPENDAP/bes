// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2003
// Please read the full copyright statement in the file COPYRIGHT.

#include <string>
#include <iostream>

using std::string ;
using std::flush ;

#include "DODSClient.h"
#include "DODSDataRequestInterface.h"
#include "DODSProcessEncodedString.h"
#include "OPeNDAPConnector.h"

// Host and port where OPeNDAP server is running
const char* DODS_HOST="cedar-l.hao.ucar.edu";
const int DODS_PORT=2003;
 
/**
   If you want to read a lot of trace messages from this module
   set this variable to 1
 */
const int debug_module = 1;

/**
   If you want to read yet a lot more messages from the dodsclient
   set this variable to 1
 */
const int debug_dods_client = 0;

OPeNDAPConnector::OPeNDAPConnector()
{
  _data_request=0;
}

OPeNDAPConnector::~OPeNDAPConnector()
{
  if(_data_request)
    {
      delete [] _data_request;
      _data_request=0;
    }
}

const char * OPeNDAPConnector::process_request(const char*s)
{
  DODSProcessEncodedString h(s);
  string str=h.get_key("request");
  _data_request=new char [strlen(str.c_str())+1];
  strcpy(_data_request,str.c_str());
  return _data_request;
}

int OPeNDAPConnector::execute(const DODSDataRequestInterface & re)
{
  cout<<"HTTP/1.0 200 OK\n";
  cout<<"Content-type: text/plain\n";
  cout<<"Content-Description: OPeNDAP Response\n\n";
  cout<<flush;
  DODSClient *dods_client= new DODSClient(DODS_HOST, DODS_PORT, debug_dods_client, 5);

  if (!dods_client->is_connected())
      {
	cout<<"Fatal, failed to get connected to OPeNDAP!\n";
	delete dods_client;
	return DODS_TERMINATE_IMMEDIATE;
      }
  else
    {
      if (!re.request)
	{
	  cout<<"Fatal, failed to get a proper request!\n";
	  delete dods_client;
	  return DODS_TERMINATE_IMMEDIATE;
	}	
      int len=strlen(re.request);
      PPTBuffer pptbuffer(len);
      memcpy((char*)pptbuffer.data,re.request,len);
      pptbuffer.valid=len;	;
      if (!dods_client->write_buffer(pptbuffer))
	{
	  cout<<"Fatal, failed to submit command to OPeNDAP!\n";
	  dods_client->close_connection();
	  delete dods_client;
	  return DODS_TERMINATE_IMMEDIATE;
	}
      bool dods_server_completed_transmition=false;
      while (!dods_server_completed_transmition)
	{
	  PPTBuffer b(4096);
	  if (!dods_client->read_buffer(&b, dods_server_completed_transmition))
	    {
	      cout<<"Fatal, failed to submit command to OPeNDAP!\n";
	      dods_client->close_connection();
	      delete dods_client;
	      return DODS_TERMINATE_IMMEDIATE;
	    }
	   cout.write((const char *)b.data, b.valid);
	}
      dods_client->close_connection();
      delete dods_client;
    }
  return DODS_EXECUTED_OK;
}
