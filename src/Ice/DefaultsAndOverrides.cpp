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

#include <Ice/DefaultsAndOverrides.h>
#include <Ice/Properties.h>
#include <Ice/Network.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(DefaultsAndOverrides* p) { p->__incRef(); }
void IceInternal::decRef(DefaultsAndOverrides* p) { p->__decRef(); }

IceInternal::DefaultsAndOverrides::DefaultsAndOverrides(const PropertiesPtr& properties) :
    overrideTimeout(false),
    overrideTimeoutValue(-1),
    overrideCompress(false),
    overrideCompressValue(false)
{
    const_cast<string&>(defaultProtocol) = properties->getPropertyWithDefault("Ice.Default.Protocol", "tcp");

    const_cast<string&>(defaultHost) = properties->getProperty("Ice.Default.Host");
    if(defaultHost.empty())
    {
	const_cast<string&>(defaultHost) = getLocalHost(true);
    }

    const_cast<string&>(defaultRouter) = properties->getProperty("Ice.Default.Router");

    string value;
    
    value = properties->getProperty("Ice.Override.Timeout");
    if(!value.empty())
    {
	const_cast<bool&>(overrideTimeout) = true;
	const_cast<Int&>(overrideTimeoutValue) = properties->getPropertyAsInt("Ice.Override.Timeout");
    }

    value = properties->getProperty("Ice.Override.Compress");
    if(!value.empty())
    {
	const_cast<bool&>(overrideCompress) = true;
	const_cast<bool&>(overrideCompressValue) = properties->getPropertyAsInt("Ice.Override.Compress");
    }

    const_cast<string&>(defaultLocator) = properties->getProperty("Ice.Default.Locator");
}
