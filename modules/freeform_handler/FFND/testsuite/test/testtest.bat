@echo off
if exist error.msg del error.msg
echo,
echo "*******************************************************************"
echo "TESTING TEST DATA: MULTIPLE VARIABLE TYPES ASCII TO BINARY TO ASCII"
echo "*******************************************************************"
echo,
rem Test the test data file ASCII to BINARY to ASCII
echo "newform test.dat -f test1.fmt > testbin.tst"
newform test.dat -f test1.fmt > testbin.tst
echo "newform testbin.tst -f test2.fmt > testasc.tst"
newform testbin.tst -f test2.fmt > testasc.tst
rem differ from test.dat because 0's appear from binary to ascii
diff -Z test.dat testasc.tst > trash
call chktrash
if errorlevel 1 goto errmsg
echo,
echo "*******************************************************************"
echo "TESTING TEST DATA: MULTIPLE VARIABLE TYPES -- ASCII TO REALIGNED "
echo "BINARY TO VAR1s BINARY (long and ulong changed to floats) TO"
echo "REALIGNED VAR1's ASCII (floats converted back to long and ulong)"
echo "*******************************************************************"
echo,
echo "newform test.dat -f test3.fmt > testalgn.bin"
newform test.dat -f test3.fmt > testalgn.bin
echo "newform testalgn.bin -f test4.fmt > testvar1.bin"
newform testalgn.bin -f test4.fmt > testvar1.bin
echo "newform testvar1.bin -f test5.fmt > testvar1.dat"
newform testvar1.bin -f test5.fmt > testvar1.dat
diff -Z testvar1.dat testvar1.sav > trash
call chktrash
if errorlevel 1 goto errmsg
del testalgn.bin
del testvar1.bin
del testvar1.dat
echo,
echo "*******************************************************************"
echo "TESTING TEST DATA: HEAD and TAIL"
echo "*******************************************************************"
echo,
echo "newform test.dat -f test6.fmt -c 5 > head5.tst"
newform test.dat -f test6.fmt -c 5 > head5.tst
diff -Z head5.tst head5.sav > trash
call chktrash
if errorlevel 1 goto errmsg
echo "newform test.dat -f test7.fmt -c -5 > tail5.tst"
newform test.dat -f test7.fmt -c -5 > tail5.tst
diff -Z tail5.tst tail5.sav > trash
call chktrash
if errorlevel 1 goto errmsg
echo,
echo "*******************************************************************"
echo "GRATUITOUS HEADER TESTS"
echo "*******************************************************************"
echo,
echo "*******************************************************************"
echo "EMBEDDED FILE AND RECORD HEADER"
echo "*******************************************************************"
echo,
echo "newform aeromag.sav -f aeromaga.fmt > aeromag.bin"
newform aeromag.sav -f aeromaga.fmt > aeromag.bin
echo "newform aeromag.bin -f aeromagb.fmt > aeromag.dat"
newform aeromag.bin -f aeromagb.fmt > aeromag.dat
diff -Z aeromag.dat aeromag.sav > trash
call chktrash
if errorlevel 1 goto errmsg
echo "newform aeromag.dat -f aeromaga.fmt > aeromag.tst"
newform aeromag.dat -f aeromaga.fmt > aeromag.tst
diff -Z aeromag.bin aeromag.tst > trash
call chktrash
if errorlevel 1 goto errmsg
echo,
echo "*******************************************************************"
echo "SEPARATE FILE AND EMBEDDED RECORD HEADER"
echo "*******************************************************************"
echo,
echo "newform aeromag1.sav -f aeromg1a.fmt > aeromag1.bin"
newform aeromag1.sav -f aeromg1a.fmt > aeromag1.bin
diff -Z aeromag1.bin aeromag.bin > trash
call chktrash
if errorlevel 1 goto errmsg
echo "newform aeromag1.bin -f aeromagb.fmt > aeromag1.dat"
newform aeromag1.bin -f aeromagb.fmt > aeromag1.dat
diff -Z aeromag1.dat aeromag.sav > trash
call chktrash
if errorlevel 1 goto errmsg
echo,
echo "*******************************************************************"
echo "EMBEDDED FILE AND RECORD HEADER TO SEPARATE FILE AND RECORD HEADER"
echo "*******************************************************************"
echo,
del temp.bin
del septest.hdr
echo "newform aeromag.sav -f aeromags.fmt -o temp.bin"
newform aeromag.sav -f aeromags.fmt -o temp.bin
diff -Z temp.bin sepbin.sav > trash
call chktrash
if errorlevel 1 goto errmsg
diff -Z septest.hdr sephdr.sav > trash
call chktrash
if errorlevel 1 goto errmsg

if exist error.msg goto errmsg
echo,
echo "***** TEST FILE ASCII TO BINARY TO ASCII WORKED LIKE A CHARM *****"
echo,
goto end
:errmsg
echo,
echo "FAILURE in test portion of test suite"
echo,
:end
