#include "DODSClientXDRControl.h"

#include <unistd.h>
extern int errno;

#include <iostream>
#include <fstream>

using std::cerr ;
using std::cout ;
using std::endl ;
using std::ofstream ;
using std::ifstream ;

#define DAP_DATA "dap.data"
#define XDR_DATA "XDR.data"

bool DODSClientXDRControl::load_source()
{
  output = fopen("XDR.data", "r+");       
  if (!output)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": can not open XDR data source."<<endl;
      return false;
    }
  if (!source)
    {
      source = new XDR;
      enum xdr_op xop = XDR_DECODE;
      xdrstdio_create(source, output, xop);
    }
  return true;
}

void DODSClientXDRControl::unload_source()
{
  if(source)
    {
      xdr_destroy(source);
      delete source;
      source=0;
    }
  if(output)
    {
      fclose(output);
      output=0;
    }
  if(!access(DAP_DATA, F_OK))
    remove (DAP_DATA);
  if(!access(XDR_DATA, F_OK))
    remove (XDR_DATA);
}


// Searching for the string separator 'Data:'

bool DODSClientXDRControl::seek_data_separator(const unsigned char *buffer, unsigned int size_of_buffer, unsigned int &size_of_dap, unsigned char* &datax, unsigned int &size_of_datax)
{
    int seek_size=size_of_buffer;
    unsigned char* pt=(unsigned char*) memchr(buffer, 'D', seek_size);

    while (pt)
	{
	    unsigned char* posible_begin_of_string=pt;
	    pt++; 
	    if (*pt=='a')
		{
		    pt++;
		    if (*pt=='t')
			{
			    pt++;
			    if (*pt=='a')
				{
				    pt++;
				    if (*pt==':')
					{
					    //before assigning data portion
					    //increment pt pointer twice to get rid of ':' and seek_ line
					    pt++; datax=++pt;

					    size_of_dap=((posible_begin_of_string-(buffer))/sizeof(unsigned char));
					    size_of_datax = size_of_buffer - (size_of_dap + strlen("Data:\n"));

					    return true;
					}
				}
			}
		}
	    seek_size= size_of_buffer - ((pt-buffer)/sizeof(unsigned char));
	    unsigned char *temp=pt;
	    pt=(unsigned char*) memchr(temp, 'D', seek_size);
	}

    return false;
}

bool DODSClientXDRControl::separate_dap_from_data(const char* source_file)
{
  ifstream source(source_file);
  if(!source)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": can not open source file."<<endl;
      return false;
    }

  ofstream temp_dap_file(DAP_DATA);
  if(!temp_dap_file)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": can not open temp file for dap portion."<<endl;
      return false;
    }

  ofstream temp_XDR_file(XDR_DATA);
  if(!temp_XDR_file)
    {
      cerr<<__FILE__<<":"<<__LINE__<<": can not open temp file for XDR portion."<<endl;
      return false;
    }

  const int size_of_buffer=4096;
  unsigned char buffer[size_of_buffer];

  unsigned int size_of_dap;
  unsigned char* datax;
  unsigned int size_of_datax;
  unsigned int number_of_bytes_readed;
  
  bool found_data_separator=false;
  
  while(!source.eof())
    {
      source.read((char *)buffer,size_of_buffer);
      number_of_bytes_readed=source.gcount();
      
      if (!found_data_separator)
	{
	  found_data_separator=seek_data_separator(buffer, number_of_bytes_readed, size_of_dap, datax, size_of_datax);
	  if (found_data_separator)
	    {
	      temp_dap_file.write((char *)buffer,size_of_dap);
	      temp_XDR_file.write((char *)datax,size_of_datax);
	    }
	  else
	    temp_dap_file.write((char *)buffer,number_of_bytes_readed);
	}
      else
	temp_XDR_file.write((char *)buffer,number_of_bytes_readed);
	  
    }
  return true;
}


