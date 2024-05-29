/******************************************************************************
 * $Id: wcsUtil.cpp 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  WCS Utility Function implementation
 * Author:   Yuanzheng Shao, yshao3@gmu.edu
 *
 ******************************************************************************
 * Copyright (c) 2011, Liping Di <ldi@gmu.edu>, Yuanzheng Shao <yshao3@gmu.edu>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdexcept>
#include <uuid/uuid.h>
#include "wcsUtil.h"
//#include "mfhdf.h"

/************************************************************************/
/*                          GetUUID()                                   */
/************************************************************************/

/**
 * \brief Used to get UUID string.
 *
 * This method will return a UUID, such as: cd32eb56-412c-11e0-9cce-67750f871b94
 *
 * @return A generated UUID string.
 */

string CPL_STDCALL GetUUID()
{
	uuid_t uuid;
	uuid_generate(uuid);
	char uuidstr[36];
	uuid_unparse(uuid, uuidstr);
	return uuidstr;
}

/************************************************************************/
/*                          SPrintArray()                               */
/************************************************************************/

/**
 * \brief Print a string based on coverage realted parameters.
 *
 * This method will return a UUID, such as: cd32eb56-412c-11e0-9cce-67750f871b94
 *
 * @return A generated UUID string.
 */

string CPL_STDCALL SPrintArray(GDALDataType eDataType, const void *paDataArray, int nValues, const char *pszDelimiter)
{
	char *pszString, *pszField;
	int i, iFieldSize, iStringSize;

	iFieldSize = 32 + strlen(pszDelimiter);
	pszField = (char *) CPLMalloc(iFieldSize + 1);
	iStringSize = nValues * iFieldSize + 1;
	pszString = (char *) CPLMalloc(iStringSize);
	memset(pszString, 0, iStringSize);
	for (i = 0; i < nValues; i++)
	{
		switch (eDataType)
		{
		case GDT_Byte:
			sprintf(pszField, "%d%s", ((GByte *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_UInt16:
			sprintf(pszField, "%u%s", ((GUInt16 *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_Int16:
		default:
			sprintf(pszField, "%d%s", ((GInt16 *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_UInt32:
			sprintf(pszField, "%u%s", ((GUInt32 *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_Int32:
			sprintf(pszField, "%d%s", ((GInt32 *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_Float32:
			sprintf(pszField, "%.7g%s", ((float *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		case GDT_Float64:
			sprintf(pszField, "%.15g%s", ((double *) paDataArray)[i], (i < nValues - 1) ? pszDelimiter : "");
			break;
		}
		strcat(pszString, pszField);
	}

	CPLFree(pszField);

	return string(pszString);
}

/************************************************************************/
/*                          StrTrimHead()                               */
/************************************************************************/

/**
 * \brief Trim s string's head.
 *
 * This method will trim a string's head, remove the sapce and line break
 * in the head of a string.
 *
 * @param str The string to be processed.
 *
 * @return A head-trimmed string.
 */

string CPL_STDCALL StrTrimHead(const string &str)
{
	string::size_type p = 0;
	/* leading space? */
	while ((p < str.size()) && (str[p] == ' ' || str[p] == '\t' || str[p] == 13 || str[p] == 10))
		p++;
	if (p >= str.size())
		return "";
	else
		return str.substr(p);
}

/************************************************************************/
/*                          StrTrimTail()                               */
/************************************************************************/

/**
 * \brief Trim s string's tail.
 *
 * This method will trim a string's tail, remove the sapce and line break
 * in the tail of a string.
 *
 * @param str The string to be processed.
 *
 * @return A tail-trimmed string.
 */

string CPL_STDCALL StrTrimTail(const string &str)
{
	string::size_type p = str.size() - 1;
	while (p >= 0 && (str[p] == ' ' || str[p] == '\t' || str[p] == 13 || str[p] == 10))
		p--;
	if (p < 0)
		return "";
	else
		return str.substr(0, p + 1);
}

/************************************************************************/
/*                          StrTrim()                                   */
/************************************************************************/

/**
 * \brief Trim s string's head and tail.
 *
 * This method will trim a string's head and tail, remove the sapce and
 * line break in both the head and end of a string.
 *
 * @param str The string to be processed.
 *
 * @return A trimmed string.
 */

string CPL_STDCALL StrTrim(const string &str)
{
	string s = StrTrimTail(str);
	return StrTrimHead(s);
}

/************************************************************************/
/*                          StrTrims()                                  */
/************************************************************************/

/**
 * \brief Trim s string's head and tail based on delimiter string.
 *
 * This method will trim a string's head and tail based on delimiter
 * string, and return the trimmed string.
 *
 * @param srcStr The string to be processed.
 *
 * @param tok The delimiter string used to trim the string.
 *
 * @return A trimmed string.
 */

string CPL_STDCALL StrTrims(const string& srcStr, const char* tok)
{
	if (srcStr.empty())
		return srcStr;

	string::size_type beginIdx, endIdx;
	beginIdx = srcStr.find_first_not_of(tok);
	endIdx = srcStr.find_last_not_of(tok);

	if (beginIdx != string::npos && endIdx != string::npos)
		return srcStr.substr(beginIdx, endIdx - beginIdx + 1);
	else
		return "";
}

/************************************************************************/
/*                          Strslip()                                   */
/************************************************************************/

/**
 * \brief Slice a string and split it into an array.
 *
 * This method will slice a string based on left and right delimiter, and then
 * split the slipped part into an array.
 *
 * @param str The string to be processed, such as "longitude[15.5,23.5]"
 *
 * @param arrStr The two dimension array to place the sliced results.
 *
 * @param leftdlm The left delimiter used to slip the string, such as '['.
 *
 * @param rightdlm The right delimiter used to slip the string, such as ']'.
 */

void CPL_STDCALL Strslip(const char* str, const char* arrStr[], const char leftdlm, const char rightdlm)
{
	char* pdest1 = 0;
	char* pdest2 = 0;

	arrStr[0] = str;
	pdest1 = (char*)strchr(str, leftdlm);
	if (!pdest1)
	{
		arrStr[1] = str;
		return;
	}

	pdest2 = (char*)strrchr(str, rightdlm);
	if (!pdest2)
		return;

	arrStr[1] = pdest1 + 1;
	*pdest1 = '\0';
	*pdest2 = '\0';

	return;
}

/************************************************************************/
/*                          CsvburstComplexCpp()                        */
/************************************************************************/

/**
 * \brief Slice a string based on single or multiple delimiter(s), and
 * then store the results to array.
 *
 * This method will slice a string based on specified delimiter(s), and then
 * place the sliced string parts to a array.
 *
 * @param line The string to be processed.
 *
 * @param strSet The array to place the sliced results.
 *
 * @param tok The delimiter string used to slice the string.
 *
 * @return The size of the sliced parts.
 */

int CPL_STDCALL CsvburstComplexCpp(const string& line, vector<string> &strSet, const char *tok)
{
	if (line.empty() || line == "")
		return 0;

	strSet.clear();
	string::size_type firstPos, idx;

	firstPos = idx = 0;
	idx = line.find_first_of(tok, firstPos);

	while (idx != string::npos)
	{
		string tmpStr = StrTrim(line.substr(firstPos, (idx - firstPos)));
		if (!tmpStr.empty() && tmpStr != "")
			strSet.push_back(tmpStr);

		firstPos = idx + 1;
		if (firstPos == string::npos)
			break;
		idx = line.find_first_of(tok, firstPos);
	}

	if (firstPos != string::npos)
	{
		strSet.push_back(StrTrim(line.substr(firstPos)));
	}

	return strSet.size();
}

/************************************************************************/
/*                          CsvburstCpp()                               */
/************************************************************************/

/**
 * \brief Slice a string based on single delimiter, and then store the results to array.
 *
 * This method will slice a string based on specified single delimiter, and then
 * place the sliced string parts to a array.
 *
 * @param line The string to be processed, such as "12,34,56".
 *
 * @param strSet The array to place the sliced results.
 *
 * @param tok The delimiter character used to slice the string, such as ','.
 *
 * @return The size of the sliced parts.
 */

int CPL_STDCALL CsvburstCpp(const string& line, vector<string> &strSet, const char tok)
{
	if (line.empty() || line == "")
		return 0;

	strSet.clear();
	string::size_type firstPos, idx;
	string::size_type panfuPos;
	panfuPos = 0;
	firstPos = idx = 0;

	idx = line.find_first_of(tok, firstPos);

	while (idx != string::npos)
	{
		if(line[idx-2] == '\"' || line[idx-2] == '\'')//Add By Yuanzheng Shao
		{
			firstPos = idx + 1;
			if (firstPos == string::npos)
				break;
			panfuPos = idx-2;
			idx = line.find_first_of(tok, firstPos);

			string tmpStr = StrTrim(line.substr(panfuPos, (idx - panfuPos)));
			if (!tmpStr.empty() && tmpStr != "")
				strSet.push_back(tmpStr);

			firstPos = idx + 1;
			idx = line.find_first_of(tok, firstPos);
		}
		else
		{
			string tmpStr = StrTrim(line.substr(firstPos, (idx - firstPos)));
			if (!tmpStr.empty() && tmpStr != "")
				strSet.push_back(tmpStr);

			firstPos = idx + 1;
			if (firstPos == string::npos)
				break;
			idx = line.find_first_of(tok, firstPos);
		}
	}

	if (firstPos != string::npos)
	{
		strSet.push_back(StrTrim(line.substr(firstPos)));
	}

	return strSet.size();
}

/************************************************************************/
/*                          Find_Compare_SubStr()                       */
/************************************************************************/

/**
 * \brief Find the substring in a string.
 *
 * This method will find a substring in a string.
 *
 * @param line The string to be processed.
 *
 * @param sub The substring to be compared.
 *
 * @return TRUE if find the substring, otherwise FALSE.
 */

int CPL_STDCALL Find_Compare_SubStr(string line, string sub)
{
	if (line.empty() || line == "" || sub.empty() || sub == "")
		return 0;

	for (unsigned int i = 0; i < line.size(); ++i)
	{
		line[i] = (char) toupper((char) line[i]);
	}
	for (unsigned int j = 0; j < sub.size(); ++j)
	{
		sub[j] = (char) toupper((char) sub[j]);
	}

	if (string::npos == line.find(sub))
		return 0;
	else
		return 1;
}

/************************************************************************/
/*                          GetFileNameList()                           */
/************************************************************************/

/**
 * \brief Fetch the file name list under the specified directory.
 *
 * This method will fetch the file name list under the specified directory,
 * and store the results to a array.
 *
 * @param dir The directory name.
 *
 * @param strList The array used to place the results.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr CPL_STDCALL GetFileNameList(char* dir, vector<string> &strList)
{
	char pwdBuf[256];
	char *pwd=getcwd (pwdBuf, 256);
	DIR *dp;
	struct dirent *entry;
	struct stat statbuf;
	if ((dp = opendir(dir)) == NULL)
	{
		SetWCS_ErrorLocator("getFileNameList()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Open Director %s", dir);
		return CE_Failure;
	}

	if (dir[strlen(dir) - 1] == '/')
		dir[strlen(dir) - 1] = '\0';

	chdir(dir);

	while ((entry = readdir(dp)) != NULL)
	{
		lstat(entry->d_name, &statbuf);
		if (S_ISDIR(statbuf.st_mode))
		{
			/* Found a directory, but ignore . and .. */
			if (strcmp(".", entry->d_name) == 0 || strcmp("..", entry->d_name) == 0)
				continue;
			/* Recurse at a new indent level */
			string subdir = dir;
			subdir = subdir + "/" + entry->d_name;
			GetFileNameList((char*)subdir.c_str(), strList);
		}
		else if (S_ISREG(statbuf.st_mode))
		{
			string fname = getcwd(NULL, 0);
			fname = fname + "/" + entry->d_name;
			strList.push_back(fname);
		}
	}
	chdir(pwd);
	closedir(dp);

	return CE_None;
}

/************************************************************************/
/*                             Julian2Date()                            */
/************************************************************************/

/**
 * \brief Convert the Julian days to date string.
 *
 * This method will convert the days of year to its date string (YYYY-MM-DD);
 *
 * @param year The year of days.
 *
 * @param days The Julian days.
 *
 * @return The date string.
 */

string Julian2Date(int year, int days)
{
	bool leapyear = ((year%4==0 && year%100!=0) || year%400==0) ? true : false;
	int month, day;

	int leapArr[24] = {1, 31, 32, 60, 61, 91, 92, 121, 122, 152, 153, 182, 183, 213, 214, 244,
		245, 274, 275, 305, 306, 335, 336, 366};
	int unLeArr[24] = {1, 31, 32, 59, 60, 90, 91, 120, 121, 151, 152, 181, 182, 212, 213, 243,
		244, 273, 274, 304, 305, 334, 335, 365};

	if(leapyear) {
		for (int i = 0; i < 24; i += 2) {
			if(days >= leapArr[i] && days <= leapArr[i+1]) {
				month = int(i/2) + 1;
				day = days - leapArr[i] + 1;
				break;
			}
		}
	} else {
		for (int i=0; i < 24; i += 2) {
			if(days >= unLeArr[i] && days <= unLeArr[i+1]) {
				month = int(i/2) + 1;
				day = days - unLeArr[i] + 1;
				break;
			}
		}
	}

	string monthStr = month<10 ? "0" + convertToString(month) : convertToString(month);
	string dayStr = day<10 ? "0" + convertToString(day) : convertToString(day);

	return convertToString(year) + "-" + monthStr + "-" + dayStr;
}

/************************************************************************/
/*                          GetTRMMBandList()                           */
/************************************************************************/

/**
 * \brief Fetch the day's list for TRMM data based on the range of date/time.
 *
 * This method will find day's list for TRMM data based on specified date/time
 * range, and store the results to a array.
 *
 * @param start The start date/time.
 *
 * @param end The end date/time.
 *
 * @param bandList The array used to place the results.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr CPL_STDCALL GetTRMMBandList(string start, string end, std::vector<int> &bandList)
{
	//calculate 2000-06-01's days, start's days, end's days
	if(EQUAL(start.c_str(), "") && EQUAL(end.c_str(), ""))
		return CE_None;

	string june1str = start.substr(0, 4) + "-06-01";
	long int june1sec = ConvertDateTimeToSeconds(june1str);
	long int startsec = 0, endsec = 0;

	if(!EQUAL(start.c_str(), ""))
		startsec = ConvertDateTimeToSeconds(start);
	if(!EQUAL(end.c_str(), ""))
		endsec = ConvertDateTimeToSeconds(end);

	int sdays = (int)(startsec - june1sec)/(24*3600) + 1;
	int edays = (int)(endsec - june1sec)/(24*3600) + 1;

	sdays = (sdays < 0) ? 0 : sdays;
	edays = (edays < 0) ? sdays : edays;

	for(int i = sdays; i <= edays; i++)
		bandList.push_back(i);

	return CE_None;
}


/************************************************************************/
/*                          GetTimeString()                             */
/************************************************************************/

/**
 * \brief Fetch the current system time to a string.
 *
 * This method will get the current system time, and convert it to a string.
 * The return date/time has two formats:
 * 		1: YYYYMMDDHHMMSS
 * 		2: YYYY-MM-DDTHH:MM:SSZ
 *
 * @param code The return string's format.
 *
 * @return The string corresponding to current system time.
 */

string GetTimeString(int code)
{
	struct tm* ptime;
	time_t now;
	time(&now);
	ptime = localtime(&now);
	int year = ptime->tm_year + 1900;
	int month = ptime->tm_mon + 1;
	int day = ptime->tm_mday;
	int hour = ptime->tm_hour;
	int minute = ptime->tm_min;
	int second = ptime->tm_sec;
	string ye, mo, da, ho, mi, se;
	ye = convertToString(year);
	mo = (month < 10) ? "0" + convertToString(month) : convertToString(month);
	da = (day < 10)	? "0" + convertToString(day) : convertToString(day);
	ho = (hour < 10)	? "0" + convertToString(hour) : convertToString(hour);
	mi = (minute < 10)	? "0" + convertToString(minute) : convertToString(minute);
	se = (second < 10)	? "0" + convertToString(second) : convertToString(second);

	string timestring;
	if(code == 1)
	{
		string part1 = ye + mo + da;
		string part2 = ho + mi + se;
		timestring = part1.append(part2);
	}
	else if(code == 2)
	{
		string part1 = ye + "-" + mo + "-" + da + "T";
		string part2 = ho + ":" + mi + ":" + se + "Z";
		timestring = part1.append(part2);
	}

	return timestring;
}

/************************************************************************/
/*                          MakeTempFile()                              */
/************************************************************************/

/**
 * \brief Generate a temporary file path.
 *
 * This method will generate a path for temporary file or output file,
 * which is based on coverage identifier.
 *
 * @param dir The directory of the temporary file.
 *
 * @param covID The coverage identifier.
 *
 * @param suffix The suffix of the file format.
 *
 * @return The full path of the temporary file.
 */

string MakeTempFile(string dir, string covID, string suffix)
{
	string sOutFileName;

	if (!dir.empty() && dir != "")
	{
		if (dir[dir.size() - 1] != '/')
		{
			dir += "/";
		}
	}
	if (!suffix.empty() && suffix != "")
	{
		if (suffix[0] != '.')
			suffix = "." + suffix;
	}

	if(covID == "")
	{
		sOutFileName = dir + GetUUID() + suffix;
	}
	else if (EQUALN(covID.c_str(),"HDF4_EOS:EOS_SWATH:",19) ||
		EQUALN(covID.c_str(),"HDF4_EOS:EOS_GRID:",18) ||
		EQUALN(covID.c_str(),"HDF5:",5))
	{
		vector<string> strSet;
		int n = CsvburstCpp(covID, strSet, ':');
		if(n == 5){//Terra&Aqua
			string srcFilePath = StrTrims(strSet[2], "\'\"");
			string srcFileName = string(CPLGetBasename(srcFilePath.c_str()));
			string datasetname = StrTrims(strSet[4], "\'\"");
			datasetname = StrReplace(datasetname, " ", "_");
			sOutFileName = dir + srcFileName + "." + datasetname + "." + GetTimeString(1) + suffix;
		}else if(n == 3) {//Aura
			string srcFilePath = StrTrims(strSet[1], "\'\"");
			string srcFileName = string(CPLGetBasename(srcFilePath.c_str()));
			string datasetname = StrTrims(strSet[2], "\'\"");
			datasetname = StrReplace(datasetname, "//", "");
			datasetname = StrReplace(datasetname, "/", "_");
			datasetname = StrReplace(datasetname, " ", "");
			sOutFileName = dir + srcFileName + "." + datasetname + "." + GetTimeString(1) + suffix;
		}else
			sOutFileName = dir + GetUUID() + suffix;
	}else if(EQUALN( covID.c_str(), "GOES:NetCDF:",12))
	{
		vector<string> strSet;
		int n = CsvburstCpp(covID, strSet, ':');
		if(n == 4){
			string srcFilePath = StrTrims(strSet[2], " \'\"");
			string srcFileName = string(CPLGetBasename(srcFilePath.c_str()));
			sOutFileName = dir + srcFileName + "." + GetTimeString(1) + suffix;
		}else
			sOutFileName = dir + GetUUID() + suffix;
	}
	else if(EQUALN(covID.c_str(),"GEOTIFF:",8))
	{
		vector<string> strSet;
		int n = CsvburstCpp(covID, strSet, ':');
		if(n == 3){
			string srcFilePath = StrTrims(strSet[1], " \'\"");
			string srcFileName = string(CPLGetBasename(srcFilePath.c_str()));
			sOutFileName = dir + srcFileName + "." + GetTimeString(1) + suffix;
		}else
			sOutFileName = dir + GetUUID() + suffix;
	}
	else if(EQUALN(covID.c_str(),"TRMM:",5))
	{
		vector<string> strSet;
		int n = CsvburstCpp(covID, strSet, ':');
		if(n == 3){
			string srcFilePath = StrTrims(strSet[1], " \'\"");
			string srcFileName = string(CPLGetBasename(srcFilePath.c_str()));
			sOutFileName = dir + srcFileName + "." + GetTimeString(1) + suffix;
		}else
			sOutFileName = dir + GetUUID() + suffix;
	}
	else
		sOutFileName = dir + GetUUID() + suffix;

	return sOutFileName;
}

/************************************************************************/
/*                          StringList()                                */
/************************************************************************/

/**
 * \brief Constructor for StringList.
 */

StringList::StringList(const string& sstrings, const char delimiter)
{
	string str = sstrings + delimiter;
	string::size_type np, op = 0;
	while ((op < str.size())
			&& ((np = str.find(delimiter, op)) != string::npos))
	{
		add(str.substr(op, np - op));
		op = ++np;
	}
}

// -----------------------------------------------------------------------
// string list constructor
// -----------------------------------------------------------------------
StringList::StringList(const string& sstrings, const string& delimiters)
{
	string str = sstrings + delimiters;
	string::size_type np, op = 0;
	while ((op < str.size()) && ((np = str.find(delimiters, op))
			!= string::npos))
	{
		add(str.substr(op, np - op));
		op = np + delimiters.size();
	}
}

// -----------------------------------------------------------------------
// string list -- append
// -----------------------------------------------------------------------
void StringList::append(StringList & s)
{
	for (int i = 0; i < s.size(); i++)
		add(s[i]);
}

// -----------------------------------------------------------------------
// string list -- append with delimiter
// -----------------------------------------------------------------------
void StringList::append(const string sstrings, const char delimiter)
{
	StringList n(sstrings, delimiter);
	append(n);
}

// -----------------------------------------------------------------------
// string list -- append with delimiters
// -----------------------------------------------------------------------
void StringList::append(const string sstrings, const string& delimiters)
{
	StringList n(sstrings, delimiters);
	append(n);
}

// -----------------------------------------------------------------------
// string list -- return the index for query string
// -----------------------------------------------------------------------
int StringList::indexof(string qstr)
{
	for (int i = 0; i < size(); i++)
	{
		if (strings[i] == qstr)
			return i;
	}
	return -1;
}

/************************************************************************/
/*                          KVP()                                       */
/************************************************************************/

/**
 * \brief Constructor for KVP.
 *
 * @param namevaluepair The key-value pair.
 */

KVP::KVP(string namevaluepair)
{
	StringList ss(namevaluepair, '=');
	if (ss.size() == 2)
	{
		name = StrTrim(ss[0]);
		value = StrTrim(ss[1]);
	}
	else
	{
		name = namevaluepair;
		value = "";
	}
}

/************************************************************************/
/*                          CFGReader()                                 */
/************************************************************************/

/**
 * \brief Constructor for CFGReader.
 *
 * This method is used to load the configuration file for WCS. Each configuration
 * item looks like a key-value pair, CFGReader() will read the file to memory and
 * store the parameters to a KVP array.
 *
 * @param configfilename The full path of the configuration file.
 */

CFGReader::CFGReader(const string &configfilename)
{
	ifstream cfgfile(configfilename.c_str());
	if (!cfgfile)
	{
		SetWCS_ErrorLocator("CFGReader");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to open configure file.");
		throw runtime_error("");
	}

	char line[MAX_LINE_LEN];
	while (!cfgfile.eof())
	{
		cfgfile.getline(line, MAX_LINE_LEN - 1, '\n');
		if (!cfgfile.fail())
		{
			if (line[0] != '#')
			{
				KVP kvp(line);
				kvps.push_back(kvp);
			}
		}
	}

	cfgfile.close();
}

/************************************************************************/
/*                          getValue()                                  */
/************************************************************************/

/**
 * \brief Fetch a configured item's value.
 *
 * This method is used to fetch a items value from a configuration file.
 *
 * @param keyname The item's name.
 *
 * @return The result string.
 */

string CFGReader::getValue(const string &keyname)
{
	for (int i = 0; i < size(); i++)
	{
		if (EQUAL(kvps[i].name.c_str(), keyname.c_str()))
			return kvps[i].value;
	}

	return "";
}

/************************************************************************/
/*                          getValue()                                  */
/************************************************************************/

/**
 * \brief Fetch a configured item's value, with default value.
 *
 * This method is used to fetch a items value from a configuration file,
 * if not found, use the default value.
 *
 * @param keyname The item's name.
 *
 * @param defaultvalue The item's default value.
 *
 * @return The result string.
 */

string CFGReader::getValue(const string &keyname, const string &defaultvalue)
{
	string ret = getValue(keyname);
	if (ret.empty() || ret == "")
		return defaultvalue;
	else
		return ret;
}

/************************************************************************/
/*                          Run()                                       */
/************************************************************************/

/**
 * \brief The entry point for CGI program.
 *
 * This method is used to fetch CGI parameters. Supports both HTTP GET
 * and POST method. POST content could be KVPs and XML string, and GET
 * content must be KVPs.
 *
 * @return CE_None on success or CE_Failure on failure.
 */

CPLErr WCSCGI::Run()
{
	/* get request parameters*/
	char *gm = getenv("REQUEST_METHOD");
	if (NULL == gm)
	{
		SetWCS_ErrorLocator("WCSCGI::Run()");
		WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Get \"REQUEST_METHOD\" Environment variable.");
		return CE_Failure;
	}

	if (EQUAL(gm, "POST"))
		me_CGIMethod = HTTP_XML_POST;
	else if (EQUAL(gm, "GET"))
		me_CGIMethod = HTTP_GET;
	else
		me_CGIMethod = UN_KNOWN;

	switch (me_CGIMethod)
	{
		case HTTP_XML_POST:
		{
			char* aString = getenv("CONTENT_LENGTH");
			if (NULL == aString || aString[0] == '\0')
			{
				WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Get \"CONTENT_LENGTH\" Environment variable.");
				SetWCS_ErrorLocator("WCSCGI::Run()");
				return CE_Failure;
			}

			unsigned int cLength = atoi(aString);

			string cString;
			cString.resize(cLength + 1);

			if (cLength != fread((char*) cString.c_str(), 1, cLength, stdin))
			{
				SetWCS_ErrorLocator("WCSCGI::Run()");
				WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Failed to Read POST Data Stream from Internet.");
				return CE_Failure;
			}

			cString = StrReplace(cString, "&amps;", "%26");
			cString = StrReplace(cString, "&amp;", "%26");

			string tmpStr = CPLUnescapeString(cString.c_str(), NULL, CPLES_URL);
			ms_CGIContent = CPLUnescapeString((char*) tmpStr.c_str(), NULL, CPLES_XML);

			string::size_type beginIdx, endIdx;

			beginIdx = ms_CGIContent.find("<?");
			endIdx = ms_CGIContent.rfind(">");
			//The post contents could be KVPs
			if (beginIdx != string::npos && endIdx != string::npos)
				ms_CGIContent = ms_CGIContent.substr(beginIdx, endIdx - beginIdx + 1);
		}
			break;
		case HTTP_GET:
		{
			string tmpStr = getenv("QUERY_STRING");
			tmpStr = StrReplace(tmpStr, "&amps;", "%26");
			tmpStr = StrReplace(tmpStr, "&amp;", "%26");
			if (tmpStr.empty() || tmpStr == "")
			{
				SetWCS_ErrorLocator("WCSCGI::Run()");
				WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "No \"QUERY_STRING\" Content.");
				return CE_Failure;
			}

			ms_CGIContent = CPLUnescapeString((char*) tmpStr.c_str(), NULL, CPLES_URL);
		}
			break;
		default:
		{
			SetWCS_ErrorLocator("WCSCGI::Run()");
			WCS_Error(CE_Failure, OGC_WCS_NoApplicableCode, "Unkown \"REQUEST_METHOD\".");
			return CE_Failure;
		}
	}

	return CE_None;
}

// -----------------------------------------------------------------------
// KVPsReader constructor
// -----------------------------------------------------------------------
KVPsReader::KVPsReader(const string& urlStr, const char &tok)
{
	StringList strLit(urlStr, tok);

	for (int i = 0; i < strLit.size(); ++i)
	{
		KVP kvp(StrTrim(strLit[i]));
		m_kvps.push_back(kvp);
	}
}

// -----------------------------------------------------------------------
// KVPsReader get value function
// -----------------------------------------------------------------------
string KVPsReader::getValue(const string &keyname)
{
	for (unsigned int i = 0; i < m_kvps.size(); i++)
	{
		if (EQUAL(m_kvps[i].name.c_str() , keyname.c_str()))
			return m_kvps[i].value;
	}
	return "";
}


vector<string> KVPsReader::getValues(const string &keyname)
{
	vector<string> valuesList;
	for (unsigned int i = 0; i < m_kvps.size(); i++)
	{
		if (EQUAL(m_kvps[i].name.c_str() , keyname.c_str()))
			valuesList.push_back(m_kvps[i].value);
	}
	return valuesList;
}

// -----------------------------------------------------------------------
// KVPsReader get value function, with default value
// -----------------------------------------------------------------------
string KVPsReader::getValue(const string &keyname, const string &defaultvalue)
{
	string ret = getValue(keyname);
	if (ret.empty() || ret == "")
		return defaultvalue;
	else
		return ret;
}

/************************************************************************/
/*                          StrReplace()                                */
/************************************************************************/

/**
 * \brief Replace a string.
 *
 * This method is used to replace a string with specified substring.
 *
 * @param str The string needs to be processed.
 *
 * @param oldSubStr The substring needs to be replaced.
 *
 * @param newStr The string will replace the oldSubStr.
 *
 * @return The processed string.
 */

string CPL_STDCALL StrReplace(string& str, const string oldSubStr, const string newStr)
{
	while(true)
	{
		string::size_type pos(0);
		if( ( pos = str.find(oldSubStr) ) != string::npos )
		{
			str.replace( pos, oldSubStr.size(), newStr );
		}else {
			break;
		}
	}
	return str;
}

/************************************************************************/
/*                          GetSingleValue()                            */
/************************************************************************/

/**
 * \brief Get a single value from a special KVP part.
 *
 * This method is used to get value from a special KVP part.
 *
 * @param subsetstr The string needs to be processed, such as "subset=Long(11)".
 *
 * @return The processed string, such as "11".
 */

string CPL_STDCALL GetSingleValue(const string& subsetstr)
{
	string::size_type idx1 = subsetstr.find_last_of('(');
	string::size_type idx2 = subsetstr.find_last_of(')');

	return StrTrim(subsetstr.substr(idx1 + 1, idx2 - idx1 -1));
}

/************************************************************************/
/*                          GetSubSetLatLon()                           */
/************************************************************************/

/**
 * \brief Get a subset spatial extent in WCS 2.0 request.
 *
 * This method is used to get values of spatial extent and place the values
 * in an array, and return the CRS code for the coordinates of the extent.
 *
 * @param subsetstr The string needs to be processed,
 * such as "subset=Lat,http://www.opengis.net/def/crs/EPSG/0/4326(32,47)".
 *
 * @param subsetvalue The array to place the spatial extent values.
 *
 * @return The processed string, such as "EPSG:4326".
 */

string CPL_STDCALL GetSubSetLatLon(const string& subsetstr, vector<double> &subsetvalue)
{
	string::size_type idx1 = subsetstr.find_last_of('(');
	string::size_type idx2 = subsetstr.find_last_of(')');

	string value = StrTrim(subsetstr.substr(idx1 + 1, idx2 - idx1 -1));
	vector<string> tmpV;
	CsvburstCpp(value, tmpV, ',');

	for(unsigned int i = 0; i < tmpV.size(); i++)
	{
		double curD;
		convertFromString(curD, tmpV.at(i));
		subsetvalue.push_back(curD);
	}

	string subsetProj;
	idx1 = subsetstr.find(',');
	idx2 = subsetstr.find('(');
	if(idx1 > idx2) 
	{
		subsetProj = "EPSG:4326";
	}
	else
	{
        	value = StrTrim(subsetstr.substr(idx1 + 1, idx2 - idx1 - 1));
        	if(value.find("http") != string::npos)
        	{
                	string::size_type idx3 = value.find_last_of('/');
                	subsetProj = "EPSG:" + value.substr(idx3 + 1);
        	}
        	else
        	{
        	        subsetProj = value;
	        }

	}

	return subsetProj;
}

//subset=phenomenonTime("2006-08-01","2006-08-22T09:22:00Z")&
//subset=phenomenonTime("2006-08-01")&
void CPL_STDCALL GetSubSetTime(const string& subsetstr, vector<string> &subsetvalue)
{
	string::size_type idx1 = subsetstr.find_last_of('(');
	string::size_type idx2 = subsetstr.find_last_of(')');
	string value = StrTrim(subsetstr.substr(idx1 + 1, idx2 - idx1 - 1));
	string valueN = StrReplace(value, "\"", "");
	CsvburstCpp(valueN, subsetvalue, ',');
}

//time type 1: 2006-08-22T09:22:00Z
//time type 1: 2006-08-01
int CPL_STDCALL CompareDateTime_GreaterThan(string time1, string time2)
{
	time_t retval1 = 0;
	time_t retval2 = 0;

	struct tm storage1 = {0,0,0,0,0,0,0,0,0};
	struct tm storage2 = {0,0,0,0,0,0,0,0,0};

	char *p1 = NULL;
	char *p2 = NULL;

	if(time1.find("T") != string::npos && time1.find("Z") != string::npos)
		p1 =	(char *)strptime(time1.c_str(), "%Y-%m-%dT%H:%M:%SZ", &storage1);
	else
		p1 = (char *)strptime(time1.c_str(), "%Y-%m-%d", &storage1);

	if(time2.find("T") != string::npos && time2.find("Z") != string::npos)
		p2 =	(char *)strptime(time2.c_str(), "%Y-%m-%dT%H:%M:%SZ", &storage2);
	else
		p2 = (char *)strptime(time2.c_str(), "%Y-%m-%d", &storage2);

	retval1 = mktime(&storage1);
	retval2 = mktime(&storage2);

	if(retval1 == retval2)
		return 0;
	else if(retval1 > retval2)
		return 1;
	else
		return -1;

}

int CPL_STDCALL	ConvertDateTimeToSeconds(string datetime)
{
	time_t retval1 = 0;
	struct tm storage1 = {0,0,0,0,0,0,0,0,0};
	char *p1 = NULL;

	if(datetime.find("T") != string::npos && datetime.find("Z") != string::npos)
		p1 =	(char *)strptime(datetime.c_str(), "%Y-%m-%dT%H:%M:%SZ", &storage1);
	else
		p1 = (char *)strptime(datetime.c_str(), "%Y-%m-%d", &storage1);

	retval1 = mktime(&storage1);

	return retval1;
}

// -----------------------------------------------------------------------
// Find lower-left and upper-right corner point coordinate
// -----------------------------------------------------------------------
void CPL_STDCALL GetCornerPoints(const GDAL_GCP* &pGCPList, const int &nGCPs, My2DPoint& lowLeft, My2DPoint& upRight)
{
        double xLeft = (numeric_limits<double>::max)();
        double xRight = -xLeft;
        double yLower = (numeric_limits<double>::max)();
        double yUpper = -yLower;

        for (int j = 0; j < nGCPs; j++)
        {
                yUpper = MAX(yUpper,pGCPList[j].dfGCPY);
                yLower = MIN(yLower,pGCPList[j].dfGCPY);
                if(pGCPList[j].dfGCPX != -999)//test MOD021KM.A2000065.1900.005.2008235220315.hdf, error GCP X value (-999)
                        xLeft = MIN(xLeft,pGCPList[j].dfGCPX);
                xRight = MAX(xRight,pGCPList[j].dfGCPX);
        }

        lowLeft.mi_X = xLeft;
        lowLeft.mi_Y = yLower;
        upRight.mi_X = xRight;
        upRight.mi_Y = yUpper;

        return;
}
