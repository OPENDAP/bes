#ifndef DODSClient_h_
#define DODSClient_h_ 1

#ifdef __cplusplus

#include <iostream>
#include <string>

//forward declaration of PPTBuffer
class PPTBuffer;

#include "PPTSocket.h"

/**
 */

class DODSClient
{
  int __timeout;
  ///
  int __debugging;
  ///
  bool __connected;
  /// 
  bool __broken_pipe;
  ///
  PPTSocket __pptsocket;
  ///
  string tem;
  ///
  void init_connection();
  ///
  bool server_complete_transmition(PPTBuffer *b);
  ///
  static void signal_broken_pipe(int sig);
  ///
  static void (*_previous_handler)(int) ;

public:
  ///
  DODSClient(const char*host, int port, int debug, int timeout=5) throw();
  ///
  DODSClient (const char *unix_socket, int debug, int timeout=5) throw();
  ///
  ~DODSClient();
  ///
  bool is_connected() { return __connected; }
  ///
  void close_connection();
  ///
  void clean_state();
  ///
  bool write_buffer(PPTBuffer &buf) throw();
  ///
  bool read_buffer(PPTBuffer *buf, bool &eof) throw();
  /// Signal Management is always tricky, instead of registering
  /// between this class a handler for SIGPIPE, the class
  /// and thus the library where it lives leaves the control
  /// of signals to the higher level code, i.e., the code
  /// relying on this class for communications. However,
  /// even that there is more flexibility in catching SIGPIPE
  /// outside you must notify  this class of the problem so
  /// the destructor can perform correctly as follows:
  /// 1. If there has been a SIGPIPE, do not try to write
  ///    any TFTP control element, just set __connected 
  ///    to false, otherwise, close according normal procedure.
  /// 2. always clean temporary files in case connection type
  ///    is Unix Socket.
  void broken_pipe (){__broken_pipe=true;}
  

};

#else


/** -- C -- methods wraping the DODSClient class */

/**
   Initialize function.
   Creates a new DODSClient object and returns a handle 
   to it as a void pointer. Other methods provided here
   need this pointer so the can invoke the methods of the
   class DODSClient.
*/
void* dodsclient_initialize(const char *host, int port, int debug);

/** see if the client is connected */
int dodsclient_is_connected(void *dods_client_handler);

/** close the connection with the DODS server */
void dodsclient_close_connection(void *dods_client_handler);

/** destroy the DODS client object created by the initialize function */
void dodsclient_destroy(void *dods_client_handler);

/** 
    Write a buffer of data using the DODSClient object handled by p
    It assumes that len is the rigth length of buffer buf.
    Return 1 (TRUE) if the operation was completed succesfully, 0( FALSE) otherwise.
    The DODS server currently reads a single buffer of data
    and DOES NOT WAIT for the PPTCLIENT_COMPLETE_DATA_TRANSMITION
    (see PPT protocol) thus you must put everything you want the server
    to do in a single buffer, write it to DODS and start reading the
    response rigth away using the method dodsclient_read_buffer.
 */
int dodsclient_write_buffer(void *dods_client_handler, unsigned char* buf, int len);

/** */
void* dodsclient_new_data_buffer(int size);

/** */
void dodsclient_destroy_data_buffer(void *data_buffer);

/** */
int dodsclient_read_buffer(void *dods_client_handler, void *data_buffer, int *eof);

/** */
unsigned char* dodsclient_get_data_from_data_buffer(void *data_buffer);

/** */
int dodsclient_get_size_of_data_buffer(void *data_buffer);

#endif // __cplusplus

#endif // DODSClient_h_

