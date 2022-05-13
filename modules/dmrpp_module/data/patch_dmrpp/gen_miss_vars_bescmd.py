import argparse as ap
import os

parser = ap.ArgumentParser(description='generate bescmd files for missing var fonc hdf5 file and the original hdf5 dmr file.')
parser.add_argument('-i',  nargs=1,
                    help='The dmrpp file of the original HDF5 file is provided. ')
args = parser.parse_args()
inputname = ''.join(args.i)
cur_dir=os.getcwd()
cur_fname=cur_dir+'/'+inputname
d_fname=cur_fname.split(".dmrpp")[0]
h5_fname=inputname.split(".dmrpp")[0]

#miss_fname_bescmd=d_fname.split(".h5")[0]
#miss_fname_bescmd=miss_fname_bescmd+"_missing"+".h5.bescmd"
miss_fname_bescmd=d_fname+"_missing.bescmd"

miss_var_fname=d_fname+".missvar"
dmrpp_file = open(miss_var_fname,'r')
miss_var_text = dmrpp_file.read()
dmrpp_file.close()

dmrpp_fname = "template.bescmd"
dmrpp_file_t = open(dmrpp_fname,'r')
data_text = dmrpp_file_t.read()
dmrpp_file_t.close()

dfile_pos = '"catalog">'
index = data_text.find(dfile_pos)+len(dfile_pos)
#temp_text_1=data_text[:index]+d_fname+data_text[index:]
temp_text_1=data_text[:index]+h5_fname+data_text[index:]
dvar_pos = '<constraint>'
index = temp_text_1.find(dvar_pos)+len(dvar_pos)
temp_text_2=temp_text_1[:index]+miss_var_text+temp_text_1[index:]

bescmd_miss_file = open(miss_fname_bescmd,'w')
bescmd_miss_file.write(temp_text_2)
bescmd_miss_file.close()

dmrpp_fname = "template-dmr.bescmd"
dmrpp_file_t = open(dmrpp_fname,'r')
data_text = dmrpp_file_t.read()
dmrpp_file_t.close()

dfile_pos = '"catalog">'
index = data_text.find(dfile_pos)+len(dfile_pos)
#hdf5_fname=cur_fname.split(".dmrpp")[0]
hdf5_fname=h5_fname.split(".dmrpp")[0]
dot_index = hdf5_fname.rfind('.')
hdf5_fname_p1 = hdf5_fname[:dot_index]
#print(hdf5_fname_p1)

#temp_text_1=data_text[:index]+hdf5_fname.split(".h5")[0]+"_missing.h5"+data_text[index:]
temp_text_1=data_text[:index]+hdf5_fname_p1+"_missing.h5"+data_text[index:]
hdf5_dmr_bescmd_fname = hdf5_fname_p1+"_missing.h5.dmr.bescmd"
hdf5_dmr_file = open(hdf5_dmr_bescmd_fname,'w')
hdf5_dmr_file.write(temp_text_1)
hdf5_dmr_file.close()

