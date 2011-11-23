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
#include <TestI.h>

using namespace std;

void
usage(const char* n)
{
    cerr << "Usage: " << n << " port\n";
}

int
run(int argc, char* argv[], const Ice::CommunicatorPtr& communicator)
{
    int port = 0;
    for(int i = 1; i < argc; ++i)
    {
	if(argv[i][0] == '-')
	{
	    cerr << argv[0] << ": unknown option `" << argv[i] << "'" << endl;
	    usage(argv[0]);
	    return EXIT_FAILURE;
	}

	if(port > 0)
	{
	    cerr << argv[0] << ": only one port can be specified" << endl;
	    usage(argv[0]);
	    return EXIT_FAILURE;
	}

	port = atoi(argv[i]);
    }

    if(port <= 0)
    {
	cerr << argv[0] << ": no port specified" << endl;
	usage(argv[0]);
	return EXIT_FAILURE;
    }

    ostringstream endpts;
    endpts << "default  -p " << port;
    communicator->getProperties()->setProperty("TestAdapter.Endpoints", endpts.str());
    Ice::ObjectAdapterPtr adapter = communicator->createObjectAdapter("TestAdapter");
    Ice::ObjectPtr object = new TestI(adapter);
    adapter->add(object, Ice::stringToIdentity("test"));
    adapter->activate();
    communicator->waitForShutdown();
    return EXIT_SUCCESS;
}

int
main(int argc, char* argv[])
{
    int status;
    Ice::CommunicatorPtr communicator;

    try
    {
        //
	// In this test, we need a longer server idle time, otherwise
	// our test servers may time out before they are used in the
	// test.
	//
	Ice::PropertiesPtr properties = Ice::getDefaultProperties(argc, argv);
	properties->setProperty("Ice.ServerIdleTime", "120"); // Two minutes.

	communicator = Ice::initialize(argc, argv);
	status = run(argc, argv, communicator);
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
