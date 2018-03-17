#!/usr/bin/env python

from pydap.client import open_url
import json
import argparse as ap
parser = ap.ArgumentParser(description='Use pydap client to access HDF5 files via Hyrax ')
parser.add_argument('url',  nargs='?',default='https://eosdap.hdfgroup.org:8080/opendap/data/hdf5/grid_1_2d.h5',help='The URL to access the HDF5 files via Hyrax')

#Change the URL to your own server's address
args=parser.parse_args()
input_url =''.join(args.url)

#Replace the URL with your own server
dataset=open_url(input_url)
#dataset=open_url('https://eosdap.hdfgroup.org:8080/opendap/data/hdf5/grid_1_2d.h5')
#dataset=open_url('https://eosdap.hdfgroup.org:8080/opendap/data/NASAFILES/hdf5/MLS-Aura_L2GP-BrO_v04-23-c03_2016d302.he5')
#dataset=open_url('https://eosdap.hdfgroup.org:8989/opendap/data/test/kent/hdf5_handler_fake/d_int.h5')

#Root or group attributes saved to a dictionary 
Attr_DictT={}
Attr_DictT["Root_or_Group_Attributes"]=dataset.attributes

#Wrapper to save information of all variables
Var_DictT={}

#Information of all variables 
Var_Dict={}

#Loop through datatype,datashape and attribute information of all variables
for k,v in dataset.items():
    temp_dtype="datatype: "+str(dataset[k].dtype)
    temp_dshape="data shape: "+str(dataset[k].shape)
    temp_content=[temp_dtype,temp_dshape]
    temp_var_attr={}
    temp_var_attr["Attributes"]=dataset[k].attributes
    temp_content.append(temp_var_attr)
    Var_Dict[k]=temp_content	
Var_DictT["Variables"]=Var_Dict
#Final information
File_List=[Attr_DictT,Var_DictT]

#Generate the output file name out of the URL
last_slash_index=input_url.rfind('/')
h5_suffix_index=input_url.find(".h5",last_slash_index)
output_string=input_url[last_slash_index+1:h5_suffix_index]

##Add 'default' in hte output file name for the default option
if "d_int" in output_string:
    output_string=output_string+"_default"
output_string=output_string+'_ddsdas.txt'

#Write the output in json
with open(output_string,'w')as f:
    json.dump(File_List,f,indent=4,separators=(',',':'))

#The following code doesn't work for python 2.7
#with open('Grid_1_2d.h5_ddsdas.json','w',encoding='utf8')as f:
#	json.dump(File_List,f,indent=4,separators=(',',':'))


