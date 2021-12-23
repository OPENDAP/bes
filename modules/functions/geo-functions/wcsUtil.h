/******************************************************************************
 * $Id: wcsUtil.h 2011-07-19 16:24:00Z $
 *
 * Project:  The Open Geospatial Consortium (OGC) Web Coverage Service (WCS)
 * 			 for Earth Observation: Open Source Reference Implementation
 * Purpose:  WCS Utility Function definition
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

#ifndef WCSUTIL_H_
#define WCSUTIL_H_

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdlib.h>
#include <time.h>

#include <cpl_string.h>
#include "wcs_error.h"
#include "BoundingBox.h"

#ifndef EQUAL
#if defined(WIN32) || defined(WIN32CE)
#  define EQUALN(a,b,n)           (strnicmp(a,b,n)==0)
#  define EQUAL(a,b)              (stricmp(a,b)==0)
#else
#  define EQUALN(a,b,n)           (strncasecmp(a,b,n)==0)
#  define EQUAL(a,b)              (strcasecmp(a,b)==0)
#endif
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef WINDOWS
#define DELIMITER "\\"
#else
#define DELIMITER "/"
#endif

const int SHORT_NAME_LEN = 128;
const int MAX_NAME_LEN = 512;
const int MAX_LIST_LEN = 1024;
const int MAX_LINE_LEN = 65536;


// -----------------------------------------------------------------------
// CGI (Common Gateway Interface) related class and functions
// -----------------------------------------------------------------------
enum CGI_METHOD_FLAG
{
	UN_KNOWN, HTTP_GET, HTTP_XML_POST
};

/************************************************************************/
/* ==================================================================== */
/*                             WCSCGI                                   */
/* ==================================================================== */
/************************************************************************/

/**
 * \class WCSCGI "wcsUtil.h"
 *
 * WCSCGI class is used to acquire WCS request, both GET and POST method
 * are supported.
 */

class WCSCGI
{
private:
	string ms_CGIContent;
	CGI_METHOD_FLAG me_CGIMethod;

public:
	WCSCGI()
	{
		me_CGIMethod = UN_KNOWN;
	}

	CPLErr Run();

	string GetRqstContent()
	{
		return ms_CGIContent;
	}

	CGI_METHOD_FLAG GetCGImethod()
	{
		return me_CGIMethod;
	}
};

/************************************************************************/
/* ==================================================================== */
/*                             StringList                               */
/* ==================================================================== */
/************************************************************************/

class StringList
{
private:
	vector<string> strings;

public:
	StringList(const string& sstrings, const char delimiter);
	StringList(const string& sstrings, const string& delimiters);
	StringList(){ }

	int indexof(string qstr);

	void add(string newstr)
	{
		strings.push_back(newstr);
	}

	void clear()
	{
		strings.clear();
	}

	int size()
	{
		return strings.size();
	}

	string& operator [](int index)
	{
		return strings[index];
	}

	void append(StringList & s);
	void append(const string sstrings, const char delimiter);
	void append(const string sstrings, const string& delimiters);

	string toString()
	{
		string s = "";
		for (int i = 0; i < size(); i++)
			s += strings[i];
		return s;
	}

	string toString(const char delimiter)
	{
		string s = "";
		for (int i = 0; i < size(); i++)
			s += strings[i] + delimiter;
		return s;
	}

	string toString(const string& delimiters)
	{
		string s = "";
		for (int i = 0; i < size(); i++)
			s += strings[i] + delimiters;
		return s;
	}
};

class S2C
{

private:
	char buf[MAX_LINE_LEN];

public:
	S2C(string s)
	{
		s.copy(buf, string::npos);
		buf[s.size()] = 0;
	}

	char * c_str()
	{
		return buf;
	}

};

/************************************************************************/
/* ==================================================================== */
/*                             KVP                                      */
/* ==================================================================== */
/************************************************************************/

class KVP
{
public:
	string name, value;

	KVP& operator =(const KVP &id)
	{
		name = id.name;
		value = id.value;
		return *this;
	}

	KVP& operator =(const KVP *pid)
	{
		name = pid->name;
		value = pid->value;
		return *this;
	}

	KVP(string n, string v) :
		name(n), value(v)
	{
	}

	KVP(string namevaluepair);
};


/************************************************************************/
/* ==================================================================== */
/*                             KVPsReader                               */
/* ==================================================================== */
/************************************************************************/

class KVPsReader
{
protected:
	vector<KVP> m_kvps;

public:
	KVPsReader()
	{
	}

	~KVPsReader()
	{
	}

	KVPsReader(const string& urlStr, const char &tok);

	KVP& operator [](const int index)
	{
		return m_kvps[index];
	}

	int size()
	{
		return m_kvps.size();
	}

	string getValue(const string &keyname);
	string getValue(const string &keyname, const string &defaultvalue);
	vector<string> getValues(const string &keyname);
};

/************************************************************************/
/* ==================================================================== */
/*                             CFGReader                                */
/* ==================================================================== */
/************************************************************************/
class CFGReader
{
protected:
	vector<KVP> kvps;

public:
	CFGReader(const string &configfilename);

	KVP& operator [](const int index)
	{
		return kvps[index];
	}

	int size()
	{
		return kvps.size();
	}

	string getValue(const string &keyname);
	string getValue(const string &keyname, const string &defaultvalue);
};


// -----------------------------------------------------------------------
// Extra Template Functions
// 		Exchange() 				--- used to exchange values
// 		convertToString()		--- convert value to string
//		convertFromString		--- convert string to values
// -----------------------------------------------------------------------
template<class T>
static void Exchange(T & a, T & b)
{
	T t;
	t = a;
	a = b;
	b = t;
}

template <class T>
static string convertToString(T &value)
{
	stringstream ss;
	ss<<value;
	string rtnstr = ss.str();
	return rtnstr;
}

template <class T>
static void convertFromString(T &value, const string& s)
{
	stringstream ss(s);
	ss>>value;
}


// -----------------------------------------------------------------------
// Extra Utility Functions
// -----------------------------------------------------------------------
CPL_C_START

// String Operation Related
int CPL_DLL CPL_STDCALL 		CsvburstCpp(const std::string& line, std::vector<std::string>  &strSet, const char tok);
int CPL_DLL CPL_STDCALL 		CsvburstComplexCpp(const string& line, vector<string> &strSet, const char* tok);
int CPL_DLL CPL_STDCALL 		Find_Compare_SubStr(string line, string sub);
void CPL_DLL CPL_STDCALL 		Strslip(const char* str, const char* arrStr[], const char leftdlm, const char rightdlm);
string CPL_DLL CPL_STDCALL 		StrReplace(string& str, const string oldSubStr, const string newStr);
string CPL_DLL CPL_STDCALL 		SPrintArray(GDALDataType eDataType, const void *paDataArray, int nValues, const char *pszDelimiter);
string CPL_DLL CPL_STDCALL 		StrTrimHead(const string &str);
string CPL_DLL CPL_STDCALL 		StrTrimTail(const string &str);
string CPL_DLL CPL_STDCALL 		StrTrims(const std::string&, const char*);
string CPL_DLL CPL_STDCALL		StrTrim(const string &str);

// Date Time Operation Related
int CPL_DLL CPL_STDCALL 		CompareDateTime_GreaterThan(string time1, string time2);
int CPL_DLL CPL_STDCALL 		ConvertDateTimeToSeconds(string datetime);
string CPL_DLL CPL_STDCALL		GetTimeString(int code);

// Directory Operation Related
CPLErr CPL_DLL CPL_STDCALL 		GetFileNameList(char* dir, std::vector<string> &strList);
string CPL_DLL CPL_STDCALL		MakeTempFile(string dir, string covID, string suffix);
string CPL_DLL CPL_STDCALL 		GetUUID();

// Request Parser Operation
void CPL_DLL CPL_STDCALL		GetSubSetTime(const string& subsetstr, vector<string> &subsetvalue);
string CPL_DLL CPL_STDCALL		GetSubSetLatLon(const string& subsetstr, vector<double> &subsetvalue);
string CPL_DLL CPL_STDCALL 		GetSingleValue(const string& subsetstr);

CPLErr CPL_DLL CPL_STDCALL 		GetTRMMBandList(string start, string end, std::vector<int> &bandList);
void CPL_DLL CPL_STDCALL 		GetCornerPoints(const GDAL_GCP* &pGCPList, const int &nGCPs, My2DPoint& lowLeft, My2DPoint& upRight);

CPL_C_END

#endif /*WCSUTIL_H_*/
