// **********************************************************************
//
// Copyright (c) 2003
// ZeroC, Inc.
// Billerica, MA, USA
//
// All Rights Reserved.
//
// Ice is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
// **********************************************************************

#ifndef ICE_UTIL_INPUT_UTIL_H
#define ICE_UTIL_INPUT_UTIL_H

#include <IceUtil/Config.h>
#include <string>
#include <limits.h>

namespace IceUtil
{

//
// strToInt64() is drop-in replacement for UNIX strtoll() 
//
ICE_UTIL_API Int64 strToInt64(const char*, char**, int);

//
// stringToInt64 converts a string into a signed 64-bit integer.
//
// bool stringToInt64(const std::string& stringToParse, Int64& result, std::string::size_type& pos);
//
// Semantics:
//
// - Ignore leading and trailing whitespace
//
// - If the string starts with '0', parse as octal
//
// - If the string starts with "0x" or "0X", parse as hexadecimal
//
// - Otherwise, parse as decimal
//
// - return value == true indicates a successful conversion and result contains the converted value
//
//	- if pos == string::npos, there are no trailing non-whitespace characters following the converted string
//
//	- if pos != string::npos, there are trailing non-whitespace characters following the converted string;
//	  pos indicates the first such character
//
// - return value == false indicates an unsuccessful conversion:
//
//	- result == 0 indicates that no digits were available for conversion
//
//	- result == INT64MIN or result == INT64MAX indicate underflow or overflow.
//
//		- if pos == string::npos, the string did not contain trailing non-whitespace characters
//		
//		- if pos != string::npos, the string contained trailing non-whitespace characters following the
//		  digits and pos indicates the first such character. (Note that all digits up to the first
//		  non-digit, non-whitespace character are consumed, regardless of how far into the digit string
//		  an overflow is detected.)
//
ICE_UTIL_API bool stringToInt64(const std::string&, Int64&, std::string::size_type&);

}

#endif
