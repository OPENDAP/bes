<?xml version="1.0" encoding="UTF-8"?>
<bes:request xmlns:bes="http://xml.opendap.org/ns/bes/1.0#" reqID="http-nio-8080-exec-9_53" reqUUID="c8d84056-76fe-45ca-a648-5dcd195941f3" clientId="/-0" clientCmdCount="8">
  <bes:setContext name="bes_timeout">300</bes:setContext>
  <bes:setContext name="dap_explicit_containers">no</bes:setContext>
  <bes:setContext name="errors">xml</bes:setContext>
  <bes:setContext name="olfsLog">0:0:0:0:0:0:0:1|&amp;|hyrax|&amp;|27E91FF793712616BF3FB9CDFE136C48|&amp;|hyrax|&amp;|1781122998861|&amp;|http-nio-8080-exec-9_53-c8d84056-76fe-45ca-a648-5dcd195941f3|&amp;|HTTP-GET|&amp;|/hyrax/data/nc/fnoc1.nc.dmr.html|&amp;|-</bes:setContext>
  <bes:setContext name="xml:base">http://localhost:8080/data/nc/fnoc1.nc</bes:setContext>
  <bes:setContext name="max_response_size">0</bes:setContext>
  <bes:setContext name="max_variable_size">0</bes:setContext>
  <bes:setContext name="uid">Bugsy</bes:setContext>
  <bes:setContext name="edl_client_application_id">SSCATdwCtNkEWliH3_WzDw</bes:setContext>
  <bes:setContext name="edl_auth_token">Bearer eyJ0eXAiOiJKV1QiLCJvcmlnaW4iOiJFYXJ0aGRhdGEgTG9naW4iLCJzaWciOiJlZGxqd3RwdWJrZXlfb3BzIiwiYWxnIjoiUlMyNTYifQ.eyJ0eXBlIjoiT0F1dGgiLCJjbGllbnRfaWQiOiJTU0NBVGR3Q3ROa0VXbGlIM19XekR3IiwiaWF0IjoxNzgxMTIyOTQzLCJpc3MiOiJodHRwczovL3Vycy5lYXJ0aGRhdGEubmFzYS5nb3YiLCJleHAiOjE3ODEyMDkzNDMsInVpZCI6Imh5cmF4IiwiaWRlbnRpdHlfcHJvdmlkZXIiOiJlZGxfb3BzIiwiYXNzdXJhbmNlX2xldmVsIjozLCJhY3IiOiJlZGwifQ.FVt6kcZqLJFdVqPz9Gy7xqFKuVJpqa57b3XEfQ_Tc9HXvqJ-D7Kk4VA-q8A6iMJx93c6fJtO1DxVeqzvodoc7Ynde_oNdGL7DTVcb9bs9TM660YHibSViMnMI9CL57_bONMJwRKVWirZEB1vsADwgyMFXujZhxLcxkQVZ8YNINVjMXS41oo2gqLzGU_yubvNRaQ9oQFKWmngpvb7v1XiQ7pDYvH4uVdytR-U7XpVdgwGATn5i59tMJvA69KLtAhJDH7GxvgDnGBf2mWqZNEiGs3XSRhNq-ELRVTsMVCmg4q0OqNn-ojpDXadlxUIcROnfJ1QhkwPFaMr4k0x2OzViA</bes:setContext>
  <bes:setContext name="dap4_checksums">false</bes:setContext>
  <bes:setContainer name="catalogContainer" space="gateway">http://localhost:8080/data/nc/fnoc1.nc</bes:setContainer>
  <bes:define name="d1" space="default">
    <bes:container name="catalogContainer" />
  </bes:define>
  <bes:get type="dds" definition="d1" />
</bes:request>
