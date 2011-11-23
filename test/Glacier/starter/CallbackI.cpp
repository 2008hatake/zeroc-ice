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

#include <Ice/Ice.h>
#include <CallbackI.h>

using namespace std;
using namespace Ice;

CallbackReceiverI::CallbackReceiverI() :
    _callback(false)
{
}

void
CallbackReceiverI::callback(const Current&)
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);
    assert(!_callback);
    _callback = true;
    notify();
}

void
CallbackReceiverI::callbackEx(const Current& current)
{
    callback(current);
    CallbackException ex;
    ex.someValue = 3.14;
    ex.someString = "3.14";
    throw ex;
}

bool
CallbackReceiverI::callbackOK()
{
    IceUtil::Monitor<IceUtil::Mutex>::Lock sync(*this);

    while(!_callback)
    {
	if(!timedWait(IceUtil::Time::milliSeconds(5000)))
	{
	    return false;
	}
    }

    _callback = false;
    return true;
}

CallbackI::CallbackI(const Ice::CommunicatorPtr& communicator) :
    _communicator(communicator)
{
}

void
CallbackI::initiateCallback(const CallbackReceiverPrx& proxy, const Current& current)
{
    proxy->callback(current.ctx);
}

void
CallbackI::initiateCallbackEx(const CallbackReceiverPrx& proxy, const Current& current)
{
    proxy->callbackEx(current.ctx);
}

void
CallbackI::shutdown(const Ice::Current&)
{
    _communicator->shutdown();
}
