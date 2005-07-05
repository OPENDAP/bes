// -*- C++ -*-

// (c) COPYRIGHT UCAR/HAO 1993-2002
// Please read the full copyright statement in the file COPYRIGHT.


#ifndef PPTSocket_h_
#define PPTSocket_h_ 1

#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <string>

using std::string ;

// Defines the packet size for the socket to be 256 bytes 
// (the buffer is 2 aligned for fast address access on alphas 2^8=256) 

//#define SIZE_COMMUNICATION_BUFFER 4096

// This is how sockaddr_in really looks like.
//
//  struct sockaddr_in {
//          sa_family_t     sin_family;
//          in_port_t       sin_port;
//          struct  in_addr sin_addr;
//          char            sin_zero[8];
//  };
// where:
//  struct in_addr {
//          union {
//                  struct { uint8_t s_b1, s_b2, s_b3, s_b4; } S_un_b ;
//                  struct { uint16_t s_w1, s_w2; } S_un_w ;
//                  uint32_t S_addr ;
//          } S_un ;
//  };
// typedef uint32_t        in_addr_t;
// typedef unsigned short  sa_family_t;
// typedef uint16_t        in_port_t;


#include "PPTException.h"


#include "PPTBuffer.h"

enum PPT_SOCKET_TYPE {PPT_TCP_SOCKET, PPT_UNIX_SOCKET} ;

class PPTSocket
{
protected:
    int			_socket ;
    bool		_set_from ;
    struct sockaddr_in	_from ;

    string		integerToString( int ) ;
    int			stringToInteger( const string& ) ;

public:

  enum PPT_SOCKET_TYPE _type;

  ///
  PPTSocket(int i=-1);
  /// Make the destructor virtual 
  virtual ~PPTSocket();


  /// Overloaded inserter for PPTBuffer.
  PPTSocket& operator << (const PPTBuffer &) throw (PPTException);
  /// Overloaded extractor for PPTBuffer.
  void operator >> (PPTBuffer &) throw (PPTException);

  ///
  void setAddress(const struct sockaddr_in &f)
  {
    
    _from=f;
    _set_from=true;
  }
  ///
  char* getAddress() throw (PPTException)
  {
    switch (_type)
      {
      case PPT_TCP_SOCKET:
	return inet_ntoa(_from.sin_addr);
	break;
      case PPT_UNIX_SOCKET:
	return "unix socket";
	break;
      default:
	throw PPTException(__FILE__,__LINE__,"Unknown socket type");
	break;

      }
      return 0 ;
  }
  

  /// 
  void setSocketDescriptor (int s)
  {
    _socket=s;
  }


  ///
  void closeSocket()
  {
    close(_socket);
  }
  ///
  int getSocketDescriptor() const
  {
    return (_socket);
  }
  ///
  bool operator ==(const PPTSocket &s) const
  {
    bool from_equal=false;
    if(
       (this->_from.sin_family==s._from.sin_family) &&
       (this->_from.sin_port==s._from.sin_port) &&
       // Ignore the sin_addr for now until I get to overload the == operator correctly
       //(this->_from.sin_addr==s._from.sin_addr) &&
       (string((char*)this->_from.sin_zero)==string((char*)s._from.sin_zero))
       )
      from_equal=true;
      
    if ( (this->_socket==s._socket) && (from_equal))
      return true;
    return false;
  }
};

#endif //PPTSocket_h_

