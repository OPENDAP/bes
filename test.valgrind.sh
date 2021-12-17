#!/bin/bash
echo ” CF DMR valgrind Test“
cd bes-testsuite
/bin/cp -f hdf5_handlerTest.valgrind.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.cf.dmr.valgrind.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi

/bin/rm -rf /tmp/h5.cf.dmr.valgrind.log

echo “NASA DAP2 valgrind test”
cd bes-testsuite
/bin/cp -f hdf5_handlerTest.valgrind.nasa.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.valgrind.nasa.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi

/bin/rm -rf /tmp/h5.valgrind.nasa.log

echo “NASA CF DMR valgrind test”
cd bes-testsuite
cp hdf5_handlerTest.valgrind.nasa.cfdmr.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.valgrind.nasa.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi
/bin/rm -rf /tmp/h5.valgrind.nasa.log

echo “NASA Default DAP4 valgrind test”
cd bes-testsuite
/bin/cp -f hdf5_handlerTest.valgrind.nasa.deault.dap4.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.valgrind.nasa.default.dap4.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi
/bin/rm -rf /tmp/h5.valgrind.nasa.default.dap4.log 

echo” CF DAP2 to DMR valgrind Test“
cd bes-testsuite
/bin/cp -f hdf5_handlerTest.dap2dmr.valgrind.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.valgrind.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi

/bin/rm -rf /tmp/h5.valgrind.log

echo” NASA CF DMR ONLY valgrind Test“
cd bes-testsuite
/bin/cp -f hdf5_handlerTest.dap2dmr.valgrind.at hdf5_handlerTest.at
cd ..
make check
/bin/cp -f /tmp/h5.valgrind.log tt.log
/bin/bash ./test_valgrind_unit.sh
status=$?
if test "$status" == "0"; then
   echo "OK"
else
   echo "FAIL"
   exit
fi
/bin/rm -rf /tmp/h5.valgrind.log

/bin/rm -rf tt.log

