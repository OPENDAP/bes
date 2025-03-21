<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="http-nio-8080-exec-5_22_9d50c3cb-5d37-4ed9-9917-45ec5a37cde4" besClient="/-0">
  <bes:setContext name="bes_timeout">300</bes:setContext>
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="olfsLog">0:0:0:0:0:0:0:1|&amp;|Mozilla#5#0 #Macintosh# Intel Mac OS X 10_15_7# AppleWebKit#605#1#15 #KHTML# like Gecko# Version#18#1#1 Safari#605#1#15|&amp;|-|&amp;|-|&amp;|1741294080854|&amp;|http-nio-8080-exec-5_22_9d50c3cb-5d37-4ed9-9917-45ec5a37cde4|&amp;|HTTP-GET|&amp;|/opendap/hyrax/Scalar_contiguous_vlstr.h5.dmrpp.dmr.html|&amp;|-</bes:setContext>
  <bes:setContext name="xml:base">http://localhost:8080/opendap/Scalar_contiguous_vlstr.h5.dmrpp</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  <bes:setContext name="max_variable_size">0</bes:setContext>
  <bes:setContext name="uid">not_logged_in</bes:setContext>
  <bes:setContainer name="catalogContainer" space="catalog">/Scalar_contiguous_vlstr.h5.dmrpp</bes:setContainer>
  <bes:define name="d1" space="default">
    <bes:container name="catalogContainer" />
  </bes:define>
  <bes:get type="dmr" definition="d1" />
</bes:request>
