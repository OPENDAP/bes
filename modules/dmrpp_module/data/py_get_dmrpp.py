import argparse
import os
import subprocess
import tempfile
import shutil

# Globals (placeholders)
GET_DMRPP_VERSION = "get_dmrpp-@get_dmrpp_version@"
BES_DATA_ROOT = "/tmp"
VERBOSE = False
VERY_VERBOSE = False
JUST_DMR = False
DMRPP_URL = ""
BES_CONF_FILE = ""
SITE_CONF_FILE = ""
OUTPUT_FILE = ""
EXISTING_DMRPP = ""
S3_UPLOAD = False
USE_AUTOMAKE_LIBS = False
CLEANUP_TEMP_FILES = True
TEMP_FILE_LIST = []

def show_usage():
    print(f" [{GET_DMRPP_VERSION}]")
    print("""
    Usage: <script_name> [options] <INPUT_DATA_FILE>
    Write the DMR++ for INPUT_DATA_FILE (a hdf5/netcdf-4 file) to stdout
    * By default the BES Data Root directory is set to /tmp.
    ...
    """)

def cleanup_temp_files():
    if CLEANUP_TEMP_FILES:
        for temp_file in TEMP_FILE_LIST:
            if VERBOSE:
                print(f"# Removing temporary file: {temp_file}")
            try:
                os.remove(temp_file)
            except OSError as e:
                print(f"Error removing file {temp_file}: {e}")

def make_temp_file(file_prefix):
    if VERBOSE:
        print(f"# Creating temporary file with prefix: {file_prefix}")
    temp_file = tempfile.mktemp(prefix=f"{file_prefix}_")
    TEMP_FILE_LIST.append(temp_file)
    return temp_file

def mk_automake_conf():
    # This replaces mk_automake_conf function from the bash script
    if VERY_VERBOSE:
        print(f"Generating automake configuration")
    return """
    BES.module.dap=/path/to/dap/.libs/libdap_module.so
    BES.module.cmd=/path/to/xmlcommand/.libs/libdap_xml_module.so
    BES.module.h5=/path/to/hdf5_handler/.libs/libhdf5_module.so
    BES.module.dmrpp=/path/to/dmrpp_module/.libs/libdmrpp_module.so
    BES.module.nc=/path/to/netcdf_handler/.libs/libnc_module.so
    BES.module.fonc=/path/to/fileout_netcdf/.libs/libfonc_module.so
    """

def mk_default_bes_conf():
    default_bes_conf_doc = f"""
    BES.LogName=./bes.log
    BES.modules=dap,cmd,h5,dmrpp,nc,fonc
    BES.module.dap=/path/to/dap/libdap_module.so
    BES.module.cmd=/path/to/xmlcommand/libdap_xml_module.so
    BES.module.h5=/path/to/hdf5_handler/libhdf5_module.so
    BES.module.dmrpp=/path/to/dmrpp_module.so
    BES.module.nc=/path/to/netcdf_handler/libnc_module.so
    BES.module.fonc=/path/to/fileout_netcdf/libfonc_module.so
    BES.Catalog.catalog.RootDirectory={BES_DATA_ROOT}
    """

    if USE_AUTOMAKE_LIBS:
        automake_libs = mk_automake_conf()
        default_bes_conf_doc += automake_libs

    return default_bes_conf_doc

def run_subprocess(command, capture_output=False):
    try:
        result = subprocess.run(command, check=True, capture_output=capture_output, text=True)
        if capture_output:
            return result.stdout.strip()
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {command}, {e}")
        return None

def mk_dmrpp(bes_conf_file, data_file, dmr_file, output_filename):
    if VERY_VERBOSE:
        print(f"Building DMR++ from {data_file} using {dmr_file}")
    command = ["build_dmrpp", "-c", bes_conf_file, "-f", os.path.join(BES_DATA_ROOT, data_file),
               "-r", dmr_file, "-u", DMRPP_URL]
    if output_filename:
        with open(output_filename, 'w') as f:
            subprocess.run(command, stdout=f)
    else:
        subprocess.run(command)

def mk_bes_conf(user_supplied_conf_file, site_conf_file):
    bes_conf_doc = mk_default_bes_conf()

    if user_supplied_conf_file:
        with open(user_supplied_conf_file, 'r') as f:
            bes_conf_doc += f.read()

    if site_conf_file:
        with open(site_conf_file, 'r') as f:
            bes_conf_doc += f.read()

    tmp_conf_file = make_temp_file("bes_conf")
    with open(tmp_conf_file, 'w') as f:
        f.write(bes_conf_doc)

    return tmp_conf_file

def main():
    global VERBOSE, VERY_VERBOSE, JUST_DMR, DMRPP_URL, BES_CONF_FILE, SITE_CONF_FILE, OUTPUT_FILE, S3_UPLOAD, CLEANUP_TEMP_FILES
    parser = argparse.ArgumentParser()
    parser.add_argument('-v', '--verbose', action='store_true', help="Verbose output")
    parser.add_argument('-V', '--very_verbose', action='store_true', help="Very verbose output")
    parser.add_argument('-D', '--just_dmr', action='store_true', help="Just print the DMR")
    parser.add_argument('-u', '--dmrpp_url', help="DMR++ URL")
    parser.add_argument('-o', '--output_file', help="Output DMR++ file")
    parser.add_argument('-c', '--bes_conf_file', help="BES configuration file")
    parser.add_argument('-s', '--site_conf_file', help="Site configuration file")
    parser.add_argument('-b', '--bes_data_root', help="BES Data Root directory")
    parser.add_argument('-U', '--s3_upload', action='store_true', help="Upload output to S3")
    parser.add_argument('input_file', help="The input data file")

    args = parser.parse_args()

    VERBOSE = args.verbose
    VERY_VERBOSE = args.very_verbose
    JUST_DMR = args.just_dmr
    DMRPP_URL = args.dmrpp_url or "OPeNDAP_DMRpp_DATA_ACCESS_URL"
    BES_CONF_FILE = args.bes_conf_file
    SITE_CONF_FILE = args.site_conf_file
    OUTPUT_FILE = args.output_file
    S3_UPLOAD = args.s3_upload

    INPUT_DATA_FILE = args.input_file

    # Make BES conf file
    bes_conf_file = mk_bes_conf(BES_CONF_FILE, SITE_CONF_FILE)

    # Generate DMR file
    dmr_file = make_temp_file("dmr")
    run_subprocess(["mkDapRequest", bes_conf_file, INPUT_DATA_FILE, "dmr"], capture_output=True)

    if JUST_DMR:
        if OUTPUT_FILE:
            shutil.copy(dmr_file, OUTPUT_FILE)
        else:
            with open(dmr_file, 'r') as f:
                print(f.read())
        return

    # Build DMR++
    mk_dmrpp(bes_conf_file, INPUT_DATA_FILE, dmr_file, OUTPUT_FILE)

    if S3_UPLOAD:
        if not OUTPUT_FILE:
            print("Output file is required to upload to S3. Use the -o option.")
            return

        run_subprocess(["aws", "s3", "cp", OUTPUT_FILE, f"{INPUT_DATA_FILE}.dmrpp"])

    cleanup_temp_files()

if __name__ == "__main__":
    main()
