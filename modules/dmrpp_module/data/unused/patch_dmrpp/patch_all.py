import argparse as ap
import os
import shutil
import subprocess
import sys


def is_program_present(p_name):
    #Check whether `p_name` is on PATH and marked as executable.
    return shutil.which(p_name) is not None


parser = ap.ArgumentParser(description='Patch dmrpp files to ensure the values of all variables can be located.')
#parser.add_argument('-i',  nargs=1,
#                    help='The dmrpp file of the original HDF5 file is provided. ')
parser.add_argument("-v","--verbosity",type=int, choices=[0,1,2],
                    help='value=0, minimal output messages. value=1, more output messages. value=2, intermediate files are kept.')
parser.add_argument("-p",type=str, 
                    help='the path where the dmrpp file resides. For example, data/dmrpp')

args = parser.parse_args()


# We use python 3.7.3.It must be >=python 3.7
if sys.version_info <=(3, 7):
    print("The tested version is python 3.7, your version is lower.")
    sys.exit(1)

if(args.verbosity is not None): 
    print("Check and provide the missing variable values location for dmrpp files.\n")
    print("Patching dmrpp step 1. Check if the necessary exectuables exist:")
    print("  besstandalone and build_dmrpp should be found in the path.")
    print("  check_dmrpp and merge_dmrpp should be either found in the path or generated from C++ source files.\n")


# Sanity check if the necessary programs exist
# besstandalone and build_dmrpp must exist 
has_standalone_and_build_dmrpp =  \
    is_program_present('besstandalone') and is_program_present('build_dmrpp')

if(has_standalone_and_build_dmrpp==False):
    print("Either besstandalone or build_dmrpp is missing. ")
    print("They are required to continue, stop!")
    sys.exit(1)

# If check_dmrpp and merge_dmrpp don't exist, check if they can be compiled
CHECK_DMRPP=""
if(is_program_present('check_dmrpp')==True):
    CHECK_DMRPP='check_dmrpp'
else:
    ret = subprocess.run(["g++","-o","check_dmrpp","check_dmrpp_out.cc"])
    if ret.returncode!=0:
        print("You may not have the check_dmrpp_out.cc or have a compiling error.")
        print("Stop.")
        sys.exit(1)
    else: 
        CHECK_DMRPP='./check_dmrpp'

MERGE_DMRPP=""
if(is_program_present('merge_dmrpp')==True):
    MERGE_DMRPP='merge_dmrpp'
else:
    ret = subprocess.run(["g++","-o","merge_dmrpp","merge_dmrpp.cc"])
    if ret.returncode!=0:
        print("You may not have the merge_dmrpp.cc or have a compiling error.")
        print("Stop.")
        sys.exit(1)
    else: 
        MERGE_DMRPP='./merge_dmrpp'

if(args.verbosity is not None): 
    print("Patching dmrpp step 2: Check if the number of HDF5 files and the corresponding DMRPP files are the same.")

# Sanity check: if we have the same number of HDF5 files and dmrpp files.
# This program needs 1-1 mapping from the HDF5 file to the dmrpp file.
files=[f for f in os.listdir('.') if os.path.isfile(f)]
# Sanity check.
for f in files:
    if f.endswith(".h5") or f.endswith(".he5") or f.endswith(".hdf5") or f.endswith(".HDF5") or f.endswith(".nc") or f.endswith(".nc4"):
        f1= f+".dmrpp"
        if(f1 not in files):
            print("No dmrpp files for all HDF5 files, stop. ")
            sys.exit(1)

for f in files:
    if f.endswith(".dmrpp"):
        f1= f.rsplit(".dmrpp")[0]
        has_hdf5 = f1.endswith(".h5")|f1.endswith(".he5")|f1.endswith(".hdf5")|f1.endswith(".HDF5")|f1.endswith(".nc")|f1.endswith(".nc4")
        if(False == has_hdf5):
            print("No HDF5 files for all dmrpp files, stop. ")
            sys.exit(1)

if(args.verbosity is not None): 
    print("Patching dmrpp step 3: Generate the BES configuration file used to generate the supplemental HDF5 file.")

# Generate the bes.conf file.
# Copy the template of the bes configuration file to the one we want to use
# To avoid the interactive copy. Here I obtain the system cp.
OSCP=shutil.which("cp")
#ret=subprocess.run(["cp","-rf","bes.conf.template","bes.conf"])
ret=subprocess.run([OSCP,"-rf","bes.conf.template","bes.conf"])
if ret.returncode!=0:
    print("Cannot copy the bes.conf.template correctly, stop.")
    sys.exit(1)

# Need to add HDF5 and fileout netCDF BES library modules to the bes configuration file.
ret=subprocess.run(["python", "add_path_besconf.py"])
if ret.returncode!=0:
    print("Doesn't find the corresponding bes modules, stop.")
    sys.exit(1)

# Now go through all dmrpp files and generate the missing var hdf5 files.      
if(args.verbosity is not None): 
    print("Patching dmrpp step 4: Check all dmrpp files and patch them if necessary under this directory.")
for f in files:
    if f.endswith(".dmrpp"):

        if(args.verbosity is not None): 
            if(args.verbosity >0):
                print("  Check and patch the dmrpp file: ",f)
                print("  P4.1: Check if",f,"needs to be patched.")

        f_h5 = f.rsplit(".dmrpp")[0]

        # f1 is the file that stores the variables that dmrpp cannot provide values.
        f1= f_h5+".missvar"

        # check_dmrpp will check the dmrpp file and write the missing variable names
        # to f1.
        ret = subprocess.run([CHECK_DMRPP,f,f1])
        if ret.returncode!=0:
            print("Check_dmrpp doesn't run successfully for dmrpp file ",f)
            print("Stop. ")
            sys.exit(1)

        # Most dmrpp files will not have any missing variables. So just go to the next dmrpp file.
        if(os.path.isfile(f1)==False):
            if(args.verbosity is not None): 
                if(args.verbosity >0):
                    print(" ", f,"doesn't miss any variable values; no need to patch. \n")
            continue

        if(args.verbosity is not None): 
            if(args.verbosity >0):
                print("  P4.2: Generate the supplemental HDF5 file that stores the missing variable values.")

        # We want to generate the bescmd file for the HDF5 file just including missing variables. 
        ret = subprocess.run(["python","gen_miss_vars_bescmd.py","-i",f])
        if ret.returncode!=0:
            print("Cannot generate the bescmd xml for the HDF5 file that just includes the missing variables. ")
            print("The original HDF5 file name is:  ",f_h5)
            print("Stop. ")
            sys.exit(1)
        
        # Prepare the bescmd and the HDF5 file names for the missing variables
        f1_bescmd_file=f_h5+"_missing.bescmd"
        dot_index = f_h5.rfind('.')
        f_missingvar_h5 = f_h5[:dot_index]+"_missing.h5"

        # Generate the HDF5 file that just includes the missing variables. 
        # Here we use the filenetCDF-4 module. 
        ret = subprocess.run(["besstandalone", "-c", "bes.conf", "-i",f1_bescmd_file, "-f",f_missingvar_h5])
        if ret.returncode!=0:
            print("Cannot generate the HDF5 file that just includes the missing variables. ")
            print("The original HDF5 file name is:  ",f_h5)
            print("Stop. ")
            sys.exit(1)

        if(args.verbosity is not None): 
            if(args.verbosity>0):
                print("  P4.3: Generate the dmrpp file of the supplemental HDF5 file that stores the missing variable values.")


        # Generate the dmr and dmrpp for the HDF5 file that just includes the missing variables. 
        f_missingvar_h5_dmr_bescmd = f_missingvar_h5+".dmr.bescmd"
        f_missingvar_h5_dmr = f_missingvar_h5+".dmr"
        f_missingvar_h5_dmrpp=f_missingvar_h5+".dmrpp"
        ret = subprocess.run(["besstandalone", "-c", "bes.conf", "-i",f_missingvar_h5_dmr_bescmd, "-f",f_missingvar_h5_dmr])   
        if ret.returncode!=0:
            print("Cannot generate the HDF5 file that just includes the missing variable value info. ")
            print("The original HDF5 file name is:  ",f_h5)
            print("Stop. ")
            sys.exit(1)

        f_missingvar_h5_dmrpp_id = open(f_missingvar_h5_dmrpp,'w')
        ret = subprocess.run(["build_dmrpp", "-f", f_missingvar_h5, "-r",f_missingvar_h5_dmr],stdout=f_missingvar_h5_dmrpp_id)   
        if ret.returncode!=0:
            print("Cannot build the dmrpp that just includes the missing variable value info. ")
            print("The HDF5 file name that just includes the missing variable value info. is:  ",f_missingvar_h5)
            print("Stop. ")
            sys.exit(1)
        f_missingvar_h5_dmrpp_id.close()

        if(args.verbosity is not None): 
            if(args.verbosity>0):
                print("  P4.4: Add the missing variable value information to the original DMRPP file.")


        # Merge the dmrpp that includes the missing variable values to the original dmrpp file.
        if(args.p is not None):
            ret = subprocess.run([MERGE_DMRPP, f_missingvar_h5_dmrpp, f,args.p,f1])   
        else:
            ret = subprocess.run([MERGE_DMRPP, f_missingvar_h5_dmrpp, f,os.getcwd(),f1])   
        if ret.returncode!=0:
            print("Cannot merge the dmrpp that just includes the missing variables to the orignal dmrpp file. ")
            print("The original dmrpp file name is:  ",f)
            print("The dmrpp file name that just includes the missing variables value info is:  ",f_missingvar_h5_dmrpp)
            print("Stop. ")
            sys.exit(1)

        if(args.verbosity is not None): 
            if(args.verbosity>0):
                print("  Sucessfully patch the dmrpp file: ",f,"\n")

        if(args.verbosity!=2):
            subprocess.run(["rm", "-rf", f1])
            subprocess.run(["rm", "-rf", f1_bescmd_file])
            subprocess.run(["rm", "-rf", f_missingvar_h5_dmr_bescmd])
            subprocess.run(["rm", "-rf", f_missingvar_h5_dmr])
            subprocess.run(["rm", "-rf", f_missingvar_h5_dmrpp])

if(args.verbosity!=2):
    subprocess.run(["rm", "-rf", "bes.conf"])
    subprocess.run(["rm", "-rf", "bes.log"])
    subprocess.run(["rm", "-rf", "mds_ledger.txt"])

print("Successfully patch all dmrpp files under this directory.\n")
