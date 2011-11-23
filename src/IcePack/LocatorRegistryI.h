
// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#ifndef ICE_PACK_LOCATOR_REGISTRY_I_H
#define ICE_PACK_LOCATOR_REGISTRY_I_H

#include <IcePack/Internal.h>
#include <Ice/Locator.h>

namespace IcePack
{

class AdapterFactory;
typedef IceUtil::Handle<AdapterFactory> AdapterFactoryPtr;

class LocatorRegistryI : public Ice::LocatorRegistry
{
public:

    LocatorRegistryI(const AdapterRegistryPtr&, const ServerRegistryPtr&, const AdapterFactoryPtr&, bool);
    
    virtual void setAdapterDirectProxy(const ::std::string&, const ::Ice::ObjectPrx&, const ::Ice::Current&);
    virtual void setServerProcessProxy(const ::std::string&, const ::Ice::ProcessPrx&, const ::Ice::Current&);

private:
    
    AdapterRegistryPtr _adapterRegistry;
    ServerRegistryPtr _serverRegistry;
    AdapterFactoryPtr _adapterFactory;
    const bool _dynamicRegistration;
};

}

#endif
