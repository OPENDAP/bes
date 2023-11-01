<?xml version="1.0" encoding="UTF-8"?>
<request reqID="some_unique_value" >
    <setContext name="dap_format">dap4</setContext>
    <!-- setContainer name="c" space="catalog">/data/grid_1_2d.h5.dds</setContainer -->
    <setContainer name="c" space="builddmrpp">/collections/C2036877806-POCLOUD/granules/20220812010000-OSISAF-L3C_GHRSST-SSTsubskin-GOES16-ssteqc_goes16_20220812_010000-v02.0-fv01.0</setContainer>
    <define name="d">
	   <container name="c" />
    </define>
    <get type="dap" definition="d" returnAs="dmrpp"/>
</request>
