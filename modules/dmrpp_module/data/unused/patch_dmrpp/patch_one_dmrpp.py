import argparse as ap
import os
import shutil
import subprocess
import sys


def is_program_present(p_name):
    #Check whether `p_name` is on PATH and marked as executable.
    return shutil.which(p_name) is not None

# We use python 3.7.3.It must be >=python 3.7
if sys.version_info <=(3, 7):
    print("The tested version is python 3.7, your version is lower.")
    sys.exit(1)

parser = ap.ArgumentParser(description='Patch a dmrpp file to ensure the values of all variables can be located.')
parser.add_argument('-i',  nargs=1,
                    help='The dmrpp file of the original HDF5 file is provided. ', required=True)
parser.add_argument("-v","--verbosity",type=int, choices=[0,1,2],
                    help='value=0, minimal output messages. value=1, more output messages. value=2, intermediate files are kept.')
parser.add_argument("-p",type=str, 
                    help='the path where the dmrpp file resides. For example, data/dmrpp')

args = parser.parse_args()
dmrpp_name = ''.join(args.i)

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
    print("Patching dmrpp step 2: Generate the BES configuration file used to generate the supplemental HDF5 file.")

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

if(args.verbosity is not None): 
    print("Patching dmrpp step 3: Check this dmrpp file and patch it if necessary.")
if dmrpp_name.endswith(".dmrpp"):

    if(args.verbosity is not None): 
        if(args.verbosity >0):
            print("  Check and patch the dmrpp file: ",dmrpp_name)
            print("  P3.1: Check if",dmrpp_name,"needs to be patched.")

    f_h5 = dmrpp_name.rsplit(".dmrpp")[0]

    # f1 is the file that stores the variables that dmrpp cannot provide values.
    f1= f_h5+".missvar"

    # check_dmrpp will check the dmrpp file and write the missing variable names
    # to f1.
    ret = subprocess.run([CHECK_DMRPP,dmrpp_name,f1])
    if ret.returncode!=0:
        print("Check_dmrpp doesn't run successfully for dmrpp file ",dmrpp_name)
        print("Stop. ")
        sys.exit(1)

    if(os.path.isfile(f1)==False):
        print("This dmrpp file has all the variable value information. ")
        print("No missing variable value information is found. ")
        sys.exit(0)

    if(args.verbosity is not None): 
        if(args.verbosity >0):
            print("  P3.2: Generate the supplemental HDF5 file that stores the missing variable values.")

    # We want to generate the bescmd file for the HDF5 file just including missing variables. 
    ret = subprocess.run(["python","gen_miss_vars_bescmd.py","-i",dmrpp_name])
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
            print("  P3.3: Generate the dmrpp file of the supplemental HDF5 file that stores the missing variable values.")

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
            print("  P3.4: Add the missing variable value information to the original DMRPP file.")

    # Merge the dmrpp that includes the missing variable values to the original dmrpp file.
    if(args.p is not None):
        ret = subprocess.run([MERGE_DMRPP, f_missingvar_h5_dmrpp, dmrpp_name,args.p,f1])   
    else:
        ret = subprocess.run([MERGE_DMRPP, f_missingvar_h5_dmrpp, dmrpp_name,os.getcwd(),f1])   
    if ret.returncode!=0:
        print("Cannot merge the dmrpp that just includes the missing variables to the orignal dmrpp file. ")
        print("The original dmrpp file name is:  ",dmrpp_name)
        print("The dmrpp file name that just includes the missing variables value info is:  ",f_missingvar_h5_dmrpp)
        print("Stop. ")
        sys.exit(1)

    print("  Sucessfully patch the dmrpp file: ",dmrpp_name,"\n")

    if(args.verbosity!=2): 
        subprocess.run(["rm", "-rf", f1])   
        subprocess.run(["rm", "-rf", f1_bescmd_file])   
        subprocess.run(["rm", "-rf", f_missingvar_h5_dmr_bescmd])   
        subprocess.run(["rm", "-rf", f_missingvar_h5_dmr])   
        subprocess.run(["rm", "-rf", f_missingvar_h5_dmrpp])   
        subprocess.run(["rm", "-rf", "bes.conf"])   
        subprocess.run(["rm", "-rf", "bes.log"])
        subprocess.run(["rm", "-rf", "mds_ledger.txt"])
else:
    if(args.verbosity!=2): 
        subprocess.run(["rm", "-rf", "bes.conf"])   
        subprocess.run(["rm", "-rf", "bes.log"])
        subprocess.run(["rm", "-rf", "mds_ledger.txt"])
    print("dmrpp file must end with .dmrpp")
    sys.exit(1)
