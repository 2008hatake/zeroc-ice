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
#include <IceStorm/IceStorm.h>
#include <Single.h>

using namespace std;
using namespace Ice;
using namespace IceStorm;

int
run(int argc, char* argv[], const CommunicatorPtr& communicator)
{
    PropertiesPtr properties = communicator->getProperties();
    const char* managerProxyProperty = "IceStorm.TopicManager.Proxy";
    string managerProxy = properties->getProperty(managerProxyProperty);
    if(managerProxy.empty())
    {
	cerr << argv[0] << ": property `" << managerProxyProperty << "' is not set" << endl;
	return EXIT_FAILURE;
    }

    ObjectPrx base = communicator->stringToProxy(managerProxy);
    IceStorm::TopicManagerPrx manager = IceStorm::TopicManagerPrx::checkedCast(base);
    if(!manager)
    {
	cerr << argv[0] << ": `" << managerProxy << "' is not running" << endl;
	return EXIT_FAILURE;
    }

    TopicPrx topic;
    try
    {
	topic = manager->retrieve("single");
    }
    catch(const NoSuchTopic& e)
    {
	cerr << argv[0] << ": NoSuchTopic: " << e.name << endl;
	return EXIT_FAILURE;
	
    }
    assert(topic);

    //
    // Get a publisher object, create a oneway proxy and then cast to
    // a Single object.
    //
    SinglePrx single = SinglePrx::uncheckedCast(topic->getPublisher()->ice_oneway());

    for(int i = 0; i < 10; ++i)
    {
	single->event();
    }

    //
    // Before we exit, we ping the proxy as twoway, to make sure that
    // all oneways are delivered.
    //
    SinglePrx::uncheckedCast(single->ice_twoway())->ice_ping();

    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    int status;
    CommunicatorPtr communicator;

    try
    {
	communicator = initialize(argc, argv);
	status = run(argc, argv, communicator);
    }
    catch(const Exception& ex)
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
	catch(const Exception& ex)
	{
	    cerr << ex << endl;
	    status = EXIT_FAILURE;
	}
    }

    return status;
}
