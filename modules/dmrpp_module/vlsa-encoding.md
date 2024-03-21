
###############################################
#
# VLSA dmr++ encoding efficiency
#
# Two methods:
#  - XML markup for each value
#  = base64 encoded and packed  
#
# The base64 encoding has a constant linear cost.
# The encoded values are 134% of the raw string.
#
# The XML markup version adds a constant size 
# increase of 15 characters per value (assuming 
# an average indent of 8 characters in the pretty 
# version we currently produce.
#
# Worst case for the XML markup is 150% increase
# for a single character string. 
#
# For strings 12 characters or longer the XML 
# more efficient base64 representation.
# 
# And you can read the XML with eyeballs.
# - - - - - - - - - - - - - - - - - - - - - - - -

# 
# Constant one time cost both versions:

<dmrpp:vlsa></dmrpp:vlsa> = 25 chars + indent(8) = 33 chars.

###############################################
# Using XML Markup for each value.
#
# The XML markup is a constant cost for each
# string value.

<v></v> = 7 additional characters +  8 char indent for pretty format = 15 constant cost, regardless of string size.

Worst case:  Every string is length 1
16 - 1 = 15 additonal chars = 150 %increase.

#  15 /   len = %increase 

  15 /     1 = 150.0% worst
  15 /    11 = 136.3% crossover with base64
  15 /    12 = 125.0% 
  15 /    15 = 100.0% doubled in length
  15 /   150 =  10.0% 
  15 /  1500 =  1.0% 
  15 / 15000 =  0.1% 
  15 / 15873 =  0.0945% # dmr++ as input


###############################################
# Base64 encoding and packed into a single
# XML element. 
# This is a linear cost

# encoded - raw = diff                        diff/len
    285 -   212 =   73 additonal characters = 134.4% increase
    581 -   433 =  148 additonal characters = 134.1% increase.
   1561 -  1170 =  391 additonal characters = 133.4% increase
  21165 - 15873 = 5292 additonal characters = 133.3% increase  # dmr++ as input



# -----------------------------------
# 212 chars

/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld28
L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjg=;

285 - 212 = 73 additonal characters = 134.4% increase


# -----------------------------------
# 433 chars

/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_
L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uXw==;

581-433 = 148 additonal characters = 134.1% increase.



# -----------------------------------
# 1170 chars

/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5
L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1;

1561-1170 = 391 additonal characters = 133.4% increase



# -----------------------------------
# 15873 chars

<?xml version="1.0" encoding="ISO-8859-1"?>
<Dataset xmlns="http://xml.opendap.org/ns/DAP/4.0#" xmlns:dmrpp="http://xml.opendap.org/dap/dmrpp/1.0.0#" dapVersion="4.0" dmrVersion="1.0" name="acos_L2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.h5" dmrpp:href="OPeNDAP_DMRpp_DATA_ACCESS_URL" dmrpp:version="3.20.13">
    <String name="AerosolTypes">
        <Dim size="5"/>
        <Attribute name="Shape" type="String">
            <Value>Aerosol_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Total</v>
            <v>Ice</v>
            <v>Kahn_2b</v>
            <v>Kahn_3b</v>
            <v>Water</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>VG90YWw=;SWNl;S2Fobl8yYg==;S2Fobl8zYg==;V2F0ZXI=;</dmrpp:vlsa>
    </String>
    <String name="AncillaryDataDescriptors">
        <Dim size="3"/>
        <Attribute name="Shape" type="String">
            <Value>AncFile_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>acos_ACOSL2fSubFielder_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.config</v>
            <v>acos_L2sSubField_sfif_101209184416.xml</v>
            <v>acos_L2sSubField_apf_101019205935.xml</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>YWNvc19BQ09TTDJmU3ViRmllbGRlcl8xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAxX1BvbEJfMTEwNDMwMTkyNzM5LmNvbmZpZw==;YWNvc19MMnNTdWJGaWVsZF9zZmlmXzEwMTIwOTE4NDQxNi54bWw=;YWNvc19MMnNTdWJGaWVsZF9hcGZfMTAxMDE5MjA1OTM1LnhtbA==;</dmrpp:vlsa>
    </String>
    <String name="AutomaticQualityFlag">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v></v>
        </dmrpp:vlsa>
        <dmrpp:vlsa></dmrpp:vlsa>
    </String>
    <String name="BuildId">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>B2.08.00</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QjIuMDguMDA=</dmrpp:vlsa>
    </String>
    <String name="CollectionLabel">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Production_v110110_L2s2800_r01</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>UHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAx</dmrpp:vlsa>
    </String>
    <String name="GranulePointer">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>acos_L2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.h5</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>YWNvc19MMnNfMTEwNDE5XzQzX1Byb2R1Y3Rpb25fdjExMDExMF9MMnMyODAwX3IwMV9Qb2xCXzExMDQzMDE5MjczOS5oNQ==</dmrpp:vlsa>
    </String>
    <String name="HDFVersionId">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>HDF5 1.8.5</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>SERGNSAxLjguNQ==</dmrpp:vlsa>
    </String>
    <String name="InputPointer">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>InputPtr_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>acos_L2f_110419_43_Production_v110110_L2f2800_r01_PolB_110430192051.h5</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>YWNvc19MMmZfMTEwNDE5XzQzX1Byb2R1Y3Rpb25fdjExMDExMF9MMmYyODAwX3IwMV9Qb2xCXzExMDQzMDE5MjA1MS5oNQ==</dmrpp:vlsa>
    </String>
    <String name="InstrumentShortName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>TANSO-FTS</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>VEFOU08tRlRT</dmrpp:vlsa>
    </String>
    <String name="L2FullPhysicsAlgorithmDescriptor">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>3-band full physics retrieval</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>My1iYW5kIGZ1bGwgcGh5c2ljcyByZXRyaWV2YWw=</dmrpp:vlsa>
    </String>
    <String name="L2FullPhysicsDataVersion">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>B2.08.00-10928</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QjIuMDguMDAtMTA5Mjg=</dmrpp:vlsa>
    </String>
    <String name="L2FullPhysicsExeVersion">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>B2.08.00-10928</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QjIuMDguMDAtMTA5Mjg=</dmrpp:vlsa>
    </String>
    <String name="L2FullPhysicsInputPointer">
        <Dim size="10"/>
        <Attribute name="Shape" type="String">
            <Value>L2FullPhysicsInputPtr_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>/acos/product/Production/v110110/Cld2800/r01/110419/acos_Cld_110419_43_Production_v110110_Cld2800_r01_110429144825.h5</v>
            <v>/acos/product/Production/v110110/Ecm2800/r01/110419/acos_Ecm_110419_43_Production_v110110_Ecm2800_r01_110429144643.h5</v>
            <v>/acos/product/Production/v110110/L1b2800/r01/110419/acos_L1b_110419_43_Production_v110110_L1b2800_r01_110429142001.h5</v>
            <v>/acos/product/Production/v110110/Snd2800/r01/110419/acos_Snd_110419_43_Production_v110110_Snd2800_r01_110429151347.txt</v>
            <v>/data1/deploy/L2/B2.08.00/Level2_B2.08.00/input/gosat/config/coxmunk</v>
            <v>/data1/deploy/L2/B2.08.00/Level2_B2.08.00/input/gosat/config/lambertian</v>
            <v>/data1/deploy/L2/B2.08.00/Level2_B2.08.00/input/gosat/input/static</v>
            <v>/groups/algorithm/l2_fp/absco/v3.2.0/co2_4700.0-6500.0_v3.2.0.chunk.hdf</v>
            <v>/groups/algorithm/l2_fp/absco/v3.2.0/h2o_4700.0-6500.0_v3.2.0.chunk.hdf</v>
            <v>/groups/algorithm/l2_fp/absco/v3.2.0/o2_12745.0-13245.0_v3.2.0.chunk.hdf</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvQ2xkMjgwMC9yMDEvMTEwNDE5L2Fjb3NfQ2xkXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfQ2xkMjgwMF9yMDFfMTEwNDI5MTQ0ODI1Lmg1;L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvRWNtMjgwMC9yMDEvMTEwNDE5L2Fjb3NfRWNtXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfRWNtMjgwMF9yMDFfMTEwNDI5MTQ0NjQzLmg1;L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvTDFiMjgwMC9yMDEvMTEwNDE5L2Fjb3NfTDFiXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfTDFiMjgwMF9yMDFfMTEwNDI5MTQyMDAxLmg1;L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvU25kMjgwMC9yMDEvMTEwNDE5L2Fjb3NfU25kXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfU25kMjgwMF9yMDFfMTEwNDI5MTUxMzQ3LnR4dA==;L2RhdGExL2RlcGxveS9MMi9CMi4wOC4wMC9MZXZlbDJfQjIuMDguMDAvaW5wdXQvZ29zYXQvY29uZmlnL2NveG11bms=;L2RhdGExL2RlcGxveS9MMi9CMi4wOC4wMC9MZXZlbDJfQjIuMDguMDAvaW5wdXQvZ29zYXQvY29uZmlnL2xhbWJlcnRpYW4=;L2RhdGExL2RlcGxveS9MMi9CMi4wOC4wMC9MZXZlbDJfQjIuMDguMDAvaW5wdXQvZ29zYXQvaW5wdXQvc3RhdGlj;L2dyb3Vwcy9hbGdvcml0aG0vbDJfZnAvYWJzY28vdjMuMi4wL2NvMl80NzAwLjAtNjUwMC4wX3YzLjIuMC5jaHVuay5oZGY=;L2dyb3Vwcy9hbGdvcml0aG0vbDJfZnAvYWJzY28vdjMuMi4wL2gyb180NzAwLjAtNjUwMC4wX3YzLjIuMC5jaHVuay5oZGY=;L2dyb3Vwcy9hbGdvcml0aG0vbDJfZnAvYWJzY28vdjMuMi4wL28yXzEyNzQ1LjAtMTMyNDUuMF92My4yLjAuY2h1bmsuaGRm;</dmrpp:vlsa>
    </String>
    <String name="L2FullPhysicsOperationsVersion">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>B2.08.00-10928</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QjIuMDguMDAtMTA5Mjg=</dmrpp:vlsa>
    </String>
    <String name="LongName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>ACOS GOSAT/TANSO-FTS Level 2 Full Physics Standard Product</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QUNPUyBHT1NBVC9UQU5TTy1GVFMgTGV2ZWwgMiBGdWxsIFBoeXNpY3MgU3RhbmRhcmQgUHJvZHVjdA==</dmrpp:vlsa>
    </String>
    <Int32 name="MissingExposures">
        <Dim size="3"/>
        <Dim size="2"/>
        <Attribute name="Shape" type="String">
            <Value>Band_Polarization_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>Signed32</Value>
        </Attribute>
        <dmrpp:chunks compressionType="deflate" deflateLevel="2" fillValue="0" byteOrder="LE">
            <dmrpp:chunkDimensionSizes>3 2</dmrpp:chunkDimensionSizes>
            <dmrpp:chunk offset="440496" nBytes="11" chunkPositionInArray="[0,0]"/>
        </dmrpp:chunks>
    </Int32>
    <String name="NominalDay">
    <String name="PlatformLongName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Greenhouse gases Observing SATellite</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>R3JlZW5ob3VzZSBnYXNlcyBPYnNlcnZpbmcgU0FUZWxsaXRl</dmrpp:vlsa>
    </String>
    <String name="PlatformShortName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>GOSAT</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>R09TQVQ=</dmrpp:vlsa>
    </String>
    <String name="PlatformType">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>spacecraft</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>c3BhY2VjcmFmdA==</dmrpp:vlsa>
    </String>
    <String name="ProcessingLevel">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Level 2</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>TGV2ZWwgMg==</dmrpp:vlsa>
    </String>
    <String name="ProducerAgency">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>NASA</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>TkFTQQ==</dmrpp:vlsa>
    </String>
    <String name="ProducerInstitution">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>JPL</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>SlBM</dmrpp:vlsa>
    </String>
    
    
    <String name="ProductionLocation">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Operations Pipeline</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>T3BlcmF0aW9ucyBQaXBlbGluZQ==</dmrpp:vlsa>
    </String>
    <String name="ProjectId">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>ACOS</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QUNPUw==</dmrpp:vlsa>
    </String>
    <String name="QAGranulePointer">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>acos_qL2s_110419_43_Production_v110110_L2s2800_r01_PolB_110430192739.txt</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>YWNvc19xTDJzXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfTDJzMjgwMF9yMDFfUG9sQl8xMTA0MzAxOTI3MzkudHh0</dmrpp:vlsa>
    </String>
    <String name="SISName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>acos_volumes.xlsx</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>YWNvc192b2x1bWVzLnhsc3g=</dmrpp:vlsa>
    </String>
    <String name="SISVersion">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>Initial</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>SW5pdGlhbA==</dmrpp:vlsa>
    </String>
    <String name="ShortName">
        <Dim size="1"/>
        <Attribute name="Shape" type="String">
            <Value>Scalar</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>ACOS_L2S</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>QUNPU19MMlM=</dmrpp:vlsa>
    </String>
    <String name="SpectralChannel">
        <Dim size="3"/>
        <Attribute name="Shape" type="String">
            <Value>Band_Array</Value>
        </Attribute>
        <Attribute name="Type" type="String">
            <Value>VarLenStr</Value>
        </Attribute>
        <dmrpp:vlsa>
            <v>0.76um O2 A-band</v>
            <v>1.6um Weak CO2</v>
            <v>2.06um Strong CO2</v>
        </dmrpp:vlsa>
        <dmrpp:vlsa>MC43NnVtIE8yIEEtYmFuZA==;MS42dW0gV2VhayBDTzI=;Mi4wNnVtIFN0cm9uZyBDTzI=;</dmrpp:vlsa>
    </String>
</Dataset>



PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iSVNPLTg4NTktMSI/Pgo8RGF0YXNldCB4bWxucz0iaHR0cDovL3htbC5vcGVuZGFwLm9yZy9ucy9EQVAvNC4wIyIgeG1sbnM6ZG1ycHA9Imh0dHA6Ly94bWwub3BlbmRhcC5vcmcvZGFwL2RtcnBwLzEuMC4wIyIgZGFwVmVyc2lvbj0iNC4wIiBkbXJWZXJzaW9uPSIxLjAiIG5hbWU9ImFjb3NfTDJzXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfTDJzMjgwMF9yMDFfUG9sQl8xMTA0MzAxOTI3MzkuaDUiIGRtcnBwOmhyZWY9Ik9QZU5EQVBfRE1ScHBfREFUQV9BQ0NFU1NfVVJMIiBkbXJwcDp2ZXJzaW9uPSIzLjIwLjEzIj4KICAgIDxTdHJpbmcgbmFtZT0iQWVyb3NvbFR5cGVzIj4KICAgICAgICA8RGltIHNpemU9IjUiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+QWVyb3NvbF9BcnJheTwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+VG90YWw8L3Y+CiAgICAgICAgICAgIDx2PkljZTwvdj4KICAgICAgICAgICAgPHY+S2Fobl8yYjwvdj4KICAgICAgICAgICAgPHY+S2Fobl8zYjwvdj4KICAgICAgICAgICAgPHY+V2F0ZXI8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlZHOTBZV3c9O1NXTmw7UzJGb2JsOHlZZz09O1MyRm9ibDh6WWc9PTtWMkYwWlhJPTs8L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iQW5jaWxsYXJ5RGF0YURlc2NyaXB0b3JzIj4KICAgICAgICA8RGltIHNpemU9IjMiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+QW5jRmlsZV9BcnJheTwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+YWNvc19BQ09TTDJmU3ViRmllbGRlcl8xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAxX1BvbEJfMTEwNDMwMTkyNzM5LmNvbmZpZzwvdj4KICAgICAgICAgICAgPHY+YWNvc19MMnNTdWJGaWVsZF9zZmlmXzEwMTIwOTE4NDQxNi54bWw8L3Y+CiAgICAgICAgICAgIDx2PmFjb3NfTDJzU3ViRmllbGRfYXBmXzEwMTAxOTIwNTkzNS54bWw8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPllXTnZjMTlCUTA5VFRESm1VM1ZpUm1sbGJHUmxjbDh4TVRBME1UbGZORE5mVUhKdlpIVmpkR2x2Ymw5Mk1URXdNVEV3WDB3eWN6STRNREJmY2pBeFgxQnZiRUpmTVRFd05ETXdNVGt5TnpNNUxtTnZibVpwWnc9PTtZV052YzE5TU1uTlRkV0pHYVdWc1pGOXpabWxtWHpFd01USXdPVEU0TkRReE5pNTRiV3c9O1lXTnZjMTlNTW5OVGRXSkdhV1ZzWkY5aGNHWmZNVEF4TURFNU1qQTFPVE0xTG5odGJBPT07PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IkF1dG9tYXRpY1F1YWxpdHlGbGFnIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj48L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPjwvZG1ycHA6dmxzYT4KICAgIDwvU3RyaW5nPgogICAgPFN0cmluZyBuYW1lPSJCdWlsZElkIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5CMi4wOC4wMDwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+UWpJdU1EZ3VNREE9PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IkNvbGxlY3Rpb25MYWJlbCI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+UHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAxPC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5VSEp2WkhWamRHbHZibDkyTVRFd01URXdYMHd5Y3pJNE1EQmZjakF4PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IkdyYW51bGVQb2ludGVyIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5hY29zX0wyc18xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAxX1BvbEJfMTEwNDMwMTkyNzM5Lmg1PC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5ZV052YzE5TU1uTmZNVEV3TkRFNVh6UXpYMUJ5YjJSMVkzUnBiMjVmZGpFeE1ERXhNRjlNTW5NeU9EQXdYM0l3TVY5UWIyeENYekV4TURRek1ERTVNamN6T1M1b05RPT08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iSERGVmVyc2lvbklkIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5IREY1IDEuOC41PC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5TRVJHTlNBeExqZ3VOUT09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IklucHV0UG9pbnRlciI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPklucHV0UHRyX0FycmF5PC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5hY29zX0wyZl8xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0wyZjI4MDBfcjAxX1BvbEJfMTEwNDMwMTkyMDUxLmg1PC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5ZV052YzE5TU1tWmZNVEV3TkRFNVh6UXpYMUJ5YjJSMVkzUnBiMjVmZGpFeE1ERXhNRjlNTW1ZeU9EQXdYM0l3TVY5UWIyeENYekV4TURRek1ERTVNakExTVM1b05RPT08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iSW5zdHJ1bWVudFNob3J0TmFtZSI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+VEFOU08tRlRTPC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5WRUZPVTA4dFJsUlQ8L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iTDJGdWxsUGh5c2ljc0FsZ29yaXRobURlc2NyaXB0b3IiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PjMtYmFuZCBmdWxsIHBoeXNpY3MgcmV0cmlldmFsPC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5NeTFpWVc1a0lHWjFiR3dnY0doNWMybGpjeUJ5WlhSeWFXVjJZV3c9PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IkwyRnVsbFBoeXNpY3NEYXRhVmVyc2lvbiI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+QjIuMDguMDAtMTA5Mjg8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlFqSXVNRGd1TURBdE1UQTVNamc9PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IkwyRnVsbFBoeXNpY3NFeGVWZXJzaW9uIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5CMi4wOC4wMC0xMDkyODwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+UWpJdU1EZ3VNREF0TVRBNU1qZz08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iTDJGdWxsUGh5c2ljc0lucHV0UG9pbnRlciI+CiAgICAgICAgPERpbSBzaXplPSIxMCIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5MMkZ1bGxQaHlzaWNzSW5wdXRQdHJfQXJyYXk8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2Pi9hY29zL3Byb2R1Y3QvUHJvZHVjdGlvbi92MTEwMTEwL0NsZDI4MDAvcjAxLzExMDQxOS9hY29zX0NsZF8xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0NsZDI4MDBfcjAxXzExMDQyOTE0NDgyNS5oNTwvdj4KICAgICAgICAgICAgPHY+L2Fjb3MvcHJvZHVjdC9Qcm9kdWN0aW9uL3YxMTAxMTAvRWNtMjgwMC9yMDEvMTEwNDE5L2Fjb3NfRWNtXzExMDQxOV80M19Qcm9kdWN0aW9uX3YxMTAxMTBfRWNtMjgwMF9yMDFfMTEwNDI5MTQ0NjQzLmg1PC92PgogICAgICAgICAgICA8dj4vYWNvcy9wcm9kdWN0L1Byb2R1Y3Rpb24vdjExMDExMC9MMWIyODAwL3IwMS8xMTA0MTkvYWNvc19MMWJfMTEwNDE5XzQzX1Byb2R1Y3Rpb25fdjExMDExMF9MMWIyODAwX3IwMV8xMTA0MjkxNDIwMDEuaDU8L3Y+CiAgICAgICAgICAgIDx2Pi9hY29zL3Byb2R1Y3QvUHJvZHVjdGlvbi92MTEwMTEwL1NuZDI4MDAvcjAxLzExMDQxOS9hY29zX1NuZF8xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX1NuZDI4MDBfcjAxXzExMDQyOTE1MTM0Ny50eHQ8L3Y+CiAgICAgICAgICAgIDx2Pi9kYXRhMS9kZXBsb3kvTDIvQjIuMDguMDAvTGV2ZWwyX0IyLjA4LjAwL2lucHV0L2dvc2F0L2NvbmZpZy9jb3htdW5rPC92PgogICAgICAgICAgICA8dj4vZGF0YTEvZGVwbG95L0wyL0IyLjA4LjAwL0xldmVsMl9CMi4wOC4wMC9pbnB1dC9nb3NhdC9jb25maWcvbGFtYmVydGlhbjwvdj4KICAgICAgICAgICAgPHY+L2RhdGExL2RlcGxveS9MMi9CMi4wOC4wMC9MZXZlbDJfQjIuMDguMDAvaW5wdXQvZ29zYXQvaW5wdXQvc3RhdGljPC92PgogICAgICAgICAgICA8dj4vZ3JvdXBzL2FsZ29yaXRobS9sMl9mcC9hYnNjby92My4yLjAvY28yXzQ3MDAuMC02NTAwLjBfdjMuMi4wLmNodW5rLmhkZjwvdj4KICAgICAgICAgICAgPHY+L2dyb3Vwcy9hbGdvcml0aG0vbDJfZnAvYWJzY28vdjMuMi4wL2gyb180NzAwLjAtNjUwMC4wX3YzLjIuMC5jaHVuay5oZGY8L3Y+CiAgICAgICAgICAgIDx2Pi9ncm91cHMvYWxnb3JpdGhtL2wyX2ZwL2Fic2NvL3YzLjIuMC9vMl8xMjc0NS4wLTEzMjQ1LjBfdjMuMi4wLmNodW5rLmhkZjwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+TDJGamIzTXZjSEp2WkhWamRDOVFjbTlrZFdOMGFXOXVMM1l4TVRBeE1UQXZRMnhrTWpnd01DOXlNREV2TVRFd05ERTVMMkZqYjNOZlEyeGtYekV4TURReE9WODBNMTlRY205a2RXTjBhVzl1WDNZeE1UQXhNVEJmUTJ4a01qZ3dNRjl5TURGZk1URXdOREk1TVRRME9ESTFMbWcxO0wyRmpiM012Y0hKdlpIVmpkQzlRY205a2RXTjBhVzl1TDNZeE1UQXhNVEF2UldOdE1qZ3dNQzl5TURFdk1URXdOREU1TDJGamIzTmZSV050WHpFeE1EUXhPVjgwTTE5UWNtOWtkV04wYVc5dVgzWXhNVEF4TVRCZlJXTnRNamd3TUY5eU1ERmZNVEV3TkRJNU1UUTBOalF6TG1nMTtMMkZqYjNNdmNISnZaSFZqZEM5UWNtOWtkV04wYVc5dUwzWXhNVEF4TVRBdlRERmlNamd3TUM5eU1ERXZNVEV3TkRFNUwyRmpiM05mVERGaVh6RXhNRFF4T1Y4ME0xOVFjbTlrZFdOMGFXOXVYM1l4TVRBeE1UQmZUREZpTWpnd01GOXlNREZmTVRFd05ESTVNVFF5TURBeExtZzE7TDJGamIzTXZjSEp2WkhWamRDOVFjbTlrZFdOMGFXOXVMM1l4TVRBeE1UQXZVMjVrTWpnd01DOXlNREV2TVRFd05ERTVMMkZqYjNOZlUyNWtYekV4TURReE9WODBNMTlRY205a2RXTjBhVzl1WDNZeE1UQXhNVEJmVTI1a01qZ3dNRjl5TURGZk1URXdOREk1TVRVeE16UTNMblI0ZEE9PTtMMlJoZEdFeEwyUmxjR3h2ZVM5TU1pOUNNaTR3T0M0d01DOU1aWFpsYkRKZlFqSXVNRGd1TURBdmFXNXdkWFF2WjI5ellYUXZZMjl1Wm1sbkwyTnZlRzExYm1zPTtMMlJoZEdFeEwyUmxjR3h2ZVM5TU1pOUNNaTR3T0M0d01DOU1aWFpsYkRKZlFqSXVNRGd1TURBdmFXNXdkWFF2WjI5ellYUXZZMjl1Wm1sbkwyeGhiV0psY25ScFlXND07TDJSaGRHRXhMMlJsY0d4dmVTOU1NaTlDTWk0d09DNHdNQzlNWlhabGJESmZRakl1TURndU1EQXZhVzV3ZFhRdloyOXpZWFF2YVc1d2RYUXZjM1JoZEdsajtMMmR5YjNWd2N5OWhiR2R2Y21sMGFHMHZiREpmWm5BdllXSnpZMjh2ZGpNdU1pNHdMMk52TWw4ME56QXdMakF0TmpVd01DNHdYM1l6TGpJdU1DNWphSFZ1YXk1b1pHWT07TDJkeWIzVndjeTloYkdkdmNtbDBhRzB2YkRKZlpuQXZZV0p6WTI4dmRqTXVNaTR3TDJneWIxODBOekF3TGpBdE5qVXdNQzR3WDNZekxqSXVNQzVqYUhWdWF5NW9aR1k9O0wyZHliM1Z3Y3k5aGJHZHZjbWwwYUcwdmJESmZabkF2WVdKelkyOHZkak11TWk0d0wyOHlYekV5TnpRMUxqQXRNVE15TkRVdU1GOTJNeTR5TGpBdVkyaDFibXN1YUdSbTs8L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iTDJGdWxsUGh5c2ljc09wZXJhdGlvbnNWZXJzaW9uIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5CMi4wOC4wMC0xMDkyODwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+UWpJdU1EZ3VNREF0TVRBNU1qZz08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iTG9uZ05hbWUiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PkFDT1MgR09TQVQvVEFOU08tRlRTIExldmVsIDIgRnVsbCBQaHlzaWNzIFN0YW5kYXJkIFByb2R1Y3Q8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlFVTlBVeUJIVDFOQlZDOVVRVTVUVHkxR1ZGTWdUR1YyWld3Z01pQkdkV3hzSUZCb2VYTnBZM01nVTNSaGJtUmhjbVFnVUhKdlpIVmpkQT09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8SW50MzIgbmFtZT0iTWlzc2luZ0V4cG9zdXJlcyI+CiAgICAgICAgPERpbSBzaXplPSIzIi8+CiAgICAgICAgPERpbSBzaXplPSIyIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPkJhbmRfUG9sYXJpemF0aW9uX0FycmF5PC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TaWduZWQzMjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOmNodW5rcyBjb21wcmVzc2lvblR5cGU9ImRlZmxhdGUiIGRlZmxhdGVMZXZlbD0iMiIgZmlsbFZhbHVlPSIwIiBieXRlT3JkZXI9IkxFIj4KICAgICAgICAgICAgPGRtcnBwOmNodW5rRGltZW5zaW9uU2l6ZXM+MyAyPC9kbXJwcDpjaHVua0RpbWVuc2lvblNpemVzPgogICAgICAgICAgICA8ZG1ycHA6Y2h1bmsgb2Zmc2V0PSI0NDA0OTYiIG5CeXRlcz0iMTEiIGNodW5rUG9zaXRpb25JbkFycmF5PSJbMCwwXSIvPgogICAgICAgIDwvZG1ycHA6Y2h1bmtzPgogICAgPC9JbnQzMj4KICAgIDxTdHJpbmcgbmFtZT0iTm9taW5hbERheSI+CiAgICA8U3RyaW5nIG5hbWU9IlBsYXRmb3JtTG9uZ05hbWUiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PkdyZWVuaG91c2UgZ2FzZXMgT2JzZXJ2aW5nIFNBVGVsbGl0ZTwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+UjNKbFpXNW9iM1Z6WlNCbllYTmxjeUJQWW5ObGNuWnBibWNnVTBGVVpXeHNhWFJsPC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlBsYXRmb3JtU2hvcnROYW1lIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5HT1NBVDwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+UjA5VFFWUT08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iUGxhdGZvcm1UeXBlIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5zcGFjZWNyYWZ0PC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5jM0JoWTJWamNtRm1kQT09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlByb2Nlc3NpbmdMZXZlbCI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+TGV2ZWwgMjwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+VEdWMlpXd2dNZz09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlByb2R1Y2VyQWdlbmN5Ij4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5OQVNBPC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5Ua0ZUUVE9PTwvZG1ycHA6dmxzYT4KICAgIDwvU3RyaW5nPgogICAgPFN0cmluZyBuYW1lPSJQcm9kdWNlckluc3RpdHV0aW9uIj4KICAgICAgICA8RGltIHNpemU9IjEiLz4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlNoYXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+U2NhbGFyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj5KUEw8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlNsQk08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIAogICAgCiAgICA8U3RyaW5nIG5hbWU9IlByb2R1Y3Rpb25Mb2NhdGlvbiI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+T3BlcmF0aW9ucyBQaXBlbGluZTwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+VDNCbGNtRjBhVzl1Y3lCUWFYQmxiR2x1WlE9PTwvZG1ycHA6dmxzYT4KICAgIDwvU3RyaW5nPgogICAgPFN0cmluZyBuYW1lPSJQcm9qZWN0SWQiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PkFDT1M8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlFVTlBVdz09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlFBR3JhbnVsZVBvaW50ZXIiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PmFjb3NfcUwyc18xMTA0MTlfNDNfUHJvZHVjdGlvbl92MTEwMTEwX0wyczI4MDBfcjAxX1BvbEJfMTEwNDMwMTkyNzM5LnR4dDwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+WVdOdmMxOXhUREp6WHpFeE1EUXhPVjgwTTE5UWNtOWtkV04wYVc5dVgzWXhNVEF4TVRCZlRESnpNamd3TUY5eU1ERmZVRzlzUWw4eE1UQTBNekF4T1RJM016a3VkSGgwPC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlNJU05hbWUiPgogICAgICAgIDxEaW0gc2l6ZT0iMSIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5TY2FsYXI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iVHlwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlZhckxlblN0cjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPGRtcnBwOnZsc2E+CiAgICAgICAgICAgIDx2PmFjb3Nfdm9sdW1lcy54bHN4PC92PgogICAgICAgIDwvZG1ycHA6dmxzYT4KICAgICAgICA8ZG1ycHA6dmxzYT5ZV052YzE5MmIyeDFiV1Z6TG5oc2MzZz08L2RtcnBwOnZsc2E+CiAgICA8L1N0cmluZz4KICAgIDxTdHJpbmcgbmFtZT0iU0lTVmVyc2lvbiI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+SW5pdGlhbDwvdj4KICAgICAgICA8L2RtcnBwOnZsc2E+CiAgICAgICAgPGRtcnBwOnZsc2E+U1c1cGRHbGhiQT09PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CiAgICA8U3RyaW5nIG5hbWU9IlNob3J0TmFtZSI+CiAgICAgICAgPERpbSBzaXplPSIxIi8+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJTaGFwZSIgdHlwZT0iU3RyaW5nIj4KICAgICAgICAgICAgPFZhbHVlPlNjYWxhcjwvVmFsdWU+CiAgICAgICAgPC9BdHRyaWJ1dGU+CiAgICAgICAgPEF0dHJpYnV0ZSBuYW1lPSJUeXBlIiB0eXBlPSJTdHJpbmciPgogICAgICAgICAgICA8VmFsdWU+VmFyTGVuU3RyPC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8ZG1ycHA6dmxzYT4KICAgICAgICAgICAgPHY+QUNPU19MMlM8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPlFVTlBVMTlNTWxNPTwvZG1ycHA6dmxzYT4KICAgIDwvU3RyaW5nPgogICAgPFN0cmluZyBuYW1lPSJTcGVjdHJhbENoYW5uZWwiPgogICAgICAgIDxEaW0gc2l6ZT0iMyIvPgogICAgICAgIDxBdHRyaWJ1dGUgbmFtZT0iU2hhcGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5CYW5kX0FycmF5PC9WYWx1ZT4KICAgICAgICA8L0F0dHJpYnV0ZT4KICAgICAgICA8QXR0cmlidXRlIG5hbWU9IlR5cGUiIHR5cGU9IlN0cmluZyI+CiAgICAgICAgICAgIDxWYWx1ZT5WYXJMZW5TdHI8L1ZhbHVlPgogICAgICAgIDwvQXR0cmlidXRlPgogICAgICAgIDxkbXJwcDp2bHNhPgogICAgICAgICAgICA8dj4wLjc2dW0gTzIgQS1iYW5kPC92PgogICAgICAgICAgICA8dj4xLjZ1bSBXZWFrIENPMjwvdj4KICAgICAgICAgICAgPHY+Mi4wNnVtIFN0cm9uZyBDTzI8L3Y+CiAgICAgICAgPC9kbXJwcDp2bHNhPgogICAgICAgIDxkbXJwcDp2bHNhPk1DNDNOblZ0SUU4eUlFRXRZbUZ1WkE9PTtNUzQyZFcwZ1YyVmhheUJEVHpJPTtNaTR3Tm5WdElGTjBjbTl1WnlCRFR6ST07PC9kbXJwcDp2bHNhPgogICAgPC9TdHJpbmc+CjwvRGF0YXNldD4K;



21165 - 15873 = 5292 additonal characters = 133.3% increase