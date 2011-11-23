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

#ifndef ICE_STRING_UTIL_H
#define ICE_STRING_UTIL_H

namespace IceInternal
{

//  
// Encodes a string into UTF8, escaping all characters outside the range [32-126]
// as well as any special characters determined by the caller.
//
std::string encodeString(const std::string&, const std::string&);

//
// Decodes a UTF8 string. Decoding starts at the given start position
// (inclusive) and stops at the given end position (exclusive). If the end
// position is zero, then the remainder of the string is used. Upon success,
// the result parameter holds the decoded string and true is returned.
// A return value of false indicates an error was detected in the encoding.
//
bool decodeString(const std::string&, std::string::size_type, std::string::size_type, std::string&);

//
// If a single or double quotation mark is found at the start position,
// then the position of the matching closing quote is returned. If no
// quotation mark is found at the start position, then 0 is returned.
// If no matching closing quote is found, then std::string::npos is
// returned.
//
std::string::size_type checkQuote(const std::string&, std::string::size_type = 0);

}

#endif
