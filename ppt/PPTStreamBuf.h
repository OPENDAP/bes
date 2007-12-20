#ifndef I_PPTStreamBuf_h
#define I_PPTStreamBuf_h 1

#include <streambuf>

class PPTStreamBuf : public std::streambuf
{
private:
    unsigned		d_bufsize ;
    int			d_fd ;
    char *		d_buffer ;
    unsigned int	count ;

			PPTStreamBuf()
			    : d_bufsize( 0 ),
			      d_buffer( 0 ),
			      count( 0 ) { }
public:
    			PPTStreamBuf(int fd, unsigned bufsize = 1) ;
  virtual		~PPTStreamBuf() ;

  unsigned int		how_many(){ return count ; }

  void			open( int fd, unsigned bufsize = 1 ) ;
  
  int			sync() ;
  
  int			overflow(int c) ;

  void			finish() ;
};

#endif // I_PPTStreamBuf_h 1

