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

#ifndef ICE_PACK_LOCATOR_I_H
#define ICE_PACK_LOCATOR_I_H

#include <IcePack/Internal.h>
#include <Ice/Locator.h>

namespace IcePack
{

class LocatorI : public ::Ice::Locator
{
public:

    LocatorI(const AdapterRegistryPtr&, const ObjectRegistryPtr&, const ::Ice::LocatorRegistryPrx&);

    virtual void findObjectById_async(const ::Ice::AMD_Locator_findObjectByIdPtr&, const ::Ice::Identity&, 
				      const ::Ice::Current&) const;

    virtual void findAdapterById_async(const ::Ice::AMD_Locator_findAdapterByIdPtr&, const ::std::string&, 
				       const ::Ice::Current&) const;

    virtual ::Ice::LocatorRegistryPrx getRegistry(const ::Ice::Current&) const;

private:
    
    AdapterRegistryPtr _adapterRegistry;
    ObjectRegistryPtr _objectRegistry;
    Ice::LocatorRegistryPrx _locatorRegistry;
};

}

#endif
