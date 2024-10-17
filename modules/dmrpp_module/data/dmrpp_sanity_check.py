def compare_files(first_file, second_file, check_function):
    """
    Compare two files using the provided check_function (such as comparing attributes or variables).
    :param first_file: First file path
    :param second_file: Second file path
    :param check_function: Function that processes each file and returns comparable data
    :return: True if the files are equivalent, False otherwise
    """
    first_result = check_function(first_file)
    second_result = check_function(second_file)

    if first_result == second_result:
        if VERBOSE:
            print(f"Files {first_file} and {second_file} match.")
        return True
    else:
        print(f"ERROR: Files {first_file} and {second_file} do not match.")
        return False

def dap2_vars(file_path):
    # Extract DAP2 variables from the file (similar to the original Bash logic)
    return run_subprocess(["grep", "-e", "<Byte", "-e", "<Int", "-e", "<Float", file_path])

def dap4_vars(file_path):
    # Extract DAP4 variables from the file (similar to the original Bash logic)
    return run_subprocess(["grep", "-e", "<Int8", "-e", "<Float32", "-e", "<Array", file_path])

def dmrpp_inventory_test(bes_conf_file, source_dmr_file, output_filename):
    """
    Inventory test to compare DAP4 variables in the source and DMR++ files.
    :param bes_conf_file: BES configuration file
    :param source_dmr_file: Source DMR file
    :param output_filename: DMR++ output file
    """
    if VERBOSE:
        print(f"# Running inventory test on {source_dmr_file} and {output_filename}")

    # Compare DAP4 variables
    compare_files(source_dmr_file, output_filename, dap4_vars)

def dmrpp_value_test(bes_conf_file, source_dmr_file, output_filename):
    """
    Value test to compare the variable values in the source and DMR++ files.
    :param bes_conf_file: BES configuration file
    :param source_dmr_file: Source DMR file
    :param output_filename: DMR++ output file
    """
    if VERBOSE:
        print(f"# Running value test on {source_dmr_file} and {output_filename}")

    # Compare DAP2 variables
    compare_files(source_dmr_file, output_filename, dap2_vars)

def run_tests(bes_conf_file, source_dmr_file, output_filename):
    """
    Runs the inventory and value tests on the generated DMR++ file.
    :param bes_conf_file: BES configuration file
    :param source_dmr_file: Source DMR file
    :param output_filename: DMR++ output file
    """
    dmrpp_inventory_test(bes_conf_file, source_dmr_file, output_filename)
    dmrpp_value_test(bes_conf_file, source_dmr_file, output_filename)
