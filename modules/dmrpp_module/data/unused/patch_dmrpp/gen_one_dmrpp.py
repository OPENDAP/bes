import argparse as ap
import subprocess
import sys

parser = ap.ArgumentParser(description='generate the original dmrpp file based on the HDF5 file name.')
parser.add_argument('-i',  nargs=1,
                    help='The  HDF5 file is provided. ',required=True)
parser.add_argument('-u',  type=str,
                    help='The URL or path of this dmrpp file resides. Example: -u data/dmrpp ')

args = parser.parse_args()
f = ''.join(args.i)
#Generate the dmr file.
ret = subprocess.run(["python","gen_dmr_bescmd.py","-i",f])
if ret.returncode!=0:
    print("Cannot generate the dmrp file, stop.")
    sys.exit(1)

# Generate the bes.conf file.
# Copy the template of the bes configuration file to the one we want to use
ret=subprocess.run(["cp","-rf","bes.conf.template","bes.conf"])
if ret.returncode!=0:
    print("Cannot copy the bes.conf.template correctly, stop.")
    sys.exit(1)

# Need to add HDF5 and fileout netCDF BES library modules to the bes configuration file.
ret=subprocess.run(["python", "add_path_besconf.py"])
if ret.returncode!=0:
    print("Doesn't find the corresponding bes modules, stop.")
    sys.exit(1)

f_bescmd_file=f+".bescmd"
f_dmr_file=f+".dmr"
subprocess.run(["besstandalone", "-c", "bes.conf", "-i",f_bescmd_file, "-f",f_dmr_file])
f_dmrpp_file=f+".dmrpp"
f_dmrpp_id = open(f_dmrpp_file,'w')

if(args.u is not None):
    subprocess.run(["build_dmrpp", "-f", f, "-r",f_dmr_file,"-u",args.u],stdout=f_dmrpp_id)
else:
    subprocess.run(["build_dmrpp", "-f", f, "-r",f_dmr_file],stdout=f_dmrpp_id)
f_dmrpp_id.close()
subprocess.run(["rm", "-rf", "bes.conf"])   

