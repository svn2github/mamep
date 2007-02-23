//============================================================
//
//  strconv.h - Win32 string conversion
//
//  Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team.
//  Visit http://mamedev.org for licensing and usage restrictions.
//
//============================================================

// standard windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// MAME headers
#include "restrack.h"
#include "mame.h"
#include "osd_so.h"

// MAMEOS headers
#include "strconv.h"
#include "unicode.h"


//============================================================
//  LOCAL VARIABLES
//============================================================

static int ansi_codepage = CP_OEMCP; //CP_ACP;


//============================================================
//  set_osdcore_acp
//============================================================

void set_osdcore_acp(int cp)
{
	// TODO: check specified cp is valid
	ansi_codepage = cp;
}


//============================================================
//  astring_from_utf8
//============================================================

int get_osdcore_acp(void)
{
	return ansi_codepage;
}


//============================================================
//  astring_from_utf8
//============================================================

CHAR *astring_from_utf8(const char *utf8string)
{
	UINT acp = get_osdcore_acp();
	WCHAR *wstring;
	int char_count;
	CHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
	wstring = (WCHAR *)alloca(char_count * sizeof(*wstring));
	MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, wstring, char_count);

	// convert UTF-16 to "ANSI code page" string
	char_count = WideCharToMultiByte(acp, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (CHAR *)malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(acp, 0, wstring, -1, result, char_count, NULL, NULL);

	return result;
}


//============================================================
//  utf8_from_astring
//============================================================

char *utf8_from_astring(const CHAR *astring)
{
	UINT acp = get_osdcore_acp();
	WCHAR *wstring;
	int char_count;
	CHAR *result;

	// convert "ANSI code page" string to UTF-16
	char_count = MultiByteToWideChar(acp, 0, astring, -1, NULL, 0);
	wstring = (WCHAR *)alloca(char_count * sizeof(*wstring));
	MultiByteToWideChar(acp, 0, astring, -1, wstring, char_count);

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (CHAR *)malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, NULL, NULL);

	return result;
}


//============================================================
//  wstring_from_utf8
//============================================================

WCHAR *wstring_from_utf8(const char *utf8string)
{
	int char_count;
	WCHAR *result;

	// convert MAME string (UTF-8) to UTF-16
	char_count = MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, NULL, 0);
	result = (WCHAR *)malloc(char_count * sizeof(*result));
	if (result != NULL)
		MultiByteToWideChar(CP_UTF8, 0, utf8string, -1, result, char_count);

	return result;
}


//============================================================
//  utf8_from_wstring
//============================================================

char *utf8_from_wstring(const WCHAR *wstring)
{
	int char_count;
	char *result;

	// convert UTF-16 to MAME string (UTF-8)
	char_count = WideCharToMultiByte(CP_UTF8, 0, wstring, -1, NULL, 0, NULL, NULL);
	result = (char *)malloc(char_count * sizeof(*result));
	if (result != NULL)
		WideCharToMultiByte(CP_UTF8, 0, wstring, -1, result, char_count, NULL, NULL);

	return result;
}
