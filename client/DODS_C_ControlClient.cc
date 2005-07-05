#include "DODSClient.h"


#include "PPTClientServerSessionProtocol.h"
#include "PPTException.h"
#include "PPTSocket.h"


// -- C++ --
#include <iostream>
#include <fstream>

using std::cerr ;
using std::endl ;

#include "DODSClientConfig.h"

/** 
    If this is defined, it means every DODSClient pointer
    and any PPTBuffer pointer allocated with this
    functions will be checked against a list of pointer 
    we mantain. This add security and reduces performance.
    Remember, static_cast will not fail to cast to
    object of type X pointers to class Y even if that is 
    incorrect.
*/
#ifdef PROCESS_L_CONTROL

// -- C++ Standard Template Library (STL) --
#include <vector>
#include <algo.h>

vector < void* > list_clients;
vector < void* > list_buffers;
vector < void* > list_ppt_buffers; 

#endif // PROCESS_L_CONTROL

#ifdef  DODS_C_CONTROL

#ifdef __cplusplus 



DODSClient* cast_pointer_to_dodsclient(void *p)
{
  if(!p)
    {
      cerr<<__FILE__<<":"<<__LINE__<<"NULL Pointer!"<<endl;
      exit(1);
    }
#ifdef PROCESS_L_CONTROL
  vector< void*  >::iterator _it;
  _it=find(list_clients.begin(),list_clients.end(), p);
  if (_it==list_clients.end())
    {
      cerr<<__FILE__<<":"<<__LINE__<<": incoming pointer is not DODSClient type."<<endl;
      return 0;
    }
#endif

  DODSClient* obj=static_cast<DODSClient*> (p);
  if (obj)
    return obj;
  else
    cerr<<__FILE__<<":"<<__LINE__<<": incoming pointer is not DODSClient type."<<endl;
  return 0;
}

PPTBuffer* cast_pointer_to_pptbuffer(void *p)
{
  if(!p)
    {
      cerr<<__FILE__<<":"<<__LINE__<<"NULL Pointer!"<<endl;
      exit(1);
    }
#ifdef PROCESS_L_CONTROL
  vector< void*  >::iterator _it;
  _it=find(list_ppt_buffers.begin(),list_ppt_buffers.end(), p);
  if (_it==list_ppt_buffers.end())
    {
      cerr<<__FILE__<<":"<<__LINE__<<": incoming pointer is not PPTBuffer type."<<endl;
      return 0;
    }
#endif
  PPTBuffer* obj=static_cast<PPTBuffer*> (p);
  if (obj)
    return obj;
  else
    cerr<<__FILE__<<":"<<__LINE__<<": incoming pointer is not PPTBuffer type."<<endl;
  return 0;
}

extern "C" // Get sure the C++ compiler mangles this names C style for the linker to work.
{
  
  void* dodsclient_initialize(const char *host, int port, int debug)
  {
    if ( (debug!=0) && (debug!=1) )
      {
	cerr<<__FILE__<<":"<<__LINE__<<": debug variable is neither 0 nor 1\n";
	exit(1);
      }
    DODSClient *cl= new DODSClient(host,port, debug, 5);
#ifdef PROCESS_L_CONTROL
    list_clients.push_back(cl);
#endif
    return cl;
  }
  
  int dodsclient_is_connected(void *p)
  {
    DODSClient *client = cast_pointer_to_dodsclient(p);
    
    if(client)
      {
	if ( client->is_connected() )
	  return 1;
      }
    
    return 0;
  }


  void dodsclient_close_connection(void *p)
  {
    DODSClient *client = cast_pointer_to_dodsclient(p);
    
    if(client)
      {
	if (client->is_connected())
	  client->close_connection();
      }
  }

  void dodsclient_destroy(void *p)
  {   
    DODSClient *client = cast_pointer_to_dodsclient(p);
 
    if(client)

#ifdef PROCESS_L_CONTROL   
      {
	vector< void*  >::iterator _it;
	_it=find(list_clients.begin(),list_clients.end(), p);
	// we can go ahead and delete because we already checked that is in the list
	list_clients.erase(_it);
	delete client;
	return;
      }
#endif 
   
    delete client;
  }
  
  int dodsclient_write_buffer(void *p, unsigned char* buf, int len)
  {
    // Load the buffer nicely
    PPTBuffer pptbuffer(len);
    memcpy((char*)pptbuffer.data,buf,len);
    pptbuffer.valid=len;

    DODSClient *client = cast_pointer_to_dodsclient(p);

    if (client)
      if ( client->write_buffer(pptbuffer) )
	return 1;
    return 0;
  }
  
  void* dodsclient_new_data_buffer(int size)
  {
    PPTBuffer *p=new PPTBuffer(size);
#ifdef PROCESS_L_CONTROL
     list_ppt_buffers.push_back(p);
#endif
     return (void*) p;
  }
  
  void dodsclient_destroy_data_buffer(void *p)
  {
    PPTBuffer *pptbuffer=cast_pointer_to_pptbuffer(p);
    if(pptbuffer)
#ifdef PROCESS_L_CONTROL   
      {
	vector< void*  >::iterator _it;
	_it=find(list_ppt_buffers.begin(),list_ppt_buffers.end(), p);
	// we can go ahead and delete because we already checked that is in the list
	list_ppt_buffers.erase(_it);
	delete pptbuffer;
	return;
      }
#endif 
    delete pptbuffer;
  }
  
  int dodsclient_read_buffer(void *p, void *data_buffer, int *eof)
  {
    DODSClient *client = cast_pointer_to_dodsclient(p);
    PPTBuffer *pptbuffer=cast_pointer_to_pptbuffer(data_buffer);

    bool _eof;
    
    if(client && pptbuffer)
      if ( !client->read_buffer(pptbuffer, _eof) )
	return 0;

    if (_eof)
      *eof=1;
    else
      *eof=0;

    return 1;
  }

  unsigned char* dodsclient_get_data_from_data_buffer(void *p)
  {
    PPTBuffer *pptbuffer=cast_pointer_to_pptbuffer(p);
    if(pptbuffer)
      return pptbuffer->data;
    else
      return 0;
  }

  int dodsclient_get_size_of_data_buffer(void *p)
  {
    PPTBuffer *pptbuffer=cast_pointer_to_pptbuffer(p);
    if(pptbuffer)
      return pptbuffer->valid;
    else
      return -1;
  }

}




#endif // __cplusplus

#endif //  DODS_C_CONTROL
