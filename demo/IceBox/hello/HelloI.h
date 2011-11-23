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

#ifndef HELLO_I_H
#define HELLO_I_H

#include <Hello.h>

#if defined(_WIN32)
#   define HELLO_API __declspec(dllexport)
#else
#   define HELLO_API /**/
#endif

class HELLO_API HelloI : public Hello
{
public:

    HelloI(const Ice::CommunicatorPtr&);

    virtual void sayHello(const Ice::Current&);
    virtual void shutdown(const Ice::Current&);

private:

    Ice::CommunicatorPtr _communicator;
};

#endif
