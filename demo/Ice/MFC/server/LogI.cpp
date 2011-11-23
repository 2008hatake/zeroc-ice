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

#include "stdafx.h"
#include "LogI.h"

using namespace std;

LogI::LogI() :
    _log(0)
{
}

void
LogI::trace(const string& category, const string& msg)
{
    string s = "[ " + category + ": " + msg + " ]";

    string::size_type idx = 0;
    while((idx = s.find("\n", idx)) != string::npos)
    {
        s.replace(idx, 1, "\r\n  ");
        idx += 3;
    }

    message(s);
}

void
LogI::warning(const string& msg)
{
    message("warning: " + msg);
}

void
LogI::error(const string& msg)
{
    message("error: " + msg);
}

void
LogI::message(const string& msg)
{
    string line = msg + "\r\n";
    if(_log)
    {
        _log->SetSel(-1, -1);
        _log->ReplaceSel(CString(line.c_str()));
    }
    else
    {
        _buffer.append(line);
    }
}

void
LogI::setControl(CEdit* log)
{
    _log = log;
    if(!_buffer.empty())
    {
        _log->ReplaceSel(CString(_buffer.c_str()));
        _buffer.clear();
    }
}
