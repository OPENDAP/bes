import os
import shutil
import sys

#The fuction module_exist() checks if the module file exists.
def module_exist(module_path):
    return (os.path.isfile(module_path)==True)

#The function add_module_path() adds the module path
def add_module_path(f_str,conf_mod_name,mod_path):
    index=f_str.find(conf_mod_name)+len(conf_mod_name)
    if(f_str[index]!='\n'):
        print(conf_mod_name)
        print(f_str[index])
        print("The module path should be empty by default,should check bes.conf")
        sys.exit(1)
    temp_text=f_str[:index]+mod_path+f_str[index:]
    return temp_text

#We use besctl to find the module path
bes_cmd='besstandalone'
locate=shutil.which(bes_cmd)
#print(locate)

#Remove the end characters, bin/besstandalone
locate_par_dir=locate[:len(locate)-17]
#print(locate_par_dir)

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

#print("continue")
bc_fname="bes.conf"
bc_fid = open(bc_fname,'r')
bc_text = bc_fid.read()
#print(bc_text)
bc_fid.close()
#bes_libdap="BES.module.dap="
#index=bc_text.find(bes_libdap)+len(bes_libdap)
#temp_text=bc_text[:index]+libdap_path+bc_text[index:]
#print(temp_text)

temp_text = add_module_path(bc_text,"BES.module.dap=",libdap_path)
temp_text = add_module_path(temp_text,"BES.module.cmd=",libdap_xml_path)
temp_text = add_module_path(temp_text,"BES.module.h5=",libhdf5_path)
temp_text = add_module_path(temp_text,"BES.module.fonc=",libfonc_path)
temp_text = add_module_path(temp_text,"BES.module.nc=",libnc_path)
cur_dir=os.getcwd()
temp_text =add_module_path(temp_text,"BES.Catalog.catalog.RootDirectory=",cur_dir)
bc_fid = open(bc_fname,'w')
bc_fid.write(temp_text)
bc_fid.close()

#print("Successfully generate the bes configuration file.")

    
