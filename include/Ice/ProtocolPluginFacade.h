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

#ifndef ICE_PROTOCOL_PLUGIN_FACADE_H
#define ICE_PROTOCOL_PLUGIN_FACADE_H

#include <IceUtil/Shared.h>
#include <Ice/ProtocolPluginFacadeF.h>
#include <Ice/CommunicatorF.h>
#include <Ice/EndpointFactoryF.h>
#include <Ice/InstanceF.h>

namespace IceInternal
{

//
// Global function to obtain a ProtocolPluginFacade given a Communicator
// instance.
//
ICE_PROTOCOL_API ProtocolPluginFacadePtr getProtocolPluginFacade(const Ice::CommunicatorPtr&);

//
// ProtocolPluginFacade wraps the internal operations that protocol
// plug-ins may need.
//
class ICE_PROTOCOL_API ProtocolPluginFacade : public ::IceUtil::Shared
{
public:

    //
    // Get the Communicator instance with which this facade is
    // associated.
    //
    Ice::CommunicatorPtr getCommunicator() const;

    //
    // Get the default hostname to be used in endpoints.
    //
    std::string getDefaultHost() const;

    //
    // Get the network trace level and category name.
    //
    int getNetworkTraceLevel() const;
    const char* getNetworkTraceCategory() const;

    //
    // Register an EndpointFactory.
    //
    void addEndpointFactory(const EndpointFactoryPtr&) const;

private:

    ProtocolPluginFacade(const Ice::CommunicatorPtr&);

    friend ICE_PROTOCOL_API ProtocolPluginFacadePtr getProtocolPluginFacade(const Ice::CommunicatorPtr&);

    InstancePtr _instance;
    Ice::CommunicatorPtr _communicator;
};

}

#endif
