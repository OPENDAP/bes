@echo off
type c:\bin\space | date 
rse c:\bin\newform > trash
type trash
del trash
echo. 
echo "*******************************************************************"
echo "                   TESTING PRECISION MANIPULATIONS"
echo "*******************************************************************"
echo.
rem
rem This section tests the FREEFORM Precision Calculations
rem
rem	The data is in the file testprec.in:
rem		1234567
rem		      1
rem		     12
rem		    123
rem		   1234
rem		  12345
rem		 123456
rem		1234567
rem
rem	These numbers are:
rem	   Precision = 1   Precision = 3   Precision = 5
rem		      .1    	    .001     	  .00001
rem		     1.2    	    .012     	  .00012
rem		    12.3    	    .123     	  .00123
rem		   123.4    	   1.234     	  .01234
rem		  1234.5    	  12.345     	  .12345
rem		 12345.6    	 123.456     	 1.23456
rem		123456.7    	1234.567     	12.34567
rem
echo. 
echo "**************************************************************"
echo "      TESTING NINE ASCII LONG TO ASCII FLOAT COMBINATIONS "
echo "**************************************************************"
echo.
rem                   convert  binary
rem                   to         1 3 5
rem                   ASCII    1      
rem                            3      
rem                            5      
rem
echo. 
echo "TESTING ASCII LONG(1) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l1.afm prec_f1.afm testprec.in al1af1.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to ASCII FLOAT(1) "
echo. 
echo "TESTING ASCII LONG(3) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l3.afm prec_f3.afm testprec.in al3af3.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to ASCII FLOAT(3) "
echo. 
echo "TESTING ASCII LONG(5) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l5.afm prec_f5.afm testprec.in al5af5.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to ASCII FLOAT(5) "
echo. 
echo "TESTING ASCII LONG(3) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l3.afm prec_f1.afm testprec.in al3af1.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to ASCII FLOAT(1) "
echo. 
echo "TESTING ASCII LONG(5) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l5.afm prec_f1.afm testprec.in al5af1.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to ASCII FLOAT(1) "
echo. 
echo "TESTING ASCII LONG(5) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l5.afm prec_f3.afm testprec.in al5af3.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to ASCII FLOAT(3) "
echo. 
echo "TESTING ASCII LONG(1) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l1.afm prec_f3.afm testprec.in al1af3.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to ASCII FLOAT(3) "
echo. 
echo "TESTING ASCII LONG(1) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l1.afm prec_f5.afm testprec.in al1af5.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to ASCII FLOAT(5) "
echo. 
echo "TESTING ASCII LONG(3) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l3.afm prec_f5.afm testprec.in al3af5.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to ASCII FLOAT(5) "
echo. 
echo. 
echo "**************************************************************"
echo "     TESTING NINE BINARY LONG TO ASCII FLOAT COMBINATIONS "
echo "**************************************************************"
echo.
rem                   convert  binary
rem                   to         1 3 5
rem                   ASCII    1 x x x
rem                            3 x x x
rem                            5 x x x
rem
echo. 
echo "TESTING BINARY LONG(1) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l1.bfm prec_f1.afm testprec.bin bl1af1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII FLOAT(1) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l3.bfm prec_f3.afm testprec.bin bl3af3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII FLOAT(3) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l5.bfm prec_f5.afm testprec.bin bl5af5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII FLOAT(5) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l3.bfm prec_f1.afm testprec.bin bl3af1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII FLOAT(1) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII FLOAT(1) "
echo.
call bin2asc prec_l5.bfm prec_f1.afm testprec.bin bl5af1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII FLOAT(1) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l5.bfm prec_f3.afm testprec.bin bl5af3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII FLOAT(3) "
echo. 
echo "TESTING BINARY LONG(1) to ASCII FLOAT(3) "
echo.
call bin2asc prec_l1.bfm prec_f3.afm testprec.bin bl1af3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII FLOAT(3) "
echo. 
echo "TESTING BINARY LONG(1) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l1.bfm prec_f5.afm testprec.bin bl1af5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII FLOAT(5) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII FLOAT(5) "
echo.
call bin2asc prec_l3.bfm prec_f5.afm testprec.bin bl3af5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII FLOAT(5) "
echo. 
echo. 
echo. 
echo "**************************************************************"
echo "TESTING NINE ASCII to BINARY LONG COMBINATIONS "
echo "**************************************************************"
echo.
rem                     convert  ASCII
rem                     to       1 3 5
rem                     binary 1 x x x
rem                            3 x x x
rem                            5 x x x
rem
rem
echo. 
echo "TESTING ASCII LONG(1) to BINARY LONG(1) "
echo.
call asc2bin prec_l1.afm prec_l1.bfm testprec.in sameprec.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to BINARY LONG(1) "
echo. 
echo "TESTING ASCII LONG(3) to BINARY LONG(3) "
echo.
call asc2bin prec_l3.afm prec_l3.bfm testprec.in sameprec.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to BINARY LONG(3) "
echo. 
echo "TESTING ASCII LONG(5) to BINARY LONG(5) "
echo.
call asc2bin prec_l5.afm prec_l5.bfm testprec.in sameprec.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to BINARY LONG(5) "
echo. 
echo "TESTING ASCII LONG(1) to BINARY LONG(3) "
echo.
call asc2bin prec_l1.afm prec_l3.bfm testprec.in asc1bin3.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to BINARY LONG(3) "
echo. 
echo "TESTING ASCII LONG(1) to BINARY LONG(5) "
echo.
call asc2bin prec_l1.afm prec_l5.bfm testprec.in asc1bin5.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(1) to BINARY LONG(5) "
echo. 
echo "TESTING ASCII LONG(3) to BINARY LONG(1) "
echo.
call asc2bin prec_l3.afm prec_l1.bfm testprec.in asc3bin1.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to BINARY LONG(1) "
echo. 
echo "TESTING ASCII LONG(3) to BINARY LONG(5) "
echo.
call asc2bin prec_l3.afm prec_l5.bfm testprec.in asc3bin5.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(3) to BINARY LONG(5) "
echo. 
echo "TESTING ASCII LONG(5) to BINARY LONG(1) "
echo.
call asc2bin prec_l5.afm prec_l1.bfm testprec.in asc5bin1.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to BINARY LONG(1) "
echo. 
echo "TESTING ASCII LONG(5) to BINARY LONG(3) "
echo.
call asc2bin prec_l5.afm prec_l3.bfm testprec.in asc5bin3.sav
call chktrash
echo.
echo "DONE TESTING ASCII LONG(5) to BINARY LONG(3) "
echo. 
echo. 
echo "**************************************************************"
echo "TESTING NINE BINARY TO ASCII LONG COMBINATIONS "
echo "**************************************************************"
echo.
rem                   convert  binary
rem                   to         1 3 5
rem                   ASCII    1 x x x 
rem                            3 x x x 
rem                            5 x x x 
rem
echo. 
echo "TESTING BINARY LONG(1) to ASCII LONG(1) "
echo.
call bin2asc prec_l1.bfm prec_l1.afm testprec.bin bin1asc1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII LONG(1) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII LONG(3) "
echo.
call bin2asc prec_l3.bfm prec_l3.afm testprec.bin bin3asc3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII LONG(3) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII LONG(5) "
echo.
call bin2asc prec_l5.bfm prec_l5.afm testprec.bin bin5asc5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII LONG(5) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII LONG(1) "
echo.
call bin2asc prec_l3.bfm prec_l1.afm testprec.bin bin3asc1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII LONG(1) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII LONG(1) "
echo.
call bin2asc prec_l5.bfm prec_l1.afm testprec.bin bin5asc1.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII LONG(1) "
echo. 
echo "TESTING BINARY LONG(5) to ASCII LONG(3) "
echo.
call bin2asc prec_l5.bfm prec_l3.afm testprec.bin bin5asc3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(5) to ASCII LONG(3) "
echo. 
echo "TESTING BINARY LONG(1) to ASCII LONG(3) "
echo.
call bin2asc prec_l1.bfm prec_l3.afm testprec.bin bin1asc3.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII LONG(3) "
echo. 
echo "TESTING BINARY LONG(1) to ASCII LONG(5) "
echo.
call bin2asc prec_l1.bfm prec_l5.afm testprec.bin bin1asc5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(1) to ASCII LONG(5) "
echo. 
echo "TESTING BINARY LONG(3) to ASCII LONG(5) "
echo.
call bin2asc prec_l3.bfm prec_l5.afm testprec.bin bin3asc5.sav
call chktrash
echo.
echo "DONE TESTING BINARY LONG(3) to ASCII LONG(5) "
echo.
cls
echo.
echo.
echo.
echo "***** PRECISION TEST WORKED LIKE A CHARM *****"
echo.
echo.
echo.
