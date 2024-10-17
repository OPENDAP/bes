def mk_dap2_bes_cmd(source_data_path, dap_type, dap2_ce="", return_as=""):
    """
    Builds the BES DAP2 command XML.
    :param source_data_path: Path to the source data file
    :param dap_type: Type of DAP (dmr, ddx, etc.)
    :param dap2_ce: Optional DAP2 Constraint Expression
    :param return_as: Optional return type
    :return: BES command as XML string
    """
    if VERBOSE:
        print(f"# Generating DAP2 BES command for {source_data_path} with type {dap_type} and CE {dap2_ce}")

    constraint_element = f"<bes:constraint>{dap2_ce}</bes:constraint>" if dap2_ce else "<bes:container name=\"c\" />"

    bes_get_element = f'<bes:get type="{dap_type}" definition="d" returnAs="{return_as}" />' if return_as else f'<bes:get type="{dap_type}" definition="d" />'

    bes_command = f"""
    <?xml version="1.0" encoding="UTF-8"?>
    <bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="get_dmrpp.py">
    <bes:setContext name="dap_explicit_containers">no</bes:setContext>
    <bes:setContext name="errors">xml</bes:setContext>
    <bes:setContext name="max_response_size">0</bes:setContext>
    <bes:setContainer name="c">{source_data_path}</bes:setContainer>
    <bes:define name="d" space="default">
    {constraint_element}
    </bes:define>
    {bes_get_element}
    </bes:request>
    """
    return bes_command

def mk_dap4_bes_cmd(source_data_path, dap4_type, dap4_ce="", return_as=""):
    """
    Builds the BES DAP4 command XML.
    :param source_data_path: Path to the source data file
    :param dap4_type: Type of DAP (dmr, dap, etc.)
    :param dap4_ce: Optional DAP4 Constraint Expression
    :param return_as: Optional return type
    :return: BES command as XML string
    """
    if VERBOSE:
        print(f"# Generating DAP4 BES command for {source_data_path} with type {dap4_type} and CE {dap4_ce}")

    constraint_element = f"<bes:dap4constraint>{dap4_ce}</bes:dap4constraint>" if dap4_ce else "<bes:container name=\"c\" />"

    bes_get_element = f'<bes:get type="{dap4_type}" definition="d" returnAs="{return_as}" />' if return_as else f'<bes:get type="{dap4_type}" definition="d" />'

    bes_command = f"""
    <?xml version="1.0" encoding="UTF-8"?>
    <bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="get_dmrpp.py">
    <bes:setContext name="dap_explicit_containers">no</bes:setContext>
    <bes:setContext name="errors">xml</bes:setContext>
    <bes:setContext name="max_response_size">0</bes:setContext>
    <bes:setContainer name="c">{source_data_path}</bes:setContainer>
    <bes:define name="d" space="default">
    {constraint_element}
    </bes:define>
    {bes_get_element}
    </bes:request>
    """
    return bes_command

def run_bes_command(command_xml, output_file=None):
    """
    Runs the BES command and optionally saves the output to a file.
    :param command_xml: The BES command as XML
    :param output_file: Optional output file to save the result
    """
    # Create a temporary file for the BES command
    bes_command_file = make_temp_file("bes_cmd")
    with open(bes_command_file, 'w') as f:
        f.write(command_xml)

    command = ["besstandalone", "-i", bes_command_file]
    if output_file:
        with open(output_file, 'w') as f:
            subprocess.run(command, stdout=f)
    else:
        result = subprocess.run(command, capture_output=True, text=True)
        return result.stdout

def generate_dap2_bes(source_data_path, dap_type, output_file, dap2_ce=""):
    bes_command = mk_dap2_bes_cmd(source_data_path, dap_type, dap2_ce)
    return run_bes_command(bes_command, output_file)

def generate_dap4_bes(source_data_path, dap4_type, output_file, dap4_ce=""):
    bes_command = mk_dap4_bes_cmd(source_data_path, dap4_type, dap4_ce)
    return run_bes_command(bes_command, output_file)
