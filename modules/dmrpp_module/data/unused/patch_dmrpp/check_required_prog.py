import os
import shutil
import sys


#The fuction module_exist() checks if the module file exists.
def module_exist(module_path):
    return (os.path.isfile(module_path)==True)


def is_program_present(p_name):
    #Check whether `p_name` is on PATH and marked as executable.
    return shutil.which(p_name) is not None

# We use python 3.7.3.It must be >=python 3.7
if sys.version_info <=(3, 7):
    print("The tested version is python 3.7, your version is lower.")
    sys.exit(1)
#besstandalone and build_dmrpp must exist 
has_standalone_and_build_dmrpp =  \
    is_program_present('besstandalone') and is_program_present('build_dmrpp')
if(has_standalone_and_build_dmrpp==False):
    print("Either besstandalone or build_dmrpp is missing. ")
    print("They are required to continue.")
    sys.exit(1)

#We use besctl to find the module path
bes_cmd='besstandalone'
locate=shutil.which(bes_cmd)
#Remove the end characters, bin/besctl
locate_par_dir=locate[:len(locate)-17]
locate_lib_path=locate_par_dir
if(locate_par_dir=='/usr/'):
    if(os.path.isfile('/usr/lib64/bes/libdap_module.so')==True):
        locate_lib_path='/usr/lib64/bes/'
    else: 
        locate_lib_path='/usr/lib/bes/'
else:
    locate_lib_path=locate_par_dir+'lib/bes/'
         

libdap_path=locate_lib_path+'libdap_module.so'
libdap_xml_path=locate_lib_path+'libdap_xml_module.so'
libhdf5_path=locate_lib_path+'libhdf5_module.so'
libfonc_path=locate_lib_path+'libfonc_module.so'
libnc_path=locate_lib_path+'libnc_module.so'
#debugging
#libfakepath='fake.so'
has_all_modules=module_exist(libdap_path) and module_exist(libdap_xml_path) \
    and module_exist(libhdf5_path) and module_exist(libfonc_path) \
    and module_exist(libnc_path) 
    #and module_exist(libfakepath) 
if(has_all_modules==False):
    print("Some required modules don't exist,cannot generate the bes configuration file.")
    sys.exit(1)
print("All required programs are present.")

    
