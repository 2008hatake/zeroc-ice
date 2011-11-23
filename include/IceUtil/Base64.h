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

#ifndef ICE_UTIL_BASE_64_H
#define ICE_UTIL_BASE_64_H

#include <IceUtil/Config.h>
#include <string>
#include <vector>

namespace IceUtil
{

class ICE_UTIL_API Base64
{
public:

    static std::string encode(const std::vector<char>&);
    static std::vector<char> decode(const std::string&);


private:

    static char encode(unsigned char);
    static unsigned char decode(char);
    static bool isBase64(char);
};

}

#endif
