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
#include <IceBox/IceBox.h>
#include <Freeze/Freeze.h>
#include <TestI.h>

#ifndef TEST_SERVICE_API
#   define TEST_SERVICE_API ICE_DECLSPEC_EXPORT
#endif


using namespace std;
using namespace Ice;

class TEST_SERVICE_API ServiceI : public ::IceBox::Service
{
public:

    ServiceI();
    virtual ~ServiceI();

    virtual void start(const string&,
                      const CommunicatorPtr&,
                      const StringSeq&);

    virtual void stop();
};

class TEST_SERVICE_API FreezeServiceI : public ::IceBox::FreezeService
{
public:

    FreezeServiceI();
    virtual ~FreezeServiceI();

    virtual void start(const string&,
		       const CommunicatorPtr&,
		       const StringSeq&,
		       const Freeze::DBEnvironmentPtr&);

    virtual void stop();
};

extern "C"
{

//
// Factory function
//
TEST_SERVICE_API ::IceBox::Service*
create(CommunicatorPtr communicator)
{
    return new ServiceI;
}

TEST_SERVICE_API ::IceBox::FreezeService*
createFreezeService(CommunicatorPtr communicator)
{
    return new FreezeServiceI;
}

}

ServiceI::ServiceI()
{
}

ServiceI::~ServiceI()
{
}

void
ServiceI::start(const string& name,
		const CommunicatorPtr& communicator,
		const StringSeq& args)
{
    Ice::PropertiesPtr properties = communicator->getProperties();

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter(name);
    Ice::ObjectPtr object = new TestI(adapter, properties);
    adapter->add(object, Ice::stringToIdentity(properties->getProperty(name + ".Identity")));
    adapter->activate();
}

void
ServiceI::stop()
{
}

FreezeServiceI::FreezeServiceI()
{
}

FreezeServiceI::~FreezeServiceI()
{
}

void
FreezeServiceI::start(const string& name,
		      const CommunicatorPtr& communicator,
		      const StringSeq& args,
		      const Freeze::DBEnvironmentPtr& dbEnv)
{
    //
    // Ensure that we can create an environment.
    //
    Freeze::DBPtr db = dbEnv->openDB("testdb", true);

    Ice::PropertiesPtr properties = communicator->getProperties();

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter(name);
    Ice::ObjectPtr object = new TestI(adapter, communicator->getProperties());
    adapter->add(object, Ice::stringToIdentity(properties->getProperty(name + ".Identity")));
    adapter->activate();
}

void
FreezeServiceI::stop()
{
}
