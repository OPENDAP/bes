<?xml version="1.0" encoding="ISO-8859-1"?>
<response reqID="some_unique_value" xmlns="http://xml.opendap.org/ns/bes/1.0#">
    <getDAP>
        <BESError>
            <Type>3</Type>
            <Message>This dataset contains variables/attributes whose data types are not compatible with the NetCDF-3 data model. If your request includes any of these incompatible variables or attributes and you choose the &#8220;NetCDF-3&#8221; download encoding, your request will FAIL.&#13;
					 You may also try constraining your request to omit the problematic data type(s), or ask for a different encoding such as DAP4 binary or NetCDF-4&#13;
					 [ Number of non-compatible variables: 3 ] &#13;
						 [ Variable: b2 | Type: Int8 ] &#13;
						 [ Variable: i64 | Type: Array ] &#13;
						 [ Variable: b4 | Type: Array ] </Message>
            <Administrator>support@opendap.org</Administrator>
            <Location>
                <File>FONcTransform.cc</File>
                <Line>728</Line>
            </Location>
        </BESError>
    </getDAP>
</response>
