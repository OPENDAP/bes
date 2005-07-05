#ifndef DODSClientXDRControl_h_
#define DODSClientXDRControl_h_ 1

#include <rpc/types.h>
#include <netinet/in.h>
#include <rpc/xdr.h>

#include <string.h>

#include <fstream>


class DODSClientXDRControl
{
  ///
  XDR *source;
  ///
  FILE *output;	
  
  // Disable copy constructor
   DODSClientXDRControl(DODSClientXDRControl &);
  // Disable asignment operator
  DODSClientXDRControl & operator = (const DODSClientXDRControl &);

public:
  
  ///
  DODSClientXDRControl(): source(0), output(0) {};
  ///
  ~DODSClientXDRControl() {  unload_source(); }
  ///
  bool load_source();
  ///
  void unload_source();
  ///
  XDR * get_source() {return source; }
  ///
  bool seek_data_separator(const unsigned char *buffer, unsigned int size_of_buffer, unsigned int &size_of_dap, unsigned char* &datax, unsigned int &size_of_datax);
  ///
  bool separate_dap_from_data(const char* source_file);
};

#endif // DODSClientXDRControl_h_
