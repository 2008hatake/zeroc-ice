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

#ifndef ICE_ENDPOINT_FACTORY_H
#define ICE_ENDPOINT_FACTORY_H

#include <IceUtil/Shared.h>
#include <Ice/EndpointF.h>
#include <Ice/EndpointFactoryF.h>

namespace IceInternal
{

class BasicStream;

class ICE_PROTOCOL_API EndpointFactory : public ::IceUtil::Shared
{
public:

    virtual ~EndpointFactory();

    virtual ::Ice::Short type() const = 0;
    virtual ::std::string protocol() const = 0;
    virtual EndpointPtr create(const std::string&) const = 0;
    virtual EndpointPtr read(BasicStream*) const = 0;
    virtual void destroy() = 0;

protected:

    EndpointFactory();
};

}

#endif
