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

#include <IceUtil/IceUtil.h>
#include <TestI.h>
#include <Ice/Ice.h>

using namespace std;

class ServantFactory : public Ice::ObjectFactory
{
public:

    virtual Ice::ObjectPtr
    create(const string& type)
    {
        assert(type == "::Test::Servant");
        return new Test::ServantI;
    }

    virtual void
    destroy()
    {
    }
};


class FacetFactory : public Ice::ObjectFactory
{
public:

    virtual Ice::ObjectPtr
    create(const string& type)
    {
        assert(type == "::Test::Facet");
        return new Test::FacetI;
    }

    virtual void
    destroy()
    {
    }
};
int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator, const string& envName)
{
    communicator->getProperties()->setProperty("Factory.Endpoints", "default -p 12345 -t 2000");

    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("Factory");

    Test::RemoteEvictorFactoryPtr factory = new Test::RemoteEvictorFactoryI(adapter, envName);
    adapter->add(factory, Ice::stringToIdentity("factory"));

    Ice::ObjectFactoryPtr servantFactory = new ServantFactory;
    communicator->addObjectFactory(servantFactory, "::Test::Servant");

    Ice::ObjectFactoryPtr facetFactory = new FacetFactory;
    communicator->addObjectFactory(facetFactory, "::Test::Facet");

    adapter->activate();

    communicator->waitForShutdown();

    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    int status;
    Ice::CommunicatorPtr communicator;
    string envName = "db";

    try
    {
        communicator = Ice::initialize(argc, argv);
        status = run(argc, argv, communicator, envName);
    }
    catch(const Ice::Exception& ex)
    {
        cerr << ex << endl;
        status = EXIT_FAILURE;
    }

    if(communicator)
    {
        try
        {
            communicator->destroy();
        }
        catch(const Ice::Exception& ex)
        {
            cerr << ex << endl;
            status = EXIT_FAILURE;
        }
    }

    return status;
}
