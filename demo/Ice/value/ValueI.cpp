// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <ValueI.h>

using namespace std;
using namespace Demo;

InitialI::InitialI(::Ice::ObjectAdapterPtr adapter)
{
    _simple = new Simple;
    _simple->message = "a message 4 u";

    _printer = new PrinterI;
    _printer->message = "Ice rulez!";
    _printerProxy = PrinterPrx::uncheckedCast(adapter->addWithUUID(_printer));

    _derivedPrinter = new DerivedPrinterI;
    _derivedPrinter->message = _printer->message;
    _derivedPrinter->derivedMessage = "a derived message 4 u";
    adapter->addWithUUID(_derivedPrinter);
}

SimplePtr
InitialI::getSimple(const Ice::Current&)
{
    return _simple;
}

::Ice::ObjectPtr
InitialI::getPrinterAsObject(const Ice::Current&)
{
    return _printer;
}

void
InitialI::getPrinter(PrinterPtr& impl, PrinterPrx& proxy, const Ice::Current&)
{
    impl = _printer;
    proxy = _printerProxy;
}

PrinterPtr
InitialI::getDerivedPrinter(const Ice::Current&)
{
    return _derivedPrinter;
}

void
InitialI::throwDerivedPrinter(const Ice::Current&)
{
    DerivedPrinterException ex;
    ex.derived = _derivedPrinter;
    throw ex;
}

void
PrinterI::printBackwards(const Ice::Current&)
{
    string s;
    s.resize(message.length());
    reverse_copy(message.begin(), message.end(), s.begin());
    cout << s << endl;
}

void
DerivedPrinterI::printUppercase(const Ice::Current&)
{
    string s;
    s.resize(derivedMessage.length());
    transform(derivedMessage.begin(), derivedMessage.end(), s.begin(), ::toupper);
    cout << s << endl;
}
