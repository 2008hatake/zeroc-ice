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

#include <IceBox/IceBox.h>

using namespace std;

void
IceBox::FailureException::ice_print(ostream& out) const
{
    Exception::ice_print(out);
    out << ":\nservice failure exception: " << reason;
}

