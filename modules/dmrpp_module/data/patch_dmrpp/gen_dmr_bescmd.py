import argparse as ap
import os

parser = ap.ArgumentParser(description='generate bescmd files for missing var fonc hdf5 file and the original hdf5 dmr file.')
parser.add_argument('-i',  nargs=1,
                    help='The dmrpp file of the original HDF5 file is provided. ',required=True)

args = parser.parse_args()
inputname = ''.join(args.i)
cur_dir=os.getcwd()
cur_fname=cur_dir+'/'+inputname

#miss_fname_bescmd=d_fname.split(".h5")[0]
#miss_fname_bescmd=miss_fname_bescmd+"_missing"+".h5.bescmd"
fname_bescmd=inputname+".bescmd"

dmrpp_fname = "template-dmr.bescmd"
dmrpp_file_t = open(dmrpp_fname,'r')
data_text = dmrpp_file_t.read()
dmrpp_file_t.close()

dfile_pos = '"catalog">'
index = data_text.find(dfile_pos)+len(dfile_pos)
#temp_text_1=data_text[:index]+d_fname+data_text[index:]
temp_text_1=data_text[:index]+inputname+data_text[index:]

bescmd_miss_file = open(fname_bescmd,'w')
bescmd_miss_file.write(temp_text_1)
bescmd_miss_file.close()


