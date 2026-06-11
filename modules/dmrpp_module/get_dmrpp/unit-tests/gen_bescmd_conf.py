# This script that generates the bes configuration and xml files for testing the gen_dmrpp_side_car.
# The functions defined in this script are used by the gen_dmrpp_side_car_test.py.
import os
import shutil
import string
import subprocess
import sys

# The function add_module_path() adds the module path
def add_module_path(f_str, conf_mod_name, mod_path):
    index = f_str.find(conf_mod_name) + len(conf_mod_name)
    if (f_str[index] != '\n'):
        print(conf_mod_name)
        print(f_str[index])
        print("The module path should be empty.")
        sys.exit(1)
    temp_text = f_str[:index] + mod_path + f_str[index:]
    return temp_text

# Generate the BES configuration file
def generate_bes_conf():
    bes_conf_name="bes.test.conf"
    bes_conf_str=r'''BES.LogName=./bes.log
BES.modules=dap,cmd,fonc,dmrpp
BES.module.dap=
BES.module.cmd=
BES.module.dmrpp=
BES.module.fonc=
BES.Catalog.catalog.RootDirectory=
BES.Data.RootDirectory=/dev/null
BES.Catalog.catalog.TypeMatch+=dmrpp:.*\.(dmrpp)$;
BES.Catalog.catalog.Include=;
BES.Catalog.catalog.Exclude=^\..*;
BES.FollowSymLinks=Yes
BES.Catalog.catalog.FollowSymLinks=Yes
BES.UncompressCache.dir=/tmp
BES.UncompressCache.prefix=uncompress_cache
BES.UncompressCache.size=500
FONc.UseCompression=true
FONc.ChunkSize=4096
FONc.ClassicModel=false
FONc.NoGlobalAttrs=true
AllowedHosts+=^https?:\/\/'''
    cur_dir_bs = os.getcwd() +'/'
    bes_conf_str=add_module_path(bes_conf_str,"BES.module.dap=",cur_dir_bs+'libdap_module.so')
    bes_conf_str=add_module_path(bes_conf_str,"BES.module.cmd=",cur_dir_bs+'libdap_xml_module.so')
    bes_conf_str=add_module_path(bes_conf_str,"BES.module.dmrpp=",cur_dir_bs+'libdmrpp_module.so')
    bes_conf_str=add_module_path(bes_conf_str,"BES.module.fonc=",cur_dir_bs+'libfonc_module.so')
    bes_conf_str=add_module_path(bes_conf_str,"BES.Catalog.catalog.RootDirectory=",os.getcwd())
    bc_fid=open(bes_conf_name, 'w')
    bc_fid.write(bes_conf_str)
    bc_fid.close()

# Generate the xml file that tells the BES to return the netCDF-4 file.
def generate_bes_cmd(dmrpp_name):
    bescmd_str_p1 = '''<?xml version="1.0" encoding="UTF-8"?>
<request reqID="some_unique_value" >
    <setContext name="dap_format">dap4</setContext>
    <setContainer name="data" space="catalog">'''
    bescmd_str_p1 = bescmd_str_p1 + dmrpp_name + "</setContainer>"
    bescmd_str_p2 = '''
    <define name="d">
        <container name="data" />'''
    bescmd_str_p3 = '''
    </define>
    <get type="dap" definition="d" returnAs="netcdf-4"/>
</request>'''
    bescmd_str=bescmd_str_p1+bescmd_str_p2+bescmd_str_p3
    bescmd_name=dmrpp_name+"_test.bescmd"
    bc_fid=open(bescmd_name, 'w')
    bc_fid.write(bescmd_str)
    bc_fid.close()
    return bescmd_name


