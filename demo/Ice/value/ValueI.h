// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef VALUE_I_H
#define VALUE_I_H

#include <Value.h>

class InitialI : public ::Demo::Initial
{
public:

    InitialI(::Ice::ObjectAdapterPtr);

    virtual ::Demo::SimplePtr getSimple(const Ice::Current&);
    virtual ::Ice::ObjectPtr getPrinterAsObject(const Ice::Current&);
    virtual void getPrinter(::Demo::PrinterPtr&, ::Demo::PrinterPrx&, const Ice::Current&);
    virtual ::Demo::PrinterPtr getDerivedPrinter(const Ice::Current&);
    virtual void throwDerivedPrinter(const Ice::Current&);

private:

    ::Demo::SimplePtr _simple;
    ::Demo::PrinterPtr _printer;
    ::Demo::PrinterPrx _printerProxy;
    ::Demo::DerivedPrinterPtr _derivedPrinter;
};

class PrinterI : virtual public ::Demo::Printer
{
public:

    virtual void printBackwards(const Ice::Current&);
};

class DerivedPrinterI : virtual public ::Demo::DerivedPrinter, virtual public PrinterI
{
public:

    virtual void printUppercase(const Ice::Current&);
};

#endif
