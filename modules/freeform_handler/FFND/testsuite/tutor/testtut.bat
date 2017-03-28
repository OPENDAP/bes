@echo off
if exist error.msg del error.msg
echo "**************************************************************"
echo "TESTING the FREEFORM Tutorial" 
echo "**************************************************************"
echo "**************************************************************"
echo "TESTING latlon (DOUBLE)" 
echo "**************************************************************"
echo "newform latlon.dat -if latlon.afm -of latlond.bfm > latlond.bin" 
newform latlon.dat -if latlon.afm -of latlond.bfm > latlond.bin
echo "newform latlond.bin -if latlond.bfm -of latlon.afm > latlon.tst" 
newform latlond.bin -if latlond.bfm -of latlon.afm > latlon.tst
diff -Z latlon.tst latlon.dat > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING latlon (LONG)" 
echo "*********************************************************"
echo "newform latlon.dat -if latlon.afm -of latlon.bfm > latlon.bin" 
newform latlon.dat -if latlon.afm -of latlon.bfm > latlon.bin
echo "newform latlon.bin -if latlon.bfm -of latlon.afm > latlon.tst" 
newform latlon.bin -if latlon.bfm -of latlon.afm > latlon.tst
diff -Z latlon.tst latlon.dat > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING latlon (deg_min_sec)" 
echo "*********************************************************"
echo "newform latlon.bin -if latlon.bfm -of d_m_s.afm > d_m_s.tst" 
newform latlon.bin -if latlon.bfm -of d_m_s.afm > d_m_s.tst
diff -Z d_m_s.tst d_m_s.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING latlon (deg_min)" 
echo "*********************************************************"
echo "newform latlon.bin -if latlon.bfm -of d_m.afm > d_m.tst" 
newform latlon.bin -if latlon.bfm -of d_m.afm > d_m.tst
diff -Z d_m.tst d_m.sav > trash
if errorlevel 1 echo "ERROR LEVEL WORKS!!!"
if errorlevel 1 echo "ERRORLEVEL stays alive!"
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING latlon (degabsm)" 
echo "*********************************************************"
echo "newform latlon.bin -if latlon.bfm -of degabsm.afm > degabsm.tst" 
newform latlon.bin -if latlon.bfm -of degabsm.afm > degabsm.tst
diff -Z degabsm.tst degabsm.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING latlon (deg_min_geog)" 
echo "*********************************************************"
echo "newform latlon.bin -if latlon.bfm -of degmgeog.afm > degmgeog.tst" 
newform latlon.bin -if latlon.bfm -of degmgeog.afm > degmgeog.tst
diff -Z degmgeog.tst degmgeog.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING Degrees, Minutes, Seconds conversion"
echo "*********************************************************"
echo "newform latlon2.bin -f ll_d_m_s.fmt -o ll_d_m_s.dat"
del ll_d_m_s.dat
newform latlon2.bin -f ll_d_m_s.fmt -o ll_d_m_s.dat
diff -Z ll_d_m_s.dat ll_d_m_s.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING Date and Time conversions"
echo "*********************************************************"
echo "*********************************************************"
echo "TESTING Year/Month/Day formatting"
echo "*********************************************************"
echo "newform mdy.dat -f yymmdd.fmt -o yymmdd.dat"
del yymmdd.dat
newform mdy.dat -f yymmdd.fmt -o yymmdd.dat
diff -Z yymmdd.dat yymmdd.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
echo "*********************************************************"
echo "TESTING Serial Time conversions"
echo "*********************************************************"
echo "newform time.dat -f serial.fmt -ift "ASCII ymdhms date" -o serial.bin"
del serial.bin
newform time.dat -f serial.fmt -ift "ASCII ymdhms date" -o serial.bin
echo "newform serial.bin -oft "ASCII serial date" -o serial.dat"
del serial.dat
newform serial.bin -oft "ASCII serial date" -o serial.dat
diff -Z serial.dat serial.sav > trash
rem if errorlevel 1 goto errmsg
call chktrash
if exist error.msg goto errmsg
echo,
echo "***** TUTORIAL TEST WORKED LIKE A CHARM *****"
echo,
goto end
:errmsg
echo,
echo "FAILURE in tutorial portion of test suite"
echo,
:end
