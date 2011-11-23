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

#include <Ice/EndpointFactory.h>

using namespace std;
using namespace Ice;
using namespace IceInternal;

void IceInternal::incRef(EndpointFactory* p) { p->__incRef(); }
void IceInternal::decRef(EndpointFactory* p) { p->__decRef(); }

IceInternal::EndpointFactory::EndpointFactory()
{
}

IceInternal::EndpointFactory::~EndpointFactory()
{
}
