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

#ifndef ICE_UTIL_UNICODE_H
#define ICE_UTIL_UNICODE_H

#include <IceUtil/Config.h>

namespace IceUtil
{

#if defined(_MSC_VER) && (_MSC_VER >= 1300)

//
// With Visual C++ 7.x, wchar_t is either a typedef unsigned short or a 
// native type (when /Zc:wchar_t is used).
// Since wstring is a typedef basic_string<wchar_t>, its type is also 
// different depending on whether /Zc:wchar_t is used or not.
//
// Ice is always built without /Zc:wchar_t, but provides wstringToString() 
// and stringToWstring() implementations for both flavors of wstring.
//

#   if defined(_NATIVE_WCHAR_T_DEFINED)
//
// We're using Ice with /Zc:wchar_t
// 
ICE_UTIL_API std::string wstringToString(const std::wstring&);
ICE_UTIL_API std::wstring stringToNativeWstring(const std::string&);

inline std::wstring 
stringToWstring(const std::string& str)
{
    return stringToNativeWstring(str);
}

#   else
//
// We're building Ice or using it without /Zc:wchar_t
//
ICE_UTIL_API std::string wstringToString(const std::wstring&);
ICE_UTIL_API std::wstring stringToWstring(const std::string&);

ICE_UTIL_API std::string wstringToString(const std::basic_string<__wchar_t>&);
ICE_UTIL_API std::basic_string<__wchar_t> stringToNativeWstring(const std::string&);

#   endif

#else

ICE_UTIL_API std::string wstringToString(const std::wstring&);
ICE_UTIL_API std::wstring stringToWstring(const std::string&);

#endif

}

#endif
