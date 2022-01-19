#!/usr/bin/env python
from pydap.client import open_dods
import numpy as np
import argparse as ap
parser = ap.ArgumentParser(description='Use pydap client to access HDF5 files via Hyrax ')
parser.add_argument('url',  nargs='?',default='https://eosdap.hdfgroup.org:8080/opendap/data/hdf5/grid_1_2d.h5.dods?temperature[0:1:1][0:1:1]',help='The URL to access the HDF5 files via Hyrax')

#Change the URL to your own server's address
args=parser.parse_args()
input_url =''.join(args.url)
#Obtain the data via pydap
dataset=open_dods(input_url)
data=[]
data=dataset.temperature[:]
#Generate the output file name from URL
last_slash_index=input_url.rfind('/')
h5_suffix_index=input_url.find(".h5",last_slash_index)
output_string=input_url[last_slash_index+1:h5_suffix_index]

#Save the data with numpy savetxt
np.savetxt(output_string+'_data.txt',data)
