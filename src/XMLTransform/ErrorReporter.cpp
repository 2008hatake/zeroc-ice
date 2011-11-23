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

#include <IceUtil/Config.h>
#include <XMLTransform/ErrorReporter.h>
#include <Ice/Xerces.h>

#include <iostream>
#include <sstream>

using namespace std;
using namespace XMLTransform;

DOMTreeErrorReporter::DOMTreeErrorReporter()
{
}

void
DOMTreeErrorReporter::warning(const ICE_XERCES_NS SAXParseException& ex)
{
    ostringstream out;
    out << "Warning at file \"" << toString(ex.getSystemId())
        << "\", line " << ex.getLineNumber()
        << ", column " << ex.getColumnNumber()
        << "\n   Message: " << toString(ex.getMessage()) << "\n";
    _errors.append(out.str());
}

void
DOMTreeErrorReporter::error(const ICE_XERCES_NS SAXParseException& ex)
{
    ostringstream out;
    out << "Error at file \"" << toString(ex.getSystemId())
        << "\", line " << ex.getLineNumber()
        << ", column " << ex.getColumnNumber()
        << "\n   Message: " << toString(ex.getMessage()) << "\n";
    _errors.append(out.str());
}

void
DOMTreeErrorReporter::fatalError(const ICE_XERCES_NS SAXParseException& ex)
{
    ostringstream out;
    out << "Fatal at file \"" << toString(ex.getSystemId())
        << "\", line " << ex.getLineNumber()
        << ", column " << ex.getColumnNumber()
        << "\n   Message: " << toString(ex.getMessage()) << "\n";
    _errors.append(out.str());
}

void
DOMTreeErrorReporter::resetErrors()
{
    _errors.clear();
}

bool
DOMTreeErrorReporter::getSawErrors() const
{
    return !_errors.empty();
}

string
DOMTreeErrorReporter::getErrors() const
{
    return _errors;
}

string
DOMTreeErrorReporter::toString(const XMLCh* s)
{
    if(s)
    {
	char* t = ICE_XERCES_NS XMLString::transcode(s);
	string r(t);
	delete[] t;
	return r;
    }
    else
    {
        return string();
    }
}
